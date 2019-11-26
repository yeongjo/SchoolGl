#version 420

out vec4 color;

in vec3 normal;
in vec3 fragPos;;

layout (std140, binding = 0) uniform Camera{
	mat4 p; // 0 : 16 * 4 = 64
	mat4 v; // 64 : 128
	mat4 vp; // 128 : 192
	vec3 viewPos; // 192 : (16*3=48)+192= 240
};

uniform vec3 ambient;
uniform vec3 lightPos;
uniform vec3 lightColor;

uniform vec3 vcolor;

void main(){
	vec3 lightDir = normalize(lightPos - fragPos);
	vec3 diffuse = max(dot(normal, lightDir), 0) * lightColor;

	vec3 result = (diffuse+ambient) * vcolor;
	color = vec4(result, 1.0);
}