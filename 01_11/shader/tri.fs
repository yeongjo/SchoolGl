#version 330

in vec3 fcolor;
in vec3 ovcolor;

out vec4 color;

uniform vec3 vcolor;

void main(){
	color = vec4(fcolor,1.0);
	//color = vec4(1,0,1,1.0);
}