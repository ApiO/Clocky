#version 330
#extension GL_ARB_draw_instanced : enable

in vec2 texture_coordinates;

uniform sampler2D diffuse;
uniform sampler2D colors;

void main(){
	gl_FragColor = texture(diffuse, texture_coordinates); // * pixel_color;
	gl_FragColor = vec4(gl_FragColor.rgb*gl_FragColor.a, gl_FragColor.a);
}