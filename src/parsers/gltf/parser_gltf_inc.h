#pragma once

#include "core/core.h"
#include "core/core_strings.h"
#include "core/math/core_math.h"
#include "core/memory/arena.h"
#include "core/perf/scope_profiler.h"
#include "core/thread_context.h"


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
	u32 instanceCount;
	u32 firstInstance;
};

struct GeometryDrawData {
	u32 materialIndex;
	f32 padding[3];
};

struct GeometryInstanceData {
	mat4 transform;
};

struct Model {
	Geometry* geometry;

	GeometryPrimitive* primitives;
	u32 primitivesCount;

	GeometryDrawData* drawData;
	u32 drawDataCount;

	GeometryInstanceData* instanceData;
	u32 instanceCount;

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
		cgltf_free(data);
		return result;
	}

	parsedResult = cgltf_validate(data);
	if (parsedResult != cgltf_result_success) {
		cgltf_free(data);
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
			i32 encodedSize = (i32)view->size;

			i32 width = 0, height = 0, nChannels = 0;
			u8* pixels = stbi_load_from_memory(bytes, encodedSize, &width, &height, &nChannels, 4);

			if (!pixels) {
				printf("Failed to load image from buffer_view for texture %u\n", textureIndex);
				continue;
			}

			result->textures[textureIndex] = {
				.data = pixels,
				.width = width,
				.height = height,
				.dataSize = width * height * 4
			};
		} else if (image->uri) {
			u64 beforeFilenameIdx = FindSubstr8(path, Str8L("/"), 0, MatchFlag_FindLast);
			String8 basePath = Substr8(path, 0, beforeFilenameIdx);
			String8 imagePath = PushStr8F(arena, "%S/%S", basePath, Str8C(image->uri));

			i32 width = 0, height = 0, nChannels = 0;
			u8* pixels = stbi_load((char*)imagePath.str, &width, &height, &nChannels, 4);

			if (!pixels) {
				printf("Failed to load image from uri for texture %u -> %s\n", textureIndex, imagePath.str);
				continue;
			}

			result->textures[textureIndex] = {
				.data = pixels,
				.width = width,
				.height = height,
				.dataSize = width * height * 4
			};
		}
	}

	u32 maxPrimitivesPerMesh = 1;
	for (u32 i = 0; i < data->meshes_count; ++i) {
		cgltf_mesh& cmesh = data->meshes[i];
		u32 counted = 0;
		for (u32 pi = 0; pi < cmesh.primitives_count; ++pi) {
			cgltf_primitive& primitive = cmesh.primitives[pi];
			if (primitive.type != cgltf_primitive_type_triangles || !primitive.indices) {
				continue;
			}
			for (u32 j = 0; j < primitive.attributes_count; ++j) {
				if (primitive.attributes[j].type == cgltf_attribute_type_position) {
					modelVertexCount += (u32)primitive.attributes[j].data->count;
				}
			}
			modelIndexCount += (u32)primitive.indices->count;
			modelPrimitiveCount++;
			counted++;
		}
		if (counted > maxPrimitivesPerMesh) maxPrimitivesPerMesh = counted;
	}

	geometry->vertices = PushArray(arena, Vertex, modelVertexCount);
	geometry->vertexCount = modelVertexCount;

	geometry->indices = PushArray(arena, u32, modelIndexCount);
	geometry->indexCount = modelIndexCount;

	result->primitives = PushArray(arena, GeometryPrimitive, modelPrimitiveCount);
	result->primitivesCount = modelPrimitiveCount;

	result->drawData = PushArray(arena, GeometryDrawData, modelPrimitiveCount);
	result->drawDataCount = modelPrimitiveCount;

	i32* meshFirstPrimitive = PushArrayNoZero(arena, i32, data->meshes_count);
	u32* meshPrimitiveCount = PushArray(arena, u32, data->meshes_count);

	MemorySet(meshFirstPrimitive, -1, sizeof(i32) * data->meshes_count);

	u32 vertexOffset = 0;
	u32 indexOffset = 0;
	u32 primitiveIndex = 0;

	for (u32 i = 0; i < data->meshes_count; ++i) {
		cgltf_mesh& cmesh = data->meshes[i];

		for (u32 pi = 0; pi < cmesh.primitives_count; ++pi) {
			cgltf_primitive& primitive = cmesh.primitives[pi];
			if (primitive.type != cgltf_primitive_type_triangles || !primitive.indices) {
				continue;
			}

			// If this is the first stored primitive for this mesh, record the base index
			if (meshFirstPrimitive[i] == -1) {
				meshFirstPrimitive[i] = (i32)primitiveIndex;
			}
			meshPrimitiveCount[i] += 1;

			GeometryPrimitive* prim = &result->primitives[primitiveIndex];

			const cgltf_accessor* posAcc = cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0);
			if (!posAcc) {
				printf("[GLTF] Primitive without positions.\n");
				continue;
			}

			u32 vertexCount = (u32)posAcc->count;
			u32 indexCount = (u32)primitive.indices->count;

			// Fill primitive info for draw calls
			prim->firstIndex = indexOffset;
			prim->vertexOffset = (i32)vertexOffset;
			prim->indexCount = indexCount;
			prim->instanceCount = 0;
			prim->firstInstance = 0;

			if (primitive.material) {
				result->drawData[primitiveIndex].materialIndex = (u32)cgltf_material_index(data, primitive.material);
			} else {
				result->drawData[primitiveIndex].materialIndex = 0;// default material
			}

			Temp scratch = ScratchBegin(&arena, 1);

			f32* scratchBuffer = PushArray(scratch.arena, f32, vertexCount * 4);

			// positions
			if (const cgltf_accessor* pos = cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0)) {
				Assert(cgltf_num_components(pos->type) == 3);
				cgltf_accessor_unpack_floats(pos, scratchBuffer, vertexCount * 3);

				for (u32 j = 0; j < vertexCount; ++j) {
					geometry->vertices[vertexOffset + j].position = { scratchBuffer[j * 3 + 0], scratchBuffer[j * 3 + 1], scratchBuffer[j * 3 + 2] };
				}
			}

			// normals
			if (const cgltf_accessor* nrm = cgltf_find_accessor(&primitive, cgltf_attribute_type_normal, 0)) {
				Assert(cgltf_num_components(nrm->type) == 3);
				cgltf_accessor_unpack_floats(nrm, scratchBuffer, vertexCount * 3);

				for (u32 j = 0; j < vertexCount; ++j) {
					geometry->vertices[vertexOffset + j].normal = { scratchBuffer[j * 3 + 0], scratchBuffer[j * 3 + 1], scratchBuffer[j * 3 + 2] };
				}
			}

			// texcoords
			if (const cgltf_accessor* tex = cgltf_find_accessor(&primitive, cgltf_attribute_type_texcoord, 0)) {
				Assert(cgltf_num_components(tex->type) == 2);
				cgltf_accessor_unpack_floats(tex, scratchBuffer, vertexCount * 2);

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

	result->materials = PushArray(arena, Material, data->materials_count);
	result->materialCount = data->materials_count;

	for (u32 i = 0; i < data->materials_count; ++i) {
		const cgltf_material* material = &data->materials[i];
		Material* mat = &result->materials[i];

		mat->diffuseFactor = { 1.0f, 1.0f, 1.0f, 1.0f };

		if (material->has_pbr_specular_glossiness) {
			if (material->pbr_specular_glossiness.diffuse_texture.texture) {
				mat->albedo = (i32)cgltf_texture_index(data, material->pbr_specular_glossiness.diffuse_texture.texture);
			}
			memcpy(mat->diffuseFactor.elements, material->pbr_specular_glossiness.diffuse_factor, sizeof(cgltf_float) * 4);

			if (material->pbr_specular_glossiness.specular_glossiness_texture.texture) {
				mat->specular = (i32)cgltf_texture_index(data, material->pbr_specular_glossiness.specular_glossiness_texture.texture);
			}
			mat->specularFactor = vec4{ material->pbr_specular_glossiness.specular_factor[0], material->pbr_specular_glossiness.specular_factor[1], material->pbr_specular_glossiness.specular_factor[2], material->pbr_specular_glossiness.glossiness_factor };

		} else if (material->has_pbr_metallic_roughness) {
			if (material->pbr_metallic_roughness.base_color_texture.texture) {
				mat->albedo = (i32)cgltf_texture_index(data, material->pbr_metallic_roughness.base_color_texture.texture);
			}
			memcpy(mat->diffuseFactor.elements, material->pbr_metallic_roughness.base_color_factor, sizeof(cgltf_float) * 4);

			if (material->pbr_metallic_roughness.metallic_roughness_texture.texture) {
				mat->specular = (i32)cgltf_texture_index(data, material->pbr_metallic_roughness.metallic_roughness_texture.texture);
			}
			mat->specularFactor = vec4{ 1.0f, 1.0f, 1.0f, 1.0f - material->pbr_metallic_roughness.roughness_factor };
		}

		if (material->normal_texture.texture) {
			mat->normal = (i32)cgltf_texture_index(data, material->normal_texture.texture);
		}

		if (material->emissive_texture.texture) {
			mat->emissive = (i32)cgltf_texture_index(data, material->emissive_texture.texture);
		}

		mat->emissiveFactor = vec3{ material->emissive_factor[0], material->emissive_factor[1], material->emissive_factor[2] };
	}

	u32* primitiveInstanceCounts = PushArray(arena, u32, result->primitivesCount);

	// count how many instances each primitive will have
	for (u32 n = 0; n < data->nodes_count; ++n) {
		const cgltf_node* node = &data->nodes[n];
		if (!node->mesh) continue;

		u32 meshIndex = cgltf_mesh_index(data, node->mesh);
		if (meshIndex >= data->meshes_count) continue;

		i32 startPrimitiveIndex = meshFirstPrimitive[meshIndex];
		u32 primitiveCount = meshPrimitiveCount[meshIndex];

		if (startPrimitiveIndex == -1 || primitiveCount == 0) continue;

		for (u32 p = 0; p < primitiveCount; ++p) {
			primitiveInstanceCounts[startPrimitiveIndex + p] += 1;
		}
	}

	// compute firstInstance and total instance count
	u32 totalInstanceCount = 0;
	for (u32 p = 0; p < result->primitivesCount; ++p) {
		u32 count = primitiveInstanceCounts[p];
		result->primitives[p].firstInstance = totalInstanceCount;
		result->primitives[p].instanceCount = count;
		totalInstanceCount += count;
	}

	result->instanceData = PushArray(arena, GeometryInstanceData, totalInstanceCount);
	result->instanceCount = totalInstanceCount;

	MemoryZero(primitiveInstanceCounts, sizeof(u32) * result->primitivesCount);

	// Pass 2: fill instanceData contiguously per-primitive
	for (u32 n = 0; n < data->nodes_count; ++n) {
		const cgltf_node* node = &data->nodes[n];
		if (!node->mesh) continue;

		f32 matrixArr[16]{};
		cgltf_node_transform_world(node, matrixArr);
		mat4 transform = matrixFromArray(matrixArr);

		u32 meshIndex = cgltf_mesh_index(data, node->mesh);
		if (meshIndex >= data->meshes_count) continue;

		i32 basePrim = meshFirstPrimitive[meshIndex];
		u32 primCount = meshPrimitiveCount[meshIndex];

		if (basePrim == -1 || primCount == 0) continue;

		for (u32 p = 0; p < primCount; ++p) {
			u32 primIdx = basePrim + p;
			u32 instanceCount = primitiveInstanceCounts[primIdx];
			u32 instanceIndex = result->primitives[primIdx].firstInstance + instanceCount;

			result->instanceData[instanceIndex].transform = transform;

			primitiveInstanceCounts[primIdx] += 1;
		}
	}

	printf("Loaded %zu meshes. Vertices: %u | Indices: %u | Primitives: %u | Instances: %u\n", data->meshes_count, result->geometry->vertexCount, result->geometry->indexCount, result->primitivesCount, result->instanceCount);

	cgltf_free(data);

	result->valid = true;
	return result;
}
