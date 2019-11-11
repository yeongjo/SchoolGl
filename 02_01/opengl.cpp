#include "ToolModule.h"
#include <stack>
#include "../glm/gtx/spline.hpp"

using namespace std;
using namespace glm;

int render_line_idx = 0;


Shader triShader, gridShader;


GLclampf f() {
	return rand() % 255 / 255.0f;
}


class ccwVec2 : public vec2 {
public:
	ccwVec2(vec2 v) {
		x = v.x;
		y = v.y;
		this->p = 0;
		this->q = 1;
	}
	float p, q;
};


class SplineObj : public Obj {
public:
	static bool isVisable;
	virtual void render() {
		if (isVisable)
			Obj::render();
	}
};


bool SplineObj::isVisable = true;


class CuttableObj;
vector<CuttableObj*> cuttableVec;

class CuttableObj : public Obj {
	vec3 velocity;
public:
	CuttableObj() : Obj() {
		cuttableVec.push_back(this);
	}

	void initRoot() {
		GenertateRandomPolygon();
		//scale = vec3(0.2f);
	}

	void setVelocity(float x, float y) {
		velocity.x = x;
		velocity.y = y;
		velocity.z = 0;
	}

	virtual ~CuttableObj() {
		Obj::~Obj();
		auto size = cuttableVec.size();
		for (size_t i = 0; i < size; i++) {
			if (cuttableVec[i] == this) {
				cuttableVec.erase(cuttableVec.begin() + i);
				break;
			}
		}
		if (splineObj)
			splineObj->destroy();
	}

	void removeDoubles(vector<vec2>& vertices) {
		sort(vertices.begin(), vertices.end(), ::operator<);
		if (vertices.begin() != vertices.end()) {
			for (auto current = vertices.begin(), prev = vertices.begin();
				++current != vertices.end(); ) {
				if (glm::length(*prev - *current) < 0.001f) {
					auto removeCurrent = current;
					current = --vertices.erase(removeCurrent);
				} else {
					++prev;
				}
			}
		}
	}

