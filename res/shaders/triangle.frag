#version 450
#extension GL_EXT_buffer_reference : require

struct Vertex {
	vec3 position;
	float tu;
	vec3 normal;
	float tv;
	vec4 color;
};

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoords;

layout (location = 0) out vec4 outFragColor;

layout(buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};

layout(push_constant, std430) uniform pc {
	mat4 mvp;
	VertexBuffer vertexBuffer;
} PushConstants;

void main() {
	vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
	vec3 N = normalize(inNormal);

	float diff = max(dot(N, lightDir), 0.0);

	vec3 baseColor = vec3(0.5, 0.5, 0.5);

	vec3 ambient = baseColor * 0.2;
	vec3 lighting = ambient + baseColor * diff;

	outFragColor = vec4(lighting, 1.0);
}
