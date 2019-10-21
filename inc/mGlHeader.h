#include <iostream>
#include <sstream>
#include <../gl/glew.h>
#include <../gl/freeglut.h>
#include <../glm/ext.hpp>
#include <../glm/gtc/matrix_transform.hpp>
#include <vector>

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
	s_vs << "../shader/" << shader;
	std::stringstream s_fs;
	s_fs << s_vs.str() << ".fs";
	s_vs << ".vs";
	char* t_vs = readTxt(s_vs.str().c_str());
	char* t_fs = readTxt(s_fs.str().c_str());

	std::stringstream ss;
	ss << "ERROR: " << shader << "vertex shader 컴파일 실패 :";
	GLuint vs = compliePartShader(t_vs, GL_VERTEX_SHADER, ss.str().c_str());
	ss.clear(); ss.str("");
	ss << "ERROR: " << shader << "fragment shader 컴파일 실패 :";
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

class VO {
public:
	GLuint VAO, VBO;

	GLbyte mode = 0;
	static const GLbyte color = 0b0000'0001;
	static const GLbyte normal = 0b0000'0010;
	// 커스텀 배열 만들어서 넘기는 법도 있을듯
	// 묶음 갯수 같은거
	GLuint create(float* vertex, GLuint count, GLbyte _mode = color) {
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		change(vertex, count, _mode);
		return VAO;
	}

	void change(float* vertex, GLuint count, GLbyte _mode = color) {
		int i = 3, ii;
		mode = _mode;
		GLbyte t = 1;
		for (ii = 0; ii < 4; ii++) {
			if (mode & t)
				i += 3;
			t = t << 1;
		}
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, count * sizeof(float), vertex, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, i * sizeof(float), 0);
		glEnableVertexAttribArray(0);
		if (mode & color) {
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, i * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);
		}
	}

};

glm::vec3 operator+ (glm::vec2& a, glm::vec3& b) {
	return glm::vec3(a.x + b.x, a.y + b.y, b.z);
}

glm::vec3 operator+ (glm::vec3& b, glm::vec2& a) {
	return glm::vec3(a.x + b.x, a.y + b.y, b.z);
}


float* vec3ArrToFloatArr(std::vector<glm::vec3>& vertexArr) {
	auto arrSize = vertexArr.size() * 3;
	float* t = new float[arrSize];
	size_t j= 0;
	for (size_t i = 0; i < arrSize; i+=3, ++j)
	{
		auto tv = vertexArr[j];
		t[0+i] = tv.x;
		t[1+i] = tv.y;
		t[2 + i] = tv.z;
	}
	return t;
}

void vec3ToFloat(glm::vec3& a, float* b) {
	for (int i = 0; i < 3; i++)
		b[i] = a[i];
}