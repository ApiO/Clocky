#version 330

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec4 vertex_color;

uniform mat4 proj_view;
out vec4 pixel_color;

void main(){
	pixel_color = vertex_color;
	gl_Position = proj_view * vec4 (vertex_position, 1.0);
}