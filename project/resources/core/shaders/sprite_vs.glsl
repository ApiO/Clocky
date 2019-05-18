#version 330

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec2 tex_coord;
layout (location = 2) in vec4 vertex_color;

uniform mat4 proj_view;
out vec2 texture_coordinates;
out vec4 pixel_color;

void main(){
	texture_coordinates = tex_coord;
	pixel_color = vertex_color;
	gl_Position = proj_view * vec4 (vertex_position, 1.0);
}