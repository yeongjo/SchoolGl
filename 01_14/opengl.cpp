#include "inc/mGlHeader2.h"
#include <map>
#include <set>
#include <time.h>

using namespace std;
using namespace glm;


vec3 getShapeLinesPos(int type, float z);
int render_line_idx = 0;




GLclampf f() {
	return rand() % 255 / 255.0f;
}



class Window {
public:
	uint w, h;
	float ratio;

	void init(int w, int h) {
		this->w = w;
		this->h = h;
		ratio = w / (float)h;
	}
};


class TickObj;
class Scene {
public:
	set<TickObj*> objs;

	void tick(float dt);

	void render();

	void active() {
		activeScene = this;
	}

	static Scene* activeScene;

	static Scene& self() {
		if (activeScene == nullptr)
			activeScene = new Scene();
		return *activeScene;
	}

	static void addObj(TickObj* obj) {
		Scene::self().objs.insert(obj);
	}
};


Scene* Scene::activeScene = nullptr;


class TickObj {
public:
	TickObj() { Scene::addObj(this); }
	virtual void tick(float dt) = 0;
	virtual void render() = 0;
};


void Scene::tick(float dt) {
	for (set<TickObj*>::iterator it = objs.begin(); it != objs.end(); it++) {
		(*it)->tick(dt);
	}
}

void Scene::render() {
	for (set<TickObj*>::iterator it = objs.begin(); it != objs.end(); it++) {
		(*it)->render();
	}
}


class Camera {
public:
	glm::vec3 pos = glm::vec3(0, 0, -1);
	glm::vec3 target;
	glm::vec3 up = glm::vec3(0, 1, 0);
	float fov = 45;

	bool isPerspective = true;

	float rot_speed = 0;
	vec3 rot;
	float arm_length = 1.5f;

	glm::mat4 v = glm::mat4(1.0f), p = glm::mat4(1.0f);
	glm::mat4 vp = glm::mat4(1.0f);

	virtual void tick(float dt) {
		bIsGetTransInThisTick = 0;
		bIsGetVPInThisTick = 0;
	}

	virtual void render(){}

	glm::mat4& getTrans(Window& win) {
		if (bIsGetTransInThisTick) return vp;
		if (~bIsGetVPInThisTick & 0b10) {
			v = glm::lookAt(
				pos, // 월드 공간에서 당신의 카메라 좌표
				target,   // 월드 스페이스에서 당신의 카메라가 볼 곳
				up        // glm::vec(0,1,0) 가 적절하나, (0,-1,0)으로 화면을 뒤집을 수 있습니다. 그래도 멋지겠죠
			);
			bIsGetVPInThisTick |= 0b10;
		}
		if (~bIsGetVPInThisTick & 0b1) {
			if(isPerspective)
				p = glm::perspective(glm::radians(fov), win.ratio, 0.1f, 100.f);
			else {
				float size = 5;
				p = glm::ortho<float>(-size* win.ratio, size* win.ratio, -size, size, 0.1, 50.f);
			}
			bIsGetVPInThisTick |= 0b1;
		}
		mat4 m(1);
		m = rotate(m, radians(rot.z), vec3(0, 0, -1));
		vp = p * m * v;
		bIsGetTransInThisTick = 1;
		return vp;
	}

	void rotateY(float y) {
		rot.y += y;
		pos.z = cos(De2Ra(rot.y)) * arm_length;
		pos.x = sin(De2Ra(rot.y)) * arm_length;
		pos.y = arm_length * 0.7f;
	}

	void rotateZ(float z) {
		rot.z += z;
		/*auto forward = normalize(target - pos);
		cross(forward, )
		up.y = cos(De2Ra(rot.y));
		up.z = sin(De2Ra(rot.y));
		up.x = 0;*/
	}

private:
	uint bIsGetTransInThisTick = 0;
	uint bIsGetVPInThisTick = 0;
};


class IShaderUniform {
public:
	virtual void setName(unsigned shaderIdx, const string& name) = 0;
	virtual void setValuePtr(void* ptr) = 0;
	virtual void setValueUniquePtr(void* ptr) = 0;
	virtual void setUniform(void* ptr = nullptr) = 0;
};


