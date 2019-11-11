#include <iostream>
#include <sstream>
#include <glew.h>
#include <freeglut.h>
#include <ext.hpp>
#include <gtc/matrix_transform.hpp>
#include <vector>
#include <algorithm>

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "tiny_obj_loader.h"

using namespace std;
using namespace glm;

#define print(fmt, ...) printf("[%s:%d:%s]" fmt "\n",__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__);
#define debug(fmt, ...) printf("Debug: " fmt "\n", ##__VA_ARGS__);
#define De2Ra(x) x*0.017453

float rc() {
	return rand() % 255 / 255.f;
}

char* readTxt(const char* file)
{
	FILE* fptr;
	long length;
	char* buf;
	fopen_s(&fptr, file, "rb"); // Open file for reading
	if (!fptr) // Return NULL on failure
		return NULL;
	fseek(fptr, 0, SEEK_END); // Seek to the end of the file
	length = ftell(fptr); // Find out how many bytes into the file we are
	buf = (char*)malloc(length + 1); // Allocate a buffer for the entire length of the file and a null terminator
	fseek(fptr, 0, SEEK_SET); // Go back to the beginning of the file
	fread(buf, length, 1, fptr); // Read the contents of the file in to the buffer
	fclose(fptr); // Close the file
	buf[length] = 0; // Null terminator
	return buf; // Return the buffer
}

class VO;

VO* readObj(const char* file_path);


GLchar glErrorLog[512];
GLuint compliePartShader(char* ch, GLuint type, const char* errorName) {
	GLuint vs = glCreateShader(type);
	glShaderSource(vs, 1, &ch, NULL);
	glCompileShader(vs);

	GLint result;

	glGetShaderiv(vs, GL_COMPILE_STATUS, &result);

	if (!result)
	{
		glGetShaderInfoLog(vs, 512, NULL, glErrorLog);
		std::cerr << errorName << glErrorLog << std::endl;
		return -1;
	}
	return vs;
}

GLuint complieShader(const char* shader) {
	std::stringstream s_vs;
	s_vs << "shader/" << shader;
	std::stringstream s_fs;
	s_fs << s_vs.str() << ".fs";
	s_vs << ".vs";
	char* t_vs = readTxt(s_vs.str().c_str());
	char* t_fs = readTxt(s_fs.str().c_str());

	std::stringstream ss;
	ss << "ERROR: " << shader << ": vertex shader 컴파일 실패 :";
	GLuint vs = compliePartShader(t_vs, GL_VERTEX_SHADER, ss.str().c_str());
	ss.clear(); ss.str("");
	ss << "ERROR: " << shader << ": fragment shader 컴파일 실패 :";
	GLuint fs = compliePartShader(t_fs, GL_FRAGMENT_SHADER, ss.str().c_str());

	GLuint ShaderProgramID = glCreateProgram(); //--- 세이더 프로그램 만들기
	glAttachShader(ShaderProgramID, vs); // 세이더 프로그램에 버텍스 세이더 붙이기
	glAttachShader(ShaderProgramID, fs); // 세이더 프로그램에 프래그먼트 세이더 붙이기
	glLinkProgram(ShaderProgramID); // 세이더 프로그램 링크하기
	glDeleteShader(vs); // 세이더 프로그램에 링크하여 세이더 객체 자체는 삭제 가능
	glDeleteShader(fs);
	GLint result;
	glGetProgramiv(ShaderProgramID, GL_LINK_STATUS, &result); // 세이더가 잘 연결되었는지 체크하기
	if (!result) {
		glGetProgramInfoLog(ShaderProgramID, 512, NULL, glErrorLog);
		std::cerr << "ERROR: " << shader << "shader program 연결 실패\n" << glErrorLog << std::endl;
		return false;
	}
	// 여러 개의 프로그램 만들 수 있고, 특정 프로그램을 사용하려면
	// glUseProgram 함수를 호출하여 사용 할 특정 프로그램을 지정한다.
	// 사용하기 직전에 호출할 수 있다.

	free(t_vs);
	free(t_fs);

	return ShaderProgramID;
}

