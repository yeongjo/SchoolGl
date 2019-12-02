#version 420

out vec4 color;

in vec3 normal;
in vec3 fragPos;
in vec2 uv;

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

uniform sampler2d texture0;

void main(){
	//color = vec4(normal, 1.0);
	//return;
	vec3 lightDir = normalize(lightPos - fragPos);
	vec3 diffuse = max(dot(normal, lightDir), 0) * lightColor;

	vec3 viewDir = normalize(viewPos - fragPos);
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = 1 * spec * lightColor;  

	vec3 result = (diffuse+ambient+specular) * vcolor * texture(texture0, uv).rgb;
	color = vec4(result, 1.0);
}