	//vector<vec2> colliderVertices;
	void checkCollide(vec3 startPos, vec3 endPos) {
		vec2 startPos2 = startPos;
		vec2 endPos2 = endPos;
		vector<vec2> collideVertex;
		vec2 t1, t2;
		for (size_t i = 0; i < vo->vertex.size(); i++) {
			vec2 x;
			if (i == vo->vertex.size() - 1) {
				t1 = getTrans() * vec4(vo->vertex[0], 1);
				t2 = getTrans() * vec4(vo->vertex[vo->vertex.size() - 1], 1);
			} else {
				t1 = getTrans() * vec4(vo->vertex[i], 1);
				t2 = getTrans() * vec4(vo->vertex[i + 1], 1);
			}
			if (segmentIntersection(t1, t2, startPos2, endPos2, x)) {
				collideVertex.push_back(x);
			}
		}
		if (collideVertex.size() <= 1) return;
		vector<vec2> leftVertices;
		vector<vec2> rightVertices;

		for (size_t i = 0; i < vo->vertex.size(); i++) {
			auto currentVertex = vec2(getTrans() * vec4(vo->vertex[i], 1));
			t1 = endPos2 - startPos2;
			t2 = currentVertex - startPos2;
			float isOnLeft = cross(t2, t1);// 양수가 나오면 왼쪽에 위치한 버텍스
			if (isOnLeft > 0)
				leftVertices.push_back(currentVertex);
			else
				rightVertices.push_back(currentVertex);
		}

		removeDoubles(collideVertex);
		removeDoubles(leftVertices);
		removeDoubles(rightVertices);
		if (collideVertex.size() <= 1) return;

		leftVertices.insert(leftVertices.end(), collideVertex.begin(), collideVertex.end());
		ccwSort(leftVertices);

		rightVertices.insert(rightVertices.end(), collideVertex.begin(), collideVertex.end());
		ccwSort(rightVertices);



		vector<vec3> vertex1, vertex2;
		for (size_t i = 1; i < leftVertices.size() - 1; i++) {
			vertex1.push_back(vec3(leftVertices[0], 0));
			vertex1.push_back(vec3(leftVertices[i], 0));
			vertex1.push_back(vec3(leftVertices[i + 1], 0));
		}
		for (size_t i = 1; i < rightVertices.size() - 1; i++) {
			vertex2.push_back(vec3(rightVertices[0], 0));
			vertex2.push_back(vec3(rightVertices[i], 0));
			vertex2.push_back(vec3(rightVertices[i + 1], 0));
		}
		auto inverseMat4 = inverse(getTrans());
		vo->vertex.clear();
		for (size_t i = 0; i < vertex1.size(); i++) {
			vo->vertex.push_back(inverseMat4 * vec4(vertex1[i], 1));
		}
		vo->bind();
		name = "this";
		if(splineObj != nullptr)
			splineObj->destroy();
		splineObj = nullptr;
		setVelocity(-0.1f, 0.01f);
		//vo->drawStyle = GL_LINE_LOOP;

		auto otherObj = new CuttableObj;
		otherObj->vo = new VO();
		for (size_t i = 0; i < vertex2.size(); i++) {
			otherObj->vo->vertex.push_back(inverseMat4 * vec4(vertex2[i], 1));
		}
		otherObj->setPos(pos);
		otherObj->setScale(scale);
		otherObj->setRotation(rot);
		otherObj->vo->bind();
		otherObj->setShader(&triShader);
		//otherObj->vo->drawStyle = GL_LINE_LOOP;
		otherObj->color = vec3(0, 0, 1);
		otherObj->name = "other";
		otherObj->setVelocity(0.1f, 0.01f);

		//for (size_t i = 0; i < leftVertices.size(); i++) {
		//	auto obj = new Obj();
		//	obj->parentObj = this;
		//	obj->loadObj("../model/cube.obj");
		//	obj->setShader(&triShader);
		//	obj->setPos(vec3(vec2(inverseMat4 * vec4(leftVertices[i], 0, 1)) + vec2(-0.01f, 0), -0.9f));
		//	obj->setScale(vec3(0.01f));
		//	//obj->color = vec3((float)i/ leftVertices.size(), (float)i / leftVertices.size(), (float)i / leftVertices.size());
		//}

		//for (size_t i = 0; i < rightVertices.size(); i++) {
		//	auto obj = new Obj();
		//	obj->parentObj = otherObj;
		//	obj->loadObj("../model/cube.obj");
		//	obj->setShader(&triShader);
		//	obj->setPos(vec3(vec2(inverseMat4 * vec4(rightVertices[i], 0, 1)) + vec2(0.01f, 0), -0.9f));
		//	obj->setScale(vec3(0.01f));
		//	obj->color = vec3((float)i / rightVertices.size(), (float)i / rightVertices.size(), (float)i / rightVertices.size());
		//}
	}

private:


	bool cmp(const ccwVec2& a, const ccwVec2& b) {
		if (a.q * b.p != a.p * b.q)
			return a.q * b.p < a.p * b.q;
		if (a.y != b.y)
			return a.y < b.y;
		return a.x < b.x;
	}

	float ccw(const vec2& A, const vec2& B, const vec2& C) {
		return (A.x * B.y + B.x * C.y + C.x * A.y - B.x * A.y - C.x * B.y - A.x * C.y);
	}