// 사용전에 bind를 반드시 호출해줘야한다.
class VO {
public:
	GLuint VAO, VBO, elementbuffer;
	vector<vec3> vertex, color, normal;
	vector<vec2> uv;
	vector<uint> vertexIndices;

	GLshort drawStyle = GL_TRIANGLES;

	int verIdx = -1;

	bool isBind = false;

	//GLbyte mode = 0;
	//static const GLbyte color	= 0b0000'0001;//1
	//static const GLbyte normal	= 0b0000'0010;//2
	//static const GLbyte uv		= 0b0000'0100;//3
	
	static VO* loadObj(const char *path) {
		return readObj(path);
	}

	void bind() {
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		size_t bufferOff = 0;
		size_t totalBufferSize = (vertex.size()+ color.size()+ normal.size())*3 + uv.size()*2;
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, totalBufferSize * sizeof(float), 0, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertex.size() * 3 * sizeof(float), &vertex[0]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		bufferOff += vertex.size()*3 * sizeof(float);
		if (color.size() > 0) {
			glBufferSubData(GL_ARRAY_BUFFER, bufferOff, color.size() * 3 * sizeof(float), &color[0]);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(bufferOff));
			glEnableVertexAttribArray(1);
			bufferOff += color.size() * 3 * sizeof(float);
		}
		if (normal.size() > 0) {
			glBufferSubData(GL_ARRAY_BUFFER, bufferOff * sizeof(float), normal.size() * 3 * sizeof(float), &normal[0]);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(bufferOff));
			glEnableVertexAttribArray(2);
			bufferOff += normal.size() * 3 * sizeof(float);
		}
		if (uv.size() > 0) {
			glBufferSubData(GL_ARRAY_BUFFER, bufferOff * sizeof(float), uv.size() * 2 * sizeof(float), &uv[0]);
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(bufferOff));
			glEnableVertexAttribArray(3);
			bufferOff += uv.size() * 3 * sizeof(float);
		}

		isBind = true;
	}

	void render() {
		assert(isBind && "not binded VO used.");
		glBindVertexArray(VAO);
		if (verIdx == -1)
			verIdx = vertexIndices.size();

		if (verIdx >= 3)
			;// glDrawElements(GL_TRIANGLES, 0, vertex.size());
		else
			glDrawArrays(drawStyle, 0, vertex.size());
	}
};

glm::vec3 operator+ (glm::vec2& a, glm::vec3& b) {
	return glm::vec3(a.x + b.x, a.y + b.y, b.z);
}

glm::vec3 operator+ (glm::vec3& b, glm::vec2& a) {
	return glm::vec3(a.x + b.x, a.y + b.y, b.z);
}

void vec3ToFloat(glm::vec3& a, float* b) {
	for (int i = 0; i < 3; i++)
		b[i] = a[i];
}

VO* readObj(const char* file_path) {
	VO* vo = new VO();

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_path);
	if (!ret) {
		print("can't load %s", file_path);
	}

	uint shape_size = shapes.size();
	for (size_t s = 0; s < shape_size; s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;

		size_t vert_size = shapes[s].mesh.num_face_vertices.size();
		for (size_t f = 0; f < vert_size; f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			//memcpy((void*)& vo->vertex[0], (void*)& attrib.vertices[0], attrib.vertices.size() * sizeof(float));

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
				tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
				tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
				// Optional: vertex colors
				tinyobj::real_t red = attrib.colors[3 * idx.vertex_index + 0];
				tinyobj::real_t green = attrib.colors[3 * idx.vertex_index + 1];
				tinyobj::real_t blue = attrib.colors[3 * idx.vertex_index + 2];

				vo->vertex.push_back(vec3(vx, vy, vz));
				vo->normal.push_back(vec3(nx, ny, nz));
				vo->uv.push_back(vec2(tx, ty));
				vo->color.push_back(vec3(rc(), rc(), rc()));
			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}
	}

	return vo;
}