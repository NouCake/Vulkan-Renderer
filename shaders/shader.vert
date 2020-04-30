#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject{ 
	mat4 model;
	mat4 view;
	mat4 proj; 
} ubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;
layout(location = 2) in vec2 texcoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 uv;

void main(){
	mat4 mvp = ubo.proj * ubo.view * ubo.model;
	gl_Position = mvp * vec4(pos, 1.0);
	//gl_Position = vec4(pos, 0.0, 1.0);
	fragColor = col;
	uv = texcoord;
}