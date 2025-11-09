#version 450
#extension GL_EXT_buffer_reference : require

struct Vertex {
	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
};

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec3 inNormal;

layout (location = 0) out vec4 outFragColor;

layout(buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};

layout(push_constant, std430) uniform pc {
	mat4 mvp;
	VertexBuffer vertexBuffer;
} PushConstants;

void main() {
	outFragColor = vec4(inColor, 1.0f);
}
