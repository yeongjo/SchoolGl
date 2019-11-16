#version 330

in vec3 vertexColor;

out vec4 color;

uniform vec3 vcolor;

void main(){
	color = vec4(vcolor * vertexColor,1.0);
	//color = vec4(1,0,1,1.0);
}