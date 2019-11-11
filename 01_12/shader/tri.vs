#version 330

layout(location=0) in vec3 pos;
layout(location=1) in vec3 vcolor;

uniform mat4 trans;
uniform mat4 vp;

out vec3 fcolor;
out vec3 ovcolor;

void main(){
	gl_Position = vp*trans*vec4(pos,1);
	fcolor = vcolor;//(gl_Position.xyz + vec3(1)) / 2;
}