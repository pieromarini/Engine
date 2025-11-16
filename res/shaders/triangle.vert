#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_shader_draw_parameters: require

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
	int albedoTexture;
	int normalTexture;
	int specularTexture;
	int emissiveTexture;

	vec4 diffuseFactor;
	vec4 specularFactor;
	vec4 emissiveFactor;
};

struct MeshDraw {
	uint materialIndex;
	float padding[3];
};

struct InstanceDraw {
	mat4 transform;
};

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out flat uint outDrawId;
layout (location = 4) out vec3 outFragPos;

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

layout (set = 0, binding = 1) readonly buffer DrawCommands {
	IndirectCommandData drawCommands[];
};

layout (std430, set = 0, binding = 2) readonly buffer Draws {
	MeshDraw draws[];
};

layout (std430, set = 0, binding = 3) readonly buffer InstanceDraws {
	InstanceDraw instanceDraws[];
};

void main() {
	uint drawId = drawCommands[gl_DrawIDARB].drawId;
	MeshDraw meshDraw = draws[drawId];
	InstanceDraw instanceDraw = instanceDraws[gl_InstanceIndex];

	mat4 transform = instanceDraw.transform;
	uint materialIndex = meshDraw.materialIndex;

	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

	vec4 position = vec4(v.position, 1.0f);

	gl_Position = PushConstants.viewProj * transform * position;

	outNormal = (transform * vec4(v.normal, 0.f)).xyz;
	outColor = v.color.xyz * materialData[materialIndex].diffuseFactor.xyz;
	outUV.x = v.tu;
	outUV.y = v.tv;
	outDrawId = drawId;
	outFragPos = v.position;
}