	void ccwSort(vector<vec2>& refSortArr) {
		vector<ccwVec2> vectorCoordLen;
		for (size_t i = 0; i < refSortArr.size(); i++) {
			ccwVec2 t(refSortArr[i]);
			vectorCoordLen.push_back(t);
		}
		std::sort(vectorCoordLen.begin(), vectorCoordLen.end(),
			[=](const ccwVec2& a, const ccwVec2& b) {return cmp(a, b); });

		// 기준점으로부터 상대 위치 계산
		for (size_t i = 1; i < vectorCoordLen.size(); i++) {
			vectorCoordLen[i].p = vectorCoordLen[i].x - vectorCoordLen[0].x;
			vectorCoordLen[i].q = vectorCoordLen[i].y - vectorCoordLen[0].y;
		}
		// 반시계 방향으로 정렬(기준점 제외)
		sort(vectorCoordLen.begin() + 1, vectorCoordLen.end(),
			[=](const ccwVec2& a, const ccwVec2& b)->bool {return cmp(a, b); });

		stack<int> s;
		s.push(0);
		s.push(1);

		int next = 2;

		while (next < vectorCoordLen.size()) {
			while (s.size() >= 2) {
				int first, second;
				second = s.top();
				s.pop();
				first = s.top();

				// first, second, next가 좌회전 ( > 0 )이라면 second push
				// 우회전( < 0 )이라면 위의 while문 계속 반복
				if (ccw(vectorCoordLen[first], vectorCoordLen[second], vectorCoordLen[next]) >= 0) {
					s.push(second);
					break;
				}
			}

			// next push
			s.push(next++);
		}

		refSortArr[0].x = vectorCoordLen[0].x;
		refSortArr[0].y = vectorCoordLen[0].y;

		int _size = refSortArr.size();
		int i = 0;
		while (!s.empty()) {
			int idx = s.top();
			debug("[%d]idx %d", i, idx);
			s.pop();
			refSortArr[i].x = vectorCoordLen[idx].x;
			refSortArr[i].y = vectorCoordLen[idx].y;
			++i;
		}
	}

public:
	float gravity = 0.04f;
	void GenertateRandomPolygon() {
		vector<vec3> randomVertex = { vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 1, 0), vec3(0, 1, 0) };
		for (size_t i = 0; i < randomVertex.size(); i++) {
			randomVertex[i] = (randomVertex[i] - vec3(0.5f, 0.5f, 0)) * 0.5f;
			randomVertex[i] += vec3(f() / 2 - 0.25f, f() / 2 - 0.25f, 0);
		}
		vo = new VO();
		for (size_t i = 1; i < 3 - rand() % 2; i++) {
			vo->vertex.push_back(randomVertex[0]);
			vo->vertex.push_back(randomVertex[i]);
			vo->vertex.push_back(randomVertex[i + 1]);

		}
		vo->drawStyle = GL_LINE_LOOP;
		vo->bind();
	}

	float splineAmount = 0;
	virtual void tick(float dt) {
		if (splineObj) {
			splineAmount += dt * 0.1f * gMoveSpeed;
			if (splineAmount < 1)
				pos = getSplinePos(splineAmount);
		} else {
			velocity.y -= gravity * dt * 6.0f;
			pos += velocity * dt * gMoveSpeed;
		}
		float clipingRange = 1.5f;
		if (pos.y < -clipingRange || pos.x > clipingRange || pos.x < -clipingRange) {
			destroy();
			return;
		}
		changedTransform();
	}

	Obj* splineObj = nullptr;

	static bool isLineDrawMode;
	static float gMoveSpeed;

	virtual void render() {
		vo->drawStyle = isLineDrawMode ? GL_LINE_LOOP : GL_POLYGON;
		Obj::render();
	}

	vector<vec3> splinePoints;

	vec3 getSplinePos(float t) {
		float j = (splinePoints.size() - 3) * t;
		int i = (int)j;
		j = mod(j, 1.0f);
		return catmullRom(splinePoints[i], splinePoints[i + 1], splinePoints[i + 2], splinePoints[i + 3], j);

	}

	void makeSpline() {
		if (!splineObj) {
			splineObj = new SplineObj();
			splineObj->vo = new VO();
			splineObj->name = "spline";
			splineObj->color = vec3(0, 0, 1);
			splineObj->setShader(&triShader);
			splinePoints = getSplinePoint(10.f);
			for (size_t i = 0; i < splinePoints.size() - 3; i++) {
				float divideCount = 10;
				for (size_t j = 0; j < divideCount; j++) {
					splineObj->vo->vertex.push_back(catmullRom(splinePoints[i], splinePoints[i + 1], splinePoints[i + 2], splinePoints[i + 3], j / divideCount));
				}
			}
			splineObj->vo->drawStyle = GL_LINE_STRIP;
			splineObj->vo->bind();
		}
	}

	vector<vec3> getSplinePoint(float amount) const {
		vector<vec3> t;
		vec3 _vel = velocity;
		vec3 _pos = pos;
		t.push_back(vec3(pos.x, pos.y - amount*(velocity.y + amount *gravity), 0));
		t.push_back(_pos);
		for (size_t i = 0; i < 3; i++) {
			_vel.y -= amount * gravity;
			_pos += _vel * amount;
			t.push_back(_pos);
		}
		return t;
	}
};

bool CuttableObj::isLineDrawMode = true;
float CuttableObj::gMoveSpeed = 1;

class CuttingObj;

class CutController {
public:
	vec3 startPos;
	vec3 endPos;