template<class T>
class ShaderUniform : public IShaderUniform {
public:
	int uniformLocation = -2;
	string uniformName;
	T* valuePtr; // 보낼 값이 있는 포인터
	bool bIsUniquePtr; // valuePtr이 나만 가지고있는지?

	// 이름설정
	void setName(unsigned shaderIdx, const string& name) {
		uniformName = name;
		uniformLocation = glGetUniformLocation(shaderIdx, name.c_str());
	}

	// 쉐이더에 넣을 값 포인터 설정
	void setValuePtr(T* ptr) {
		valuePtr = ptr;
		bIsUniquePtr = false;
	}

	// 쉐이더에 넣을 값 포인터 설정
	void setValueUniquePtr(T* ptr) {
		valuePtr = ptr;
		bIsUniquePtr = true;
	}

	// 쉐이더에 값 전송
	// 값이 변경될때만 하면되나?
	// TODO 쉐이더 use 에서 호출되니까 파라미터로 뭘 넘길수가 없어서 나중에 손봐야함
	void setUniform(void* ptr = nullptr) {
		if (uniformLocation != -1) {
			if(ptr)
				setUniformWithType(ptr);
			else
				setUniformWithType(valuePtr);
		}
	}

	~ShaderUniform() {
		if (bIsUniquePtr)
			delete valuePtr;
	}

	void setUniformWithType(void* ptr);

	void setValuePtr(void* ptr) {
		setValuePtr((T*)ptr);
	}

	void setValueUniquePtr(void* ptr) {
		setValueUniquePtr((T*)ptr);
	}
};


template<>
void ShaderUniform<mat4>::setUniformWithType(void* ptr) {
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, (const float*)ptr);
}

template<>
void ShaderUniform<vec3>::setUniformWithType(void* ptr) {
	glUniform3f(uniformLocation, 1, GL_FALSE, *(GLfloat*)ptr);
}


class Shader {
public:
	unsigned int id;
	map<string, IShaderUniform*> shaderUniforms;

	void complieShader(const char* shaderPath) {
		id = ::complieShader(shaderPath);
	}

	void addUniform(const string& name, mat4* ptr) {
		auto uniform = new ShaderUniform<mat4>();
		print("mat4 add uniform")
			insertShaderUniform(uniform, name, ptr);
	}

	void addUniform(const string& name, vec3* ptr) {
		auto uniform = new ShaderUniform<vec3>();
		print("vec3 add uniform")
			insertShaderUniform(uniform, name, ptr);
	}

	void addUniform(const string& name, const mat4& ptr) {
		auto uniform = new ShaderUniform<mat4>();
		print("mat4 add unique uniform");
		if (uniform->valuePtr) {
			*(mat4*)uniform->valuePtr = ptr;
			insertShaderUniformWithUniqueValue(uniform, name, uniform->valuePtr);
		} else
			insertShaderUniformWithUniqueValue(uniform, name, new mat4(ptr));
	}

	// TODO 예외처리해서 실수 막을건지 아님 너그럽게 봐줄건지
	IShaderUniform* getUniform(const char* name) {
		auto findKey = shaderUniforms.find(name);
		if (findKey == shaderUniforms.end()) {
			assert(findKey != shaderUniforms.end());
			return nullptr;
		}
		return findKey->second;
	}

	void changeUniformValue(const char* name, void* ptr) {
		auto uniform = getUniform(name);
		if(uniform)
			getUniform(name)->setValuePtr(ptr);
	}

	void use() {
		glUseProgram(id);
		if (!shaderUniforms.empty())
			for (auto it = shaderUniforms.begin(); it != shaderUniforms.end(); it++) {
				it->second->setUniform();
			}
	}

private:
	void insertShaderUniform(IShaderUniform* uniform, const string& name, void* ptr) {
		uniform->setName(id, name);
		uniform->setValuePtr(ptr);
		shaderUniforms.insert(make_pair(name, uniform));
	}

	void insertShaderUniformWithUniqueValue(IShaderUniform* uniform, const string& name, void* ptr) {
		uniform->setName(id, name);
		uniform->setValueUniquePtr(ptr);
		shaderUniforms.insert(make_pair(name, uniform));
	}
};

