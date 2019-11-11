#version 420

in vec3 vcolor;


out vec4 color;


void main(){
	color = vec4(vcolor,1.0);
	//color = vec4(1,0,0,1);
}