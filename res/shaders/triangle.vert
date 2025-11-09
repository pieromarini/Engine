#version 450
#extension GL_EXT_buffer_reference : require

struct Vertex {
	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
};

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec3 outNormal;

layout(buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};

layout(push_constant, std430) uniform pc {
	mat4 mvp;
	VertexBuffer vertexBuffer;
} PushConstants;

void main() {
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

	outColor = v.color.rgb;
	outNormal = v.normal;

	gl_Position = PushConstants.mvp * vec4(v.position, 1.0f);
}