class CameraShaderUniformBuffer{
public:
	unsigned int UBO;
	mat4* p, * v, * vp;
	
	void create(Camera& cam, Window& win) {
		glGenBuffers(1, &UBO);
		glBindBuffer(GL_UNIFORM_BUFFER, UBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4) * 3, NULL, GL_STATIC_DRAW); // 152 바이트 메모리 할당
		setData(cam, win);
	}

	void setData(Camera& cam, Window& win) {
		this->p = &cam.p;
		this->v = &cam.v;
		this->vp = &cam.getTrans(win);
	}

	// 값변경될때마다 한번만 호출하면되나
	void bindBuffer() {
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), p);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), v);
		glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), sizeof(glm::mat4), vp);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, UBO);
	}
};


class Obj : public TickObj {
public:
	VO* vo;

	glm::vec3 pos;
	vec3 scale = vec3(1);
	vec3 rot;

	Obj* parentObj;

	Shader* shader;

	glm::mat4 trans = glm::mat4(1.0f);
	vec3 color = vec3(f(), f(), f());

	float rotateVelocity = 0;
	float speed = 1;


	// call after this have verteies
	void loadObj(const char* path) {
		vo = VO::loadObj(path);
		vo->bind();
	}

	void destroy() {
	}

	void setPos(const vec3& p) {
		pos = p; bIsGetTransInThisTick = false;
	}
	void setRotation(const vec3& r) {
		rot = r; bIsGetTransInThisTick = false;
	}
	void setScale(const vec3& s) {
		scale = s; bIsGetTransInThisTick = false;
	}

	void rotateY(float y) {
		bIsGetTransInThisTick = false; rot.y += y;
	}

	glm::mat4& getTrans() {
		if (bIsGetTransInThisTick) return trans;
		trans = glm::mat4(1.0f);
		//trans = glm::rotate(trans, glm::radians(rotrot), glm::vec3(0.0, 1, 0));
		trans = glm::translate(trans, pos);
		trans = glm::scale(trans, scale);
		trans = glm::rotate(trans, glm::radians(rot.y), glm::vec3(0.0, 1, 0));
		trans = glm::rotate(trans, glm::radians(rot.z), glm::vec3(0.0, 0.0, 1.0));
		trans = glm::rotate(trans, glm::radians(rot.x), glm::vec3(1, 0.0, 0));
		if (parentObj)
			trans = parentObj->getTrans() * trans;
		bIsGetTransInThisTick = true;
		return trans;
	}

	virtual void tick(float dt) {
		rotateY(speed * rotateVelocity);
	}

	void setColor(GLuint shaderId) {
		unsigned int loc = glGetUniformLocation(shaderId, "vcolor");
		glUniform3f(loc, color.x, color.y, color.z);
	}

	virtual void render() {
		if (shader) {
			shader->changeUniformValue("trans", &getTrans());
			shader->use();
			setColor(shader->id);
		}
		vo->render();
	}

private:
	bool bIsGetTransInThisTick = false;
};


Window win;
Camera cam;
Shader triShader, gridShader;

VO gridVO;

GLUquadricObj* qobj;

Obj objs[11];

bool isTimerEnd = false;

CameraShaderUniformBuffer camShader;

