#pragma once

#include "core/core.h"
#include "core/math/matrix.h"
#include "core/math/vector.h"
#include "core/memory/arena.h"
#include "core/thread_context.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

struct Vertex {
	vec3 position;
	f32 tu;
	vec3 normal;
	f32 tv;
	vec4 color;
};

struct Mesh {
	Vertex* vertices;
	u32 vertexCount;

	u32* indices;
	u32 indexCount;
};

struct Scene {
	Mesh* meshes;
	u32 meshCount;

	mat4* transforms;

	b32 valid;
};

inline Scene* parseGLTF(Arena* arena, String8 path) {
	Scene* result = PushStruct(arena, Scene);

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

	result->meshes = PushArray(arena, Mesh, data->meshes_count);
	result->meshCount = data->meshes_count;

	for (u32 i = 0; i < data->meshes_count; ++i) {
		cgltf_mesh& cmesh = data->meshes[i];
		Mesh* mesh = &result->meshes[i];

		for (u32 pi = 0; pi < cmesh.primitives_count; ++pi) {
			cgltf_primitive& primitive = cmesh.primitives[pi];
			if (primitive.type != cgltf_primitive_type_triangles || !primitive.indices) {
				printf("[GLTF] Non triangle-based primitive or no indices.");
				continue;
			}

			u32 vertexCount = primitive.attributes[0].data->count;
			u32 indexCount = primitive.indices->count;

			mesh->vertices = PushArray(arena, Vertex, vertexCount);
			mesh->indices = PushArray(arena, u32, indexCount);

			Temp scratch = ScratchBegin(&arena, 1);

			f32* scratchBuffer = PushArray(scratch.arena, f32, vertexCount * 4);

			if (const cgltf_accessor* pos = cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0)) {
				Assert(cgltf_num_components(pos->type) == 3);
				cgltf_accessor_unpack_floats(pos, scratchBuffer, vertexCount * 3);

				for (u32 j = 0; j < vertexCount; ++j) {
					mesh->vertices[j].position = { scratchBuffer[j * 3 + 0], scratchBuffer[j * 3 + 1], scratchBuffer[j * 3 + 2] };
				}
			}

			if (const cgltf_accessor* pos = cgltf_find_accessor(&primitive, cgltf_attribute_type_normal, 0)) {
				Assert(cgltf_num_components(pos->type) == 3);
				cgltf_accessor_unpack_floats(pos, scratchBuffer, vertexCount * 3);

				for (u32 j = 0; j < vertexCount; ++j) {
					mesh->vertices[j].normal = { scratchBuffer[j * 3 + 0], scratchBuffer[j * 3 + 1], scratchBuffer[j * 3 + 2] };
				}
			}

			if (const cgltf_accessor* pos = cgltf_find_accessor(&primitive, cgltf_attribute_type_texcoord, 0)) {
				Assert(cgltf_num_components(pos->type) == 2);
				cgltf_accessor_unpack_floats(pos, scratchBuffer, vertexCount * 2);

				for (u32 j = 0; j < vertexCount; ++j) {
					mesh->vertices[j].tu = scratchBuffer[j * 2 + 0];
					mesh->vertices[j].tv = scratchBuffer[j * 2 + 1];
				}
			}

			ScratchEnd(scratch);

			cgltf_accessor_unpack_indices(primitive.indices, mesh->indices, 4, indexCount);

			mesh->vertexCount = vertexCount;
			mesh->indexCount = indexCount;
		}
	}

	// NOTE(piero): nodes_count should be the higher bound for the amount we can expect.
	result->transforms = PushArray(arena, mat4, data->nodes_count);
	for (u32 i = 0; i < data->nodes_count; ++i) {
		const cgltf_node* node = &data->nodes[i];

		if (node->mesh) {
			f32 matrix[16]{};
			cgltf_node_transform_world(node, matrix);
			u32 meshIndex = cgltf_mesh_index(data, node->mesh);
			result->transforms[meshIndex] = matrixFromArray(matrix);
		}
	}

	printf("Loaded %zu meshes", data->meshes_count);

	cgltf_free(data);

	result->valid = true;

	return result;
}