	bool isDraging = false;

	CuttingObj *cuttingObj = nullptr;


	void startDrag(const vec3& pos) {
		isDraging = true;
		startPos = pos;
		setMovingMousePos(pos);
	}

	void endDrag(const vec3& pos) {
		isDraging = false;
		setMovingMousePos(pos);
		// TODO add cut
		size_t size = cuttableVec.size();
		for (size_t i = 0; i < size; i++) {
			cuttableVec[i]->checkCollide(startPos, endPos);
		}
	}

	void setMovingMousePos(const vec3& pos);
};

class CuttingObj : public Obj {
public:
	CutController* controller;

	CuttingObj(CutController* controller) : Obj() {
		vo = new VO();
		for (size_t i = 0; i < 2; i++){
			vo->vertex.push_back(vec3());
		}
		vo->drawStyle = GL_LINES;

		this->controller = controller;
		this->controller->cuttingObj = this;
	}

	void init() {
		vo->bind();
	}

	void onChangedVertex() {
		vo->vertex[0] = controller->startPos;
		vo->vertex[1] = controller->endPos;
		vo->bind();
	}

	virtual void render() {
		assert(controller && "controller not assigned");
		if (!controller->isDraging) return;
		Obj::render();
	}
};

void CutController::setMovingMousePos(const vec3& pos) {
	if (!isDraging) return;
	endPos = pos;
	assert(cuttingObj);
	cuttingObj->onChangedVertex();
}

Window win;
Camera cam;

CuttingObj* cuttingObj;

CuttableObj* cuttableObj;


VO gridVO;

CameraShaderUniformBuffer camShader;

bool isTimerEnd = false;

void throwCuttableObj(float direc) {
	cuttableObj = new CuttableObj();
	cuttableObj->setShader(&triShader);
	cuttableObj->initRoot();
	cuttableObj->setPos(vec3(direc * 1.4f, 0, 0));
	cuttableObj->setVelocity(0.3f* -direc + f()*0.4f, 0.2f + f()*0.2f);
	cuttableObj->makeSpline();
}

void init() {
	glLineWidth(3);

	triShader.complieShader("tri");
	gridShader.complieShader("grid");

	cuttingObj = new CuttingObj(new CutController());
	cuttingObj->init();
	cuttingObj->setShader(&triShader);
	
	cam.arm_length = 20;
	cam.rotateY(120);

	camShader.create(cam, win);
	camShader.bindBuffer();

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
float throwRemainTime = 3;

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

	throwRemainTime += dt;
	if (throwRemainTime > 3) {
		throwRemainTime = 0;
		throwCuttableObj(rand()%2?-1:1);
	}

	glutPostRedisplay();
}

GLvoid drawScene() // 콜백 함수: 출력
{
	//printf("drawScene\n");
	glClearColor(0, 0, 0, 1.0f); // 바탕색을 ‘blue’로 지정
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 설정된 색으로 전체를 칠하기


	camShader.bindBuffer();

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
	glutInitWindowPosition(1920, 0); // 윈도우의 위치 지정
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
	case 'w':
		SplineObj::isVisable = !SplineObj::isVisable;
		break;
	case 'e':
		CuttableObj::isLineDrawMode = !CuttableObj::isLineDrawMode;
		break;
	case 'r':
		CuttableObj::gMoveSpeed *= 2;
		break;
	case 't':
		CuttableObj::gMoveSpeed /= 2;
		break;
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
	float screenMouseX = _x / (800.0f / 2) - 1;
	float screenMouseY = -_y / (600.0f / 2) + 1;
	debug("Mouse: button(%d), state(%d), window(%d,%d), screen(%02f, %02f)", button, state, _x, _y, screenMouseX, screenMouseY);

	switch (state) {
	case 0: // Mouse Down
		cuttingObj->controller->startDrag(vec3(screenMouseX, screenMouseY, 0));
		break;
	case 1: // Mouse Up
		cuttingObj->controller->endDrag(vec3(screenMouseX, screenMouseY, 0));
		break;
	}
}

void mouseMotionHandler(int x, int y) {
	float screenMouseX = x / (800.0f / 2) - 1;
	float screenMouseY = -y / (600.0f / 2) + 1;
	cuttingObj->controller->setMovingMousePos(vec3(screenMouseX, screenMouseY, 0));
}