void init() {
	glLineWidth(2);
	qobj = gluNewQuadric(); // 객체 생성하기
	gluQuadricDrawStyle(qobj, GLU_LINE); // 도형 스타일

	triShader.complieShader("tri");
	gridShader.complieShader("grid");

	triShader.addUniform("trans", &objs[0].getTrans());

	gridShader.addUniform("trans", mat4(1));

	cam.arm_length = 20;
	cam.rotateY(120);

	camShader.create(cam, win);
	camShader.bindBuffer();

	//objs[0].loadObj("../model/sphere.obj");
	vec3 sidesPos[] = { vec3(1, 0, 0), vec3(-1, 0, 0), vec3(0, 0, 1), vec3(0, 0, -1) };
	vec3 sidesRot[] = { vec3(0, 0, 0), vec3(0, 180, 0), vec3(0, 270, 0), vec3(0, 90, 0) };
	vec3 floorsPos[] = { vec3(0, 0, 0), vec3(0, 2, 0), vec3(0, 0, 0)};
	for (size_t i = 0; i < 4; i++) {
		objs[i].loadObj("../model/opengl_0115/cube_front.obj");
		objs[i].pos = sidesPos[i];
		objs[i].rot = sidesRot[i];
		objs[i].parentObj = &objs[4];
	}
	for (size_t i = 4; i < 7; i++) {
		objs[i].loadObj("../model/opengl_0115/cube_top.obj");
		objs[i].pos = floorsPos[i - 4];
	}
	objs[5].parentObj = &objs[4];
	for (size_t i = 7; i < 11; i++) {
		objs[i].loadObj("../model/opengl_0115/tri_side.obj");
		objs[i].pos = sidesPos[i - 7];
		objs[i].rot = sidesRot[i - 7];
		objs[i].parentObj = &objs[6];
	}

	for (size_t i = 0; i < 11; i++) {
		objs[i].shader = &triShader;
	}

	objs[4].pos.z = -3;
	objs[6].pos.z = 3;

	gridVO.drawStyle = GL_LINES;
	gridVO.vertex.push_back(vec3(0, 100, 0));
	gridVO.vertex.push_back(vec3(0, -100, 0));
	gridVO.vertex.push_back(vec3(100, 0, 0));
	gridVO.vertex.push_back(vec3(-100, 0, 0));
	gridVO.vertex.push_back(vec3(0, 0, 100));
	gridVO.vertex.push_back(vec3(0, 0, -100));
	gridVO.bind();
}


float upPlaneSpinSpeed = 0;
float cubeYSpinSpeed = 0;
float cubeFrontOpenSpeed = 1;
float triSideOpenSpeed = 1;
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

	objs[5].rot.z += dt * 90 * upPlaneSpinSpeed;
	objs[4].rot.y += dt * 90 * cubeYSpinSpeed;
	objs[6].rot.y += dt * 90 * cubeYSpinSpeed;
	objs[0].rot.z += dt * 90 * cubeFrontOpenSpeed;
	if (objs[0].rot.z > 0) objs[0].rot.z = 0;
	if (objs[0].rot.z < -120) objs[0].rot.z = -120;

	int rotIdx[] = {2, 2, 2, 2};
	for (size_t i = 0; i < 4; i++) {
		objs[i+7].rot[rotIdx[i]] += dt * 90 * triSideOpenSpeed;
		if (objs[i + 7].rot[rotIdx[i]] > 0) objs[i + 7].rot[rotIdx[i]] = 0;
		if (objs[i + 7].rot[rotIdx[i]] < -120) objs[i + 7].rot[rotIdx[i]] = -120;
	}

	glutPostRedisplay();
}

GLvoid drawScene() {
	glClearColor(0, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "Unable to initialize GLEW" << std::endl;

		exit(EXIT_FAILURE);
	}/* else
		debug("GLEW Initialized");*/

	glEnable(GL_DEPTH_TEST);

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
void Keyboard(unsigned char key, int x, int y) {
	printf("Keyboard down: %c [%d, %d]\n", key, x, y);

	switch (key) {

	case 'q': isTimerEnd = true;  glutLeaveMainLoop(); break;
	}
}

void keyboardUp(unsigned char key, int x, int y) {
	printf("Keyboard up: %c [%d, %d]\n", key, x, y);
	float moveScale = 10.f;
	switch(key) {
	case 'y':
		cubeYSpinSpeed = !cubeYSpinSpeed;
	break; 
	case 't':
		upPlaneSpinSpeed = !upPlaneSpinSpeed;
		break;
	case 'f':
		cubeFrontOpenSpeed = -cubeFrontOpenSpeed;
		break;
	case 'o':
		triSideOpenSpeed = -triSideOpenSpeed;
		break;
	case 'p':
		cam.isPerspective = !cam.isPerspective;
		break;
	case 'z':
		cam.rotateY(moveScale);
		break;
	case 'Z':
		cam.rotateY(-moveScale);
		break;
	}
	cam.getTrans(win);
	camShader.bindBuffer();
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