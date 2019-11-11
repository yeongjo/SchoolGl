#version 420

layout(location=0) in vec3 pos;
//layout(location=1) in vec3 vcolor;

uniform mat4 trans;

layout (std140, binding = 0) uniform Camera{
	mat4 p; // 16 * 4 = 64
	mat4 v; // 128
	mat4 vp; // 192
};

//out vec3 fcolor;
out vec3 vcolor;

void main(){
	gl_Position = vp*trans*vec4(pos,1);
	vcolor = abs(pos);
}