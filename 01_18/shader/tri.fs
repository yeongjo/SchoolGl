#version 330

out vec4 color;

uniform vec3 vcolor;

void main(){
	color = vec4(vcolor,1.0);
	//color = vec4(1,0,1,1.0);
}