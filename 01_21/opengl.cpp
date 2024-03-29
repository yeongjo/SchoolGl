﻿#include "ToolModule.h"

class Light : public Obj {
public:
	float rotSpeed = 2;
	bool isRotating = true;
	float length = 5;
	float delta = 0;

	Light() : Obj() {
		loadObj("../model/sphere.obj");
		scale = vec3(0.2f);
	}

	void tick(float dt) {
		if (!isRotating) return;
		delta += rotSpeed * dt;
		float x = cos(delta) * length;
		float z = sin(delta) * length;
		pos.x = x;
		pos.y = z;
		updateTransform();
	}

	void setLightToShader(Shader& shader) {
		shader.addUniform("ambient", new vec3(0.1f, 0.1f, 0.1f));
		shader.addUniform("lightPos", &getPos());
		shader.addUniform("lightColor", &color);
	}
};


class JohnSnow : public Obj {
public:
	float speed = -2;

	virtual void tick(float dt) {
		updateTransform();
		pos.y += dt * speed;
		if (pos.y < 0) {
			pos.y = 2;
		}
	}
};


class ShapeObj : public Obj {
public:
	float speed = 6;
	virtual void tick(float dt) {
		updateTransform();
		rot.y += dt * speed;
	}
};

class Orbit : public Obj {
public:
	Obj* parentObj = nullptr;
	vector<Obj*> childObjs;

	VO* orbitVO = nullptr;

	mat4 orbitTrans;

	float speed = 2;
	float arm = 3;

	void initOrbit() {
		orbitVO = new VO();

		for (size_t i = 0; i < 60; i++) {
			float _x = cos(De2Ra(i * 6)) * arm;
			float _y = sin(De2Ra(i * 6)) * arm;
			orbitVO->vertex.push_back(vec3(_x, 0, _y));
		}
		orbitVO->bind();
		orbitVO->drawStyle = GL_LINE_LOOP;
	}

	virtual void tick(float dt) {
		pos.x = arm;
		rot.y += speed;
		trans = parentObj->getTrans();
		trans = glm::rotate(trans, glm::radians(rot.x), glm::vec3(1, 0, 0));
		trans = glm::rotate(trans, glm::radians(rot.y), glm::vec3(0, 1, 0));
		trans = glm::translate(trans, pos);
		trans = glm::scale(trans, scale);

		if (parentObj) {
			orbitTrans = parentObj->getTrans();
			orbitTrans = glm::rotate(orbitTrans, glm::radians(rot.x), glm::vec3(1, 0, 0));
		}
	}

	void render() {
		if (orbitVO) {
			shader->changeUniformValue("trans", &orbitTrans);
			shader->use();
			orbitVO->render();
		}
		Obj::render();
	}
};


Window win;
Camera cam;
Shader triShader, gridShader;

ShapeObj* triObj;
Obj* floorObj;

Light* light;
vector<Orbit*> orbits;
vector<JohnSnow*> johnSnow;

VO gridVO;

bool isTimerEnd = false;


