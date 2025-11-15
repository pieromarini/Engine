#pragma once

#include "core/core.h"
#include "core/core_strings.h"
#include "core/math/matrix.h"
#include "core/math/vector.h"
#include "core/memory/arena.h"
#include "core/thread_context.h"
#include "core/perf/scope_profiler.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

struct Vertex {
	vec3 position;
	f32 tu;
	vec3 normal;
	f32 tv;
	vec4 color;
};

struct alignas(16) Material {
	i32 albedo;
	i32 normal;
	i32 specular;
	i32 emissive;

	vec4 diffuseFactor;
	vec4 specularFactor;
	vec3 emissiveFactor;
};

struct Texture {
	u8* data;
	i32 width;
	i32 height;
	i32 dataSize;
};

struct Geometry {
	Vertex* vertices;
	u32 vertexCount;

	u32* indices;
	u32 indexCount;
};

struct GeometryPrimitive {
	u32 indexCount;
	u32 firstIndex;
	i32 vertexOffset;
};

struct GeometryDrawData {
	mat4 transform;
	u32 materialIndex;
	f32 padding[3];
};

struct Model {
	Geometry* geometry;
	GeometryPrimitive* primitives;
	u32 primitivesCount;

	GeometryDrawData* drawData;
	u32 drawDataCount;

	Material* materials;
	u32 materialCount;

	Texture* textures;
	u32 textureCount;

	b32 valid;
};

