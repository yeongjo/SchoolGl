#include "ToolModule.h"
#include <stack>
#include "../glm/gtx/spline.hpp"

using namespace std;
using namespace glm;

int render_line_idx = 0;




class CraneObj : public Obj {
public:

};


Window win;
Camera cam;

VO gridVO;

Obj *craneObj[3];

Shader triShader, gridShader;


bool isTimerEnd = false;

void init() {
	glLineWidth(3);

	triShader.complieShader("tri");
	gridShader.complieShader("grid");

	cam.init();

	for (size_t i = 0; i < 3; i++) {
		craneObj[i] = new Obj();
		craneObj[i]->loadObj("../model/");
		craneObj[i]->setShader(&triShader);
	}
	
	cam.armLength = 20;
	cam.rotateArmY(120);

	gridVO.drawStyle = GL_LINES;
	gridVO.vertex.push_back(vec3(0, 100, 0));
	gridVO.vertex.push_back(vec3(0, -100, 0));
	gridVO.vertex.push_back(vec3(100, 0, 0));
	gridVO.vertex.push_back(vec3(-100, 0, 0));
	gridVO.bind();
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
		prevTime = GetTickCount64() - 10;
	}
	thisTickTime = GetTickCount64();
	dt = (thisTickTime - prevTime) * 0.001f;
	dt = dt > 0.05f ? dt = 0.05f : dt;
	prevTime = thisTickTime;

	cam.tick(dt);
	Scene::activeScene->tick(dt);
	
	glutPostRedisplay();
}

GLvoid drawScene() // 콜백 함수: 출력
{
	//printf("drawScene\n");
	glClearColor(0, 0, 0, 1.0f); // 바탕색을 ‘blue’로 지정
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 설정된 색으로 전체를 칠하기

	cam.bind(win);
	Scene::activeScene->render();

	gridShader.use();
	gridVO.render();

	glutSwapBuffers(); // 화면에 출력하기
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
void mouseHandler(int button, int state, int x, int y);
void specialInput(int key, int x, int y);
void keyboardUp(unsigned char key, int x, int y);
void mouseMotionHandler(int x, int y);

void main(int argc, char** argv) // 윈도우 출력하고 콜백함수 설정 
{ //--- 윈도우 생성하기
	glutInit(&argc, argv); // glut 초기화
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // 디스플레이 모드 설정
	win.init(800, 600);
	glutInitWindowPosition(0, 60); // 윈도우의 위치 지정
	glutInitWindowSize(win.w, win.h); // 윈도우의 크기 지정
	glutCreateWindow(__FILE__); // 윈도우 생성(윈도우 이름)

	glEnable(GL_DEPTH_TEST);
		//--- GLEW 초기화하기
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "Unable to initialize GLEW" << std::endl;

		exit(EXIT_FAILURE);
	}
	else
		std::cout << "GLEW Initialized\n";

	init();

	glutDisplayFunc(drawScene); // 출력 함수의 지정
	glutReshapeFunc(Reshape); // 다시 그리기 함수 지정

	glutKeyboardFunc(Keyboard);
	glutKeyboardUpFunc(keyboardUp);
	glutMouseFunc(mouseHandler);
	glutMotionFunc(mouseMotionHandler);
	glutSpecialFunc(specialInput);

	timerFunc(1);

	glutMainLoop(); // 이벤트 처리 시작 
}

int renderIdx = 0;


GLvoid Reshape(int w, int h) {
	printf("Reshape\n");
	glViewport(0, 0, w, h);
}

float cam_y = 20;
float rot_speed = 60;
void Keyboard(unsigned char key, int x, int y)
{
	debug("Keyboard down: %c [x:%d, y:%d]", key, x, y);
	
	switch (key) {
	case 'q': isTimerEnd = true;  glutLeaveMainLoop(); break;
	}
}

void keyboardUp(unsigned char key, int x, int y) {
	debug("Keyboard up: %c [x:%d, y:%d]", key, x, y);

}

void specialInput(int key, int x, int y) {
	debug("specialInput: %c [x:%d, y:%d]", key, x, y);
	/*switch (key) {
	case GLUT_KEY_UP:
		obj.pos.y += 0.1f;
		break;
	case GLUT_KEY_DOWN:
		obj.pos.y -= 0.1f;

		break;
	case GLUT_KEY_LEFT:
		obj.pos.x -= 0.1f;

		break;
	case GLUT_KEY_RIGHT:
		obj.pos.x += 0.1f;

		break;
	}*/
}

// button: 0(left), 1(mid), 2(right), 3(scrollup), 4(scrolldown)
// state: 0(up), 1(down)
void mouseHandler(int button, int state, int _x, int _y) {
	float screenMouseX = _x / (Window::get().halfWidth / 2) - 1;
	float screenMouseY = -_y / (Window::get().halfHeight / 2) + 1;
	debug("Mouse: button(%d), state(%d), window(%d,%d), screen(%02f, %02f)", button, state, _x, _y, screenMouseX, screenMouseY);

	switch (state) {
	//case 0: // Mouse Down
	//	cuttingObj->controller->startDrag(vec3(screenMouseX, screenMouseY, 0));
	//	break;
	//case 1: // Mouse Up
	//	cuttingObj->controller->endDrag(vec3(screenMouseX, screenMouseY, 0));
	//	break;
	}
}

void mouseMotionHandler(int x, int y) {
	float screenMouseX = x / (Window::get().halfWidth / 2) - 1;
	float screenMouseY = -y / (Window::get().halfHeight / 2) + 1;
	//cuttingObj->controller->setMovingMousePos(vec3(screenMouseX, screenMouseY, 0));
}