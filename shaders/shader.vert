#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject{ 
	mat4 model;
	mat4 view;
	mat4 proj; 
} ubo;

layout(location = 0) in vec2 pos;
layout(location = 1) in vec3 col;

layout(location = 0) out vec3 fragColor;

void main(){
	mat4 mvp = ubo.proj * ubo.view * ubo.model;
	gl_Position = mvp * vec4(pos.x, pos.y, -0.3, 1.0);
	//gl_Position = vec4(pos, 0.0, 1.0);
	fragColor = col;
}