inline Model* parseGLTF(Arena* arena, String8 path) {
	PerfScope;

	Model* result = PushStruct(arena, Model);

	cgltf_options options = {};
	cgltf_data* data = nullptr;
	cgltf_result parsedResult = cgltf_parse_file(&options, (char*)path.str, &data);

	if (parsedResult != cgltf_result_success) {
		return result;
	}

	parsedResult = cgltf_load_buffers(&options, data, (char*)path.str);
	if (parsedResult != cgltf_result_success) {
		return result;
	}

	parsedResult = cgltf_validate(data);
	if (parsedResult != cgltf_result_success) {
		return result;
	}

	result->geometry = PushStruct(arena, Geometry);

	Geometry* geometry = result->geometry;

	u32 modelVertexCount = 0;
	u32 modelIndexCount = 0;
	u32 modelPrimitiveCount = 0;

	result->textures = PushArray(arena, Texture, data->textures_count);
	result->textureCount = data->textures_count;
	for (u32 i = 0; i < data->textures_count; ++i) {
		cgltf_texture* texture = &data->textures[i];
		Assert(texture->image);

		u32 textureIndex = cgltf_texture_index(data, texture);

		cgltf_image* image = texture->image;
		if (image->buffer_view) {
			cgltf_buffer_view* view = image->buffer_view;
			cgltf_buffer* buffer = view->buffer;
			u8* bytes = (u8*)buffer->data + view->offset;
			i32 size = (i32)view->size;

			i32 width = 0, height = 0, nChannels = 0;
			u8* pixels = stbi_load_from_memory(bytes, size, &width, &height, &nChannels, 4);

			if (!pixels) {
				printf("Failed to load image");
				continue;
			}

			result->textures[textureIndex] = {
				.data = pixels,
				.width = width,
				.height = height,
				.dataSize = size
			};
		} else if (image->uri) {
			printf("Image URI loading not implemented.");
			Assert(false);
		}
	}


	for (u32 i = 0; i < data->meshes_count; ++i) {
		cgltf_mesh& cmesh = data->meshes[i];

		for (u32 pi = 0; pi < cmesh.primitives_count; ++pi) {
			cgltf_primitive& primitive = cmesh.primitives[pi];
			if (primitive.type != cgltf_primitive_type_triangles || !primitive.indices) {
				printf("[GLTF] Non triangle-based primitive or no indices.");
				continue;
			}

			for (u32 j = 0; j < primitive.attributes_count; ++j) {
				if (primitive.attributes[j].type == cgltf_attribute_type_position) {
					modelVertexCount += primitive.attributes[j].data->count;
				}
			}
			modelIndexCount += primitive.indices->count;
			modelPrimitiveCount++;
		}
	}

	geometry->vertices = PushArray(arena, Vertex, modelVertexCount);
	geometry->vertexCount = modelVertexCount;

	geometry->indices = PushArray(arena, u32, modelIndexCount);
	geometry->indexCount = modelIndexCount;

	result->primitives = PushArray(arena, GeometryPrimitive, modelPrimitiveCount);
	result->primitivesCount = modelPrimitiveCount;

	result->drawData = PushArray(arena, GeometryDrawData, modelPrimitiveCount);
	result->drawDataCount = modelPrimitiveCount;

	u32 vertexOffset = 0;
	u32 indexOffset = 0;
	u32 primitiveIndex = 0;

	for (u32 i = 0; i < data->meshes_count; ++i) {
		cgltf_mesh& cmesh = data->meshes[i];

		for (u32 pi = 0; pi < cmesh.primitives_count; ++pi) {
			cgltf_primitive& primitive = cmesh.primitives[pi];
			if (primitive.type != cgltf_primitive_type_triangles || !primitive.indices) {
				printf("[GLTF] Non triangle-based primitive or no indices.");
				continue;
			}

			GeometryPrimitive* prim = &result->primitives[primitiveIndex];

			const cgltf_accessor* posAcc = cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0);
			if (!posAcc) {
				printf("[GLTF] Primitive without positions.");
				continue;
			}
			u32 vertexCount = (u32)posAcc->count;
			u32 indexCount = primitive.indices->count;

			// Fill primitive info for draw calls
			prim->firstIndex = indexOffset;
			prim->vertexOffset = (i32)vertexOffset;
			prim->indexCount = indexCount;
			result->drawData[primitiveIndex].materialIndex = cgltf_material_index(data, primitive.material);

			Temp scratch = ScratchBegin(&arena, 1);

			f32* scratchBuffer = PushArray(scratch.arena, f32, vertexCount * 4);

			if (const cgltf_accessor* pos = cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0)) {
				Assert(cgltf_num_components(pos->type) == 3);
				cgltf_accessor_unpack_floats(pos, scratchBuffer, vertexCount * 3);

				for (u32 j = 0; j < vertexCount; ++j) {
					geometry->vertices[vertexOffset + j].position = { scratchBuffer[j * 3 + 0], scratchBuffer[j * 3 + 1], scratchBuffer[j * 3 + 2] };
				}
			}

			if (const cgltf_accessor* pos = cgltf_find_accessor(&primitive, cgltf_attribute_type_normal, 0)) {
				Assert(cgltf_num_components(pos->type) == 3);
				cgltf_accessor_unpack_floats(pos, scratchBuffer, vertexCount * 3);

				for (u32 j = 0; j < vertexCount; ++j) {
					geometry->vertices[vertexOffset + j].normal = { scratchBuffer[j * 3 + 0], scratchBuffer[j * 3 + 1], scratchBuffer[j * 3 + 2] };
				}
			}

			if (const cgltf_accessor* pos = cgltf_find_accessor(&primitive, cgltf_attribute_type_texcoord, 0)) {
				Assert(cgltf_num_components(pos->type) == 2);
				cgltf_accessor_unpack_floats(pos, scratchBuffer, vertexCount * 2);

				for (u32 j = 0; j < vertexCount; ++j) {
					geometry->vertices[vertexOffset + j].tu = scratchBuffer[j * 2 + 0];
					geometry->vertices[vertexOffset + j].tv = scratchBuffer[j * 2 + 1];
				}
			}

			ScratchEnd(scratch);

			cgltf_accessor_unpack_indices(primitive.indices, geometry->indices + indexOffset, 4, indexCount);

			vertexOffset += vertexCount;
			indexOffset += indexCount;
			primitiveIndex++;
		}
	}

	Assert(vertexOffset == modelVertexCount);

	for (u32 i = 0; i < data->nodes_count; ++i) {
		const cgltf_node* node = &data->nodes[i];

		if (node->mesh) {
			f32 matrix[16]{};
			cgltf_node_transform_world(node, matrix);

			u32 meshIndex = cgltf_mesh_index(data, node->mesh);
			result->drawData[meshIndex].transform = matrixFromArray(matrix);
		}
	}

	result->materials = PushArray(arena, Material, data->materials_count);
	result->materialCount = data->materials_count;
	for (u32 i = 0; i < data->materials_count; ++i) {
		const cgltf_material* material = &data->materials[i];

		Material* mat = &result->materials[i];

		mat->diffuseFactor = { 1.0f, 1.0f, 1.0f, 1.0f };

		if (material->has_pbr_specular_glossiness) {
			if (material->pbr_specular_glossiness.diffuse_texture.texture) {
				mat->albedo = i32(cgltf_texture_index(data, material->pbr_specular_glossiness.diffuse_texture.texture));
			}
			memcpy(mat->diffuseFactor.elements, material->pbr_specular_glossiness.diffuse_factor, sizeof(cgltf_float) * 4);

			if (material->pbr_specular_glossiness.specular_glossiness_texture.texture) {
				mat->specular = i32(cgltf_texture_index(data, material->pbr_specular_glossiness.specular_glossiness_texture.texture));
			}
			mat->specularFactor = vec4{ material->pbr_specular_glossiness.specular_factor[0], material->pbr_specular_glossiness.specular_factor[1], material->pbr_specular_glossiness.specular_factor[2], material->pbr_specular_glossiness.glossiness_factor };

		} else if (material->has_pbr_metallic_roughness) {
			if (material->pbr_metallic_roughness.base_color_texture.texture) {
				mat->albedo = i32(cgltf_texture_index(data, material->pbr_metallic_roughness.base_color_texture.texture));
			}
			memcpy(mat->diffuseFactor.elements, material->pbr_metallic_roughness.base_color_factor, sizeof(cgltf_float) * 4);

			if (material->pbr_metallic_roughness.metallic_roughness_texture.texture) {
				mat->specular = i32(cgltf_texture_index(data, material->pbr_metallic_roughness.metallic_roughness_texture.texture));
			}
			mat->specularFactor = vec4{ 1.0f, 1.0f, 1.0f, 1.0f - material->pbr_metallic_roughness.roughness_factor };
		}

		if (material->normal_texture.texture) {
			mat->normal = i32(cgltf_texture_index(data, material->normal_texture.texture));
		}

		if (material->emissive_texture.texture) {
			mat->emissive = i32(cgltf_texture_index(data, material->emissive_texture.texture));
		}

		mat->emissiveFactor = vec3{ material->emissive_factor[0], material->emissive_factor[1], material->emissive_factor[2] };
	}

	printf("Loaded %zu meshes. Vertices: %u | Indices: %u", data->meshes_count, result->geometry->vertexCount, result->geometry->indexCount);

	cgltf_free(data);

	result->valid = true;

	return result;
}
