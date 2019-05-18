#version 330
#extension GL_ARB_draw_instanced : enable

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec2 tex_coord;

uniform mat4      proj_view;
uniform sampler2D positions;

out vec2 texture_coordinates;

void main(){
	int y = (gl_InstanceID * 4) / 1024;

	mat4 m = mat4(texture2D(positions,vec2((gl_InstanceID*4+0)&1023,y) * (1.0/1024.0)),
	              texture2D(positions,vec2((gl_InstanceID*4+1)&1023,y) * (1.0/1024.0)),
	              texture2D(positions,vec2((gl_InstanceID*4+2)&1023,y) * (1.0/1024.0)),
	              texture2D(positions,vec2((gl_InstanceID*4+3)&1023,y) * (1.0/1024.0)));

	texture_coordinates = tex_coord;
	gl_Position = proj_view * m * vec4(vertex_position, 1.0);
}