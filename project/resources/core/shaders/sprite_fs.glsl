#version 330

in vec2 texture_coordinates;

in vec4 pixel_color;
uniform sampler2D binded_texture;

void main(){
	gl_FragColor = texture(binded_texture, texture_coordinates) * pixel_color;
	gl_FragColor = vec4(gl_FragColor.rgb*gl_FragColor.a, gl_FragColor.a);
}