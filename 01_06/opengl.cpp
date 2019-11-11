﻿#include "inc/mGlHeader2.h"

using namespace std;
using namespace glm;

glm::vec3 t_vertex[] = {
	glm::vec3(-0.1, 0, 0.0),
	glm::vec3(0, 0.1, 0.0),
	glm::vec3(0.1, 0.0, 0),
};

glm::vec3 t_vertex_color[] = {
	glm::vec3(1, 0, 0.0),
	glm::vec3(0, 1, 0.0),
	glm::vec3(1, 1, 0),
};


glm::vec2 t_vertex_moveDirec[] = {
	glm::vec2(0, -1),
	glm::vec2(1, 0),
	glm::vec2(0, 1),
	glm::vec2(-1, 0) };

class Obj {
public:
	std::vector<glm::vec3> vertexList;
	std::vector<glm::vec3> colorList;

	VO vo;

	glm::vec3 pos;
	glm::vec3 moveVec;
	float rotz;
	float speed = 0.03;

	glm::mat4 trans = glm::mat4(1.0f);

	bool bIsGetTransInThisTick = false;

	float* getVertexArr() {
		return (float*)(&vertexList[0]);
	}

	void init() {
		vo.create(vertexList, 3*3, 0);

		moveVec.x = (0.5f - rand()%200/200.f)* speed;
		moveVec.y = (0.5f - rand() % 200 / 200.f) * speed;
	}

	void destroy() {
	}

	void setPos(const vec3& p) {
		pos = p;
	}

	void rotate(float val) {
		rotz += val;
	}

	glm::mat4& getTrans() {
		if (bIsGetTransInThisTick) return trans;
		trans = glm::mat4(1.0f);
		trans = glm::translate(trans, pos);
		trans = glm::rotate(trans, glm::radians(rotz), glm::vec3(0.0, 0.0, 1.0));
		bIsGetTransInThisTick = true;
		return trans;
	}

	virtual void tick(float dt) {
		bIsGetTransInThisTick = false;
		pos += moveVec;

		if (pos.x < -0.9f || pos.x > 0.9f) { rotate(90); moveVec.x = -moveVec.x; }
		if (pos.y < -0.9f || pos.y > 0.9f) { rotate(90); moveVec.y = -moveVec.y; }
	}

	virtual void render() {
		glBindVertexArray(vo.VAO); ;
		glDrawArrays(GL_TRIANGLES, 0, vertexList.size());
	}
};

GLuint tri_shader;
Obj objs[10];


bool isTimerEnd = false;

void init() {
	for (size_t i = 0; i < 10; i++) {
		objs[i].vertexList.push_back(t_vertex[0]);
		objs[i].vertexList.push_back(t_vertex[1]);
		objs[i].vertexList.push_back(t_vertex[2]);
		//objs[i].vertexList.push_back(t_vertex_color[0]);
		//objs[i].vertexList.push_back(t_vertex_color[1]);
		//objs[i].vertexList.push_back(t_vertex_color[2]);
		
		objs[i].init();
	}
}
float checkTime = 0;
float direction = 1;
bool stop = false;

DWORD prevTime = 0;
DWORD thisTickTime = 0;
float dt;

void loop() {
	if (stop) return;


	if (prevTime == 0) {
		prevTime = GetTickCount() - 10;
	}
	thisTickTime = GetTickCount();
	dt = (thisTickTime - prevTime) * 0.001f;
	dt = dt > 0.05f ? dt = 0.05f : dt;
	prevTime = thisTickTime;


	for (size_t i = 0; i < 10; i++) {

		objs[i].tick(dt);
	}
	glutPostRedisplay();
}

void timerFunc(int v) {
	//r = f(), g = f(), b = f();
	loop();
	glutPostRedisplay();
	if (!isTimerEnd)
		glutTimerFunc(10, timerFunc, 0);
}

GLvoid drawScene(GLvoid);
GLvoid Reshape(int w, int h);
void Keyboard(unsigned char key, int x, int y);
void Mouse(int button, int state, int x, int y);

void main(int argc, char** argv) // 윈도우 출력하고 콜백함수 설정 
{ //--- 윈도우 생성하기
	glutInit(&argc, argv); // glut 초기화
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA); // 디스플레이 모드 설정
	glutInitWindowPosition(1920, 0); // 윈도우의 위치 지정
	glutInitWindowSize(800, 600); // 윈도우의 크기 지정
	glutCreateWindow("Example1"); // 윈도우 생성(윈도우 이름)

		//--- GLEW 초기화하기
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "Unable to initialize GLEW" << std::endl;

		exit(EXIT_FAILURE);
	}
	else
		std::cout << "GLEW Initialized\n";

	init();
	tri_shader = complieShader("tri");

	glutDisplayFunc(drawScene); // 출력 함수의 지정
	glutReshapeFunc(Reshape); // 다시 그리기 함수 지정

	glutKeyboardFunc(Keyboard);
	glutMouseFunc(Mouse);

	timerFunc(1);

	glutMainLoop(); // 이벤트 처리 시작 
}

GLclampf r=0.6f, g=1, b=1;
GLclampf x = 0, y = 0, z = 1;

GLfloat x1 = -0.5f, g_y1 = -0.5f, x2 = 0.5f, y2 = 0.5f;


GLvoid drawScene() // 콜백 함수: 출력
{
	//printf("drawScene\n");
	glClearColor(r, g, b, 1.0f); // 바탕색을 ‘blue’로 지정
	glClear(GL_COLOR_BUFFER_BIT); // 설정된 색으로 전체를 칠하기
	
	for (size_t i = 0; i < 10; i++) {

		glUseProgram(tri_shader);
		unsigned int transformLoc = glGetUniformLocation(tri_shader, "trans");
		glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(objs[i].getTrans()));
		objs[i].render();
	}

	glutSwapBuffers(); // 화면에 출력하기
}
GLvoid Reshape(int w, int h) {
	printf("Reshape\n");
	glViewport(0, 0, w, h);
}


GLclampf f() {
	return rand() % 255 / 255.0f;
}


void Keyboard(unsigned char key, int x, int y)
{
	printf("Keyboard\n");
	switch (key) {

	case 'q': isTimerEnd = true;  glutLeaveMainLoop(); break;
	}
	glutPostRedisplay();
}

int nextIdx = 0;
void Mouse(int button, int state, int _x, int _y) {
	//printf("Mouse\n");
	if (state == 1) return;
	printf("%d\n", state);

	float t_x = _x / (800.0f / 2) - 1;
	float t_y = -_y / (600.0f / 2) + 1;
	printf("%d %d %02f %02f\n", _x, _y, t_x, t_y);
	/*if ((t_x < x1 || x2 < t_x) || (t_y > y2 || t_y < g_y1))
		r = f(), g = f(), b = f();
	else
		x = f(), y = f(), z = f();*/
	
	
	//glEnableVertexAttribArray(0);

	
	glutPostRedisplay();
	switch (state) {
	case 0:

		break;
	}
}