void init() {
	cam.init();
	cam.pos.y = 4;

	triShader.complieShader("tri");
	gridShader.complieShader("grid");

	triObj = new ShapeObj();
	floorObj = new Obj();

	floorObj->loadObj("../model/cube.obj");
	triObj->loadObj("../model/tri.obj");

	floorObj->setShader(&triShader);
	triObj->setShader(&triShader);

	floorObj->setScale(vec3(5, 0.1f, 5));
	floorObj->getPos().x = -1;

	orbits.push_back(new Orbit());
	orbits.push_back(new Orbit());
	orbits.push_back(new Orbit());
	int armLen = 3;
	int speed = 1;
	for (auto var : orbits) {
		armLen += 2;
		speed++;
		var->arm = armLen;
		var->speed = speed;
		var->setShader(&triShader);
		var->parentObj = triObj;
		var->loadObj("../model/sphere.obj");
		var->color = vec3(f(), f(), f());
	}

	for (size_t i = 0; i < 20; i++) {
		johnSnow.push_back(new JohnSnow());
		float x = f() * 10 - 5;
		float y = f() * 1;
		float z = f() * 10 - 5;

		johnSnow.back()->loadObj("../model/cube.obj");
		string name = "snow";
		johnSnow.back()->name = name+ std::to_string(i);
		johnSnow.back()->setPos(vec3(x,y,z));
		johnSnow.back()->setScale(vec3(0.3f));
		johnSnow.back()->setShader(&triShader);
	}

	light = new Light();
	light->setLightToShader(triShader);
	light->setShader(&triShader);

	gridVO.drawStyle = GL_LINES;
	gridVO.vertex.push_back(vec3(0, 100, 0));
	gridVO.vertex.push_back(vec3(0, -100, 0));
	gridVO.vertex.push_back(vec3(100, 0, 0));
	gridVO.vertex.push_back(vec3(-100, 0, 0));
	gridVO.vertex.push_back(vec3(0, 0, 100));
	gridVO.vertex.push_back(vec3(0, 0, -100));
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
		prevTime = GetTickCount() - 10;
	}
	thisTickTime = GetTickCount();
	dt = (thisTickTime - prevTime) * 0.001f;
	dt = dt > 0.05f ? dt = 0.05f : dt;
	prevTime = thisTickTime;

	cam.tick(dt);
	Scene::activeScene->tick(dt);

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
void specialInput(int key, int x, int y);

void main(int argc, char** argv) // 윈도우 출력하고 콜백함수 설정 
{ //--- 윈도우 생성하기
	glutInit(&argc, argv); // glut 초기화
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // 디스플레이 모드 설정
	win.init(800, 600);
	glutInitWindowPosition(0, 30); // 윈도우의 위치 지정
	glutInitWindowSize(win.w, win.h); // 윈도우의 크기 지정
	glutCreateWindow("Example1"); // 윈도우 생성(윈도우 이름)

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
	glutMouseFunc(Mouse);
	glutSpecialFunc(specialInput);

	timerFunc(1);

	glutMainLoop(); // 이벤트 처리 시작 
}

int renderIdx = 0;

GLvoid drawScene() // 콜백 함수: 출력
{
	glClearColor(0, 0, 0, 1.0f); // 바탕색을 ‘blue’로 지정
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 설정된 색으로 전체를 칠하기

	cam.bind(win);
	Scene::activeScene->render();

	gridShader.use();
	gridVO.render();

	glutSwapBuffers(); // 화면에 출력하기
}


GLvoid Reshape(int w, int h) {
	printf("Reshape\n");
	glViewport(0, 0, w, h);
}

bool isCullAble = true;
bool isLineAble = false;
void Keyboard(unsigned char key, int x, int y)
{
	printf("Keyboard: %c [%d, %d]\n", key, x, y);
	switch (key) {
	case 'c':
		renderIdx = 0;
		break;
	case 'p':
		renderIdx = 1;
		break;
	case 'h':
		isCullAble = !isCullAble;
		if(isCullAble)
			glEnable(GL_CULL_FACE);
		else
			glDisable(GL_CULL_FACE);
		break;
	case 'y':
	{
		//for (size_t i = 0; i < 2; i++) {
		//	objs[i].speed = -objs[i].speed;
		//}
	}
		break;
	case 'w':
		isLineAble = !isLineAble;
		if(isLineAble)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case 'q': isTimerEnd = true;  glutLeaveMainLoop(); break;
	}
}

void specialInput(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_UP:
		//do something here
		break;
	case GLUT_KEY_DOWN:
		//do something here
		break;
	case GLUT_KEY_LEFT:
		//do something here
		break;
	case GLUT_KEY_RIGHT:
		//do something here
		break;
	}
}

int nextIdx = 0;
void Mouse(int button, int state, int _x, int _y) {
	//printf("Mouse\n");
	if (state == 1) return;
	printf("%d\n", state);

	float t_x = _x / (800.0f / 2) - 1;
	float t_y = -_y / (600.0f / 2) + 1;
	printf("%d %d %02f %02f\n", _x, _y, t_x, t_y);
	
	cam.pos.z += t_y;
	printf("z: %f\n", cam.pos.z);

	switch (state) {
	case 0:

		break;
	}
}