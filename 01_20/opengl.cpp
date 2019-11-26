#include "ToolModule.h"
#include <map>
#include <set>

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
		pos.z = z;
		updateTransform();
	}
};

class OrbitRoot : public Obj {
public:
	float rotateVelocity = 0;

	virtual void tick(float dt) {
		rot.y += rotateVelocity * dt;
		updateTransform();
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

VO gridVO;

GLUquadricObj* qobj;
OrbitRoot objs[1];
Orbit orbit[6];
Light* light;



bool isTimerEnd = false;

void init() {
	glLineWidth(2);
	qobj = gluNewQuadric(); // 객체 생성하기
	gluQuadricDrawStyle(qobj, GLU_LINE); // 도형 스타일

	triShader.complieShader("tri");
	gridShader.complieShader("grid");

	//objs = new Orbit(0);
	triShader.addUniform("trans", &objs[0].getTrans());

	gridShader.addUniform("trans", mat4(1));

	cam.init();
	cam.rotateZ(10);
	cam.pos.y = 1;
	cam.pos.x = 3;

	objs[0].loadObj("../model/sphere.obj");
	objs[0].setShader(&triShader);

	orbit[0].parentObj = &objs[0];
	orbit[1].parentObj = &objs[0];
	orbit[2].parentObj = &objs[0];
	orbit[0].setRotation(vec3(-45, -120, 0));
	orbit[1].setRotation(vec3(0,0,0));
	orbit[2].setRotation(vec3(45, 120, 0));

	orbit[3].parentObj = &orbit[0];
	orbit[4].parentObj = &orbit[1];
	orbit[5].parentObj = &orbit[2];

	for (size_t i = 0; i < 6; i++) {
		orbit[i].loadObj("../model/sphere.obj");
		orbit[i].initOrbit();
		orbit[i].setShader(&triShader);
		orbit[i].setScale(vec3(0.7f));
	}

	light = new Light();
	light->setShader(&triShader);
	//light->setPos(vec3(0, 2, 0));
	light->name = "light";
	light->color = vec3(1);

	triShader.addUniform("ambient", new vec3(0.1f, 0.1f, 0.1f));
	triShader.addUniform("lightPos", &light->getPos());
	triShader.addUniform("lightColor", new vec3(1.0f, 1, 1.0f));

	triShader.logAllUniforms();


	gridVO.drawStyle = GL_LINES;
	gridVO.vertex.push_back(vec3(0, 100, 0));
	gridVO.vertex.push_back(vec3(0, -100, 0));
	gridVO.vertex.push_back(vec3(100, 0, 0));
	gridVO.vertex.push_back(vec3(-100, 0, 0));
	gridVO.vertex.push_back(vec3(0, 0, 100));
	gridVO.vertex.push_back(vec3(0, 0, -100));
	gridVO.bind();
}

void changeDrawMode(short mode) {
	for (size_t i = 0; i < 6; i++) {
		orbit[i].vo->drawStyle = mode;
	}
	objs[0].vo->drawStyle = mode;
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

GLvoid drawScene() {
	glClearColor(0, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	cam.bind(win);
	Scene::activeScene->render();

	gridShader.use();
	gridVO.render();

	glutSwapBuffers();
}

void timerFunc(int v) {
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
void keyboardUp(unsigned char key, int x, int y);

void main(int argc, char** argv) // 윈도우 출력하고 콜백함수 설정 
{ //--- 윈도우 생성하기
	glutInit(&argc, argv); // glut 초기화
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // 디스플레이 모드 설정
	win.init(800, 600);
	glutInitWindowPosition(0, 30); // 윈도우의 위치 지정
	glutInitWindowSize(win.w, win.h); // 윈도우의 크기 지정
	glutCreateWindow("Example1"); // 윈도우 생성(윈도우 이름)

	glEnable(GL_DEPTH_TEST);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "Unable to initialize GLEW" << std::endl;

		exit(EXIT_FAILURE);
	}/* else
		debug("GLEW Initialized");*/

	init();

	glutDisplayFunc(drawScene); // 출력 함수의 지정
	glutReshapeFunc(Reshape); // 다시 그리기 함수 지정

	glutKeyboardFunc(Keyboard);
	glutKeyboardUpFunc(keyboardUp);
	glutMouseFunc(Mouse);
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
bool bIsRenderLine = true;
void Keyboard(unsigned char key, int x, int y) {
	printf("Keyboard down: %c [%d, %d]\n", key, x, y);

	float moveScale = 0.1f;
	switch (key) {
	case 'w':
		objs[0].getPos().y += moveScale;
		break;
	case 's':
		objs[0].getPos().y -= moveScale;
		break;
	case 'a':
		objs[0].getPos().x += moveScale;
		break;
	case 'd':
		objs[0].getPos().x -= moveScale;
		break;
	case 'z':
		objs[0].getPos().z += moveScale;
		break;
	case 'x':
		objs[0].getPos().z -= moveScale;
		break;
	case 'y':
		if (!objs[0].rotateVelocity)
			objs[0].rotateVelocity = 20;
		objs[0].rotateVelocity = -objs[0].rotateVelocity;
		break;
	case 't':
		bIsRenderLine = !bIsRenderLine;
		if (bIsRenderLine)
			changeDrawMode(GL_LINES);
		else
			changeDrawMode(GL_POLYGON);
		break;
	case 'q': isTimerEnd = true;  glutLeaveMainLoop(); break;
	}
}

void keyboardUp(unsigned char key, int x, int y) {
	printf("Keyboard up: %c [%d, %d]\n", key, x, y);
	switch(key) {
	
	}
}

void specialInput(int key, int x, int y) {
	printf("Keyboard: %c [%d, %d]\n", key, x, y);
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