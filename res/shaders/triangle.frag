#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

#define DEBUG 0

uint hash(uint a) {
	a = (a+0x7ed55d16) + (a<<12);
	a = (a^0xc761c23c) ^ (a>>19);
	a = (a+0x165667b1) + (a<<5);
	a = (a+0xd3a2646c) ^ (a<<9);
	a = (a+0xfd7046c5) + (a<<3);
	a = (a^0xb55a4f09) ^ (a>>16);
	return a;
}

struct Vertex {
	vec3 position;
	float tu;
	vec3 normal;
	float tv;
	vec4 color;
};

struct IndirectCommandData {
	uint drawId;

	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	int vertexOffset;
	uint firstInstance;
};

struct MaterialData {
	uint albedoTexture;
	uint normalTexture;
	uint specularTexture;
	uint emissiveTexture;
	vec4 colorFactors;
	vec4 metal_rough_factors;
};

struct MeshDraw {
	mat4 transform;
	uint materialIndex;
	float padding[3];
};

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in flat uint drawId;
layout (location = 4) in vec3 inFragPos;

layout (location = 0) out vec4 outFragColor;

layout(buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};

layout(push_constant, std430) uniform pc {
	mat4 viewProj;
	VertexBuffer vertexBuffer;
} PushConstants;

/*
layout (set = 1, binding = 0) uniform sampler2D textures[];
*/

layout (set = 0, binding = 0) readonly buffer GLTFMaterialData {
	MaterialData materialData[];
};

layout (set = 0, binding = 1) readonly buffer DrawCommands {
	IndirectCommandData drawCommands[];
};

layout (std430, set = 0, binding = 2) readonly buffer Draws {
	MeshDraw draws[];
};

void main() {
	MeshDraw meshDraw = draws[drawId];
	MaterialData material = materialData[meshDraw.materialIndex];

	vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
	vec3 N = normalize(inNormal);

	float diff = max(dot(N, lightDir), 0.0);

	vec3 baseColor = vec3(0.5, 0.5, 0.5);

	vec3 ambient = baseColor * 0.2;
	vec3 lighting = ambient + baseColor * diff;

	outFragColor = vec4(lighting, 1.0);

#if DEBUG
	uint mhash = hash(drawId);
	outFragColor = vec4(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255), 255) / 255.0;
#endif
}
