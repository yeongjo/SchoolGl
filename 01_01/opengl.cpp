#include <iostream>
#include <glew.h>
#include <freeglut.h>
#include <ext.hpp>
#include <gtc/matrix_transform.hpp>

GLvoid drawScene(GLvoid);
GLvoid Reshape(int w, int h);
void Keyboard(unsigned char key, int x, int y);

void main(int argc, char** argv) // 윈도우 출력하고 콜백함수 설정 
{ //--- 윈도우 생성하기
	glutInit(&argc, argv); // glut 초기화
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA); // 디스플레이 모드 설정
	glutInitWindowPosition(0, 0); // 윈도우의 위치 지정
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

	glutDisplayFunc(drawScene); // 출력 함수의 지정
	glutReshapeFunc(Reshape); // 다시 그리기 함수 지정

	glutKeyboardFunc(Keyboard);

	glutMainLoop(); // 이벤트 처리 시작 
}

GLclampf r=1, g=1, b=1;

GLvoid drawScene() // 콜백 함수: 출력
{
	glClearColor(r, g, b, 1.0f); // 바탕색을 ‘blue’로 지정
	glClear(GL_COLOR_BUFFER_BIT); // 설정된 색으로 전체를 칠하기
	


	glutSwapBuffers(); // 화면에 출력하기
}
GLvoid Reshape(int w, int h) {
	glViewport(0, 0, w, h);
}
bool isTimerEnd = false;

GLclampf f() {
	return rand() % 255 / 255.0f;
}
void timerFunc(int v) {
	r = f(), g = f(), b = f();
	glutPostRedisplay();
	if(!isTimerEnd)
		glutTimerFunc(100, timerFunc, 0);
}


void Keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case 'r': r=1,g=0,b=0; break;
	case 'g':r = 0, g = 1, b = 0; break;
	case 'b':r = 0, g = 0, b = 1; break;
	case 'a':r = f(), g = f(), b = f(); break;
	case 'w':r = 1, g = 1, b = 1; break;
	case 'k':r = 0, g = 0, b = 0; break;
	case 't':isTimerEnd = false; glutTimerFunc(100, timerFunc, 0); break;
	case 's':isTimerEnd = true; break;
	case 'q': glutLeaveMainLoop(); break;
	}
	glutPostRedisplay();
}