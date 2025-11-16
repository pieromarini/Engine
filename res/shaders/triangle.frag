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

	vec4 diffuseFactor;
	vec4 specularFactor;
	vec4 emissiveFactor;
};

struct MeshDraw {
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

layout (set = 0, binding = 0) readonly buffer GLTFMaterialData {
	MaterialData materialData[];
};

layout (std430, set = 0, binding = 2) readonly buffer Draws {
	MeshDraw draws[];
};

layout (set = 1, binding = 0) uniform sampler2D textures[];

void main() {
	MeshDraw meshDraw = draws[drawId];
	MaterialData material = materialData[meshDraw.materialIndex];

	vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
	vec3 N = normalize(inNormal);

	vec3 color = vec3(1.0f);
	if (material.albedoTexture >= 0) {
		color *= texture(textures[nonuniformEXT(material.albedoTexture)], inUV).xyz;
	}

	float diff = max(dot(N, lightDir), 0.0);

	vec3 ambient = color * 0.2;
	vec3 diffuse = diff * color;

	outFragColor = vec4(ambient + diffuse, 1.0);

#if DEBUG
	uint mhash = hash(drawId);
	outFragColor = vec4(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255), 255) / 255.0;
#endif
}
