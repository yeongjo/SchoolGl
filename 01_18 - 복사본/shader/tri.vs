#version 420

layout(location=0) in vec3 pos;

layout (std140, binding = 0) uniform Camera{
	mat4 p; // 16 * 4 = 64
	mat4 v; // 128
	mat4 vp; // 192
};

uniform mat4 trans;

out vec3 vertexColor;

vec3 vec3Random(vec3 st) {
  st = vec3(dot(st, vec3(0.040,-0.250,-0.150)),
  dot(st, vec3(269.5,183.3,123.3)),
  dot(st, vec3(169.5,383.3,163.3)));
  return -1.0 + 2.0 * fract(sin(st) * 43758.633);
}

void main(){
	gl_Position = vp*trans*vec4(pos,1);
	vertexColor = vec3Random(vec3(gl_VertexID));
}