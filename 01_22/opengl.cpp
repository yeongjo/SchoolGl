#include "ToolModule.h"
#include "bullet.h"
#include "../GL/GLDebugDrawer.h"
#include "stb_image.h"

GLDebugDrawer	gDebugDrawer;

float stageSizeX = 40;
float stageSizeZ = 40;


class Texture {
public:
	int width, height, nrChannels;
	unsigned int id;
	void load(const char* path) {
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// load image, create texture and generate mipmaps
		stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
		// The FileSystem::getPath(...) is part of the GitHub repository so we can find files on any IDE/platform; replace it with your own image path.
		unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
		if (data) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		} else {
			assert("Failed to load texture: " && path && 0);
		}
		stbi_image_free(data);
	}

	void setUniformToShader(Shader& shader, const char* name) {
		glUniform1i(glGetUniformLocation(shader.id, name), 0);
		// or set it via the texture class
		shader.addUniform(name, 1);
	}
};


class PhysicsObj : public Obj {
public:
	btRigidBody* body;
	float mass;
	vec3 physicsOrigin;

	void setBoxPhysics(float mass, const vec3& origin=vec3(0), const vec3& boxSize=vec3(1)) {
		btVector3 localInertia(0, 0, 0);
		bool isDynamic = (mass != 0.f);
		physicsOrigin = origin;

		btTransform startTransform;
		startTransform.setIdentity();
		startTransform.setOrigin(btVector3(origin.x+pos.x, origin.y + pos.y, origin.z + pos.z));

		btCollisionShape* colShape = new btBoxShape(btVector3(boxSize.x, boxSize.y, boxSize.z));
		if (isDynamic)
			colShape->calculateLocalInertia(mass, localInertia);

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);

		this->mass = mass;
		body = new btRigidBody(rbInfo);
		Bullet::dynamicsWorld->addRigidBody(body);
	}

	void tick(float dt) {
		Obj::tick(dt);
		if (body && body->getMotionState()) {
			btTransform trans;
			body->getMotionState()->getWorldTransform(trans);
			pos = vec3(float(trans.getOrigin().getX()), float(trans.getOrigin().getY()), float(trans.getOrigin().getZ())) - physicsOrigin;

			btQuaternion temRot = trans.getRotation();
			quat q = toQuat(temRot);
			quat_2_euler_ogl(q, rot.x, rot.z, rot.y);
			rot = degrees(rot);
			
			/*if (name == "Robot") {
				cout << glm::to_string(pos) << endl;
			}*/
		}
	}

	void move(vec3 vec) {
		body->translate(btVector3(vec.x, vec.y, vec.z));
		//body->translate
	}

	void rotate(vec3 vec) {
		btTransform trans;
		trans = body->getWorldTransform();
		btQuaternion quaternion;
		quaternion.setEuler(vec.y, vec.x, vec.z);
		trans.setRotation(quaternion);
		body->setWorldTransform(trans);
		body->setAngularVelocity(btVector3(0, 0, 0));
	}
};

class Light : public Obj {
public:
	float rotSpeed = 2;
	bool isRotating = true;
	float length = 20;
	float delta = 0;

	Light() : Obj() {
		loadObj("../model/sphere.obj");
		setShader(Shader::shaders["tri"]);
		scale = vec3(0.2f);
		setLightToShader(*shader);
	}

	virtual void initName() {
		name = "Light";
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

Window win;
Camera* cam;

VO gridVO;

Obj objs[11];

vector<PhysicsObj*> obstacle;

Light* light;

Shader triShader, gridShader, stageShader;

bool cameraFirstView = false;


float upPlaneSpinSpeed = 0;
float cubeYSpinSpeed = 0;
float cubeFrontOpenSpeed = 1;
float triSideOpenSpeed = 1;

void onKeyboard(unsigned char key, int x, int y, bool isDown) {
	if (isDown) {
		float moveScale = 10;
		switch (key) {
		case 'r':
			cam->armVector.z = cam->armVector.z < -1 ? -0.001f : -50;
			printf("%f\n", cam->armVector.z);
			break;
		case '2': {
		}
			break;

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
			cam->isPerspective = !cam->isPerspective;
			break;
		case 'z':
			cam->rotateY(moveScale);
			break;
		case 'Z':
			cam->rotateY(-moveScale);
			break;
		}
		
	}
}

void init() {
	glLineWidth(3);

	triShader.complieShader("tri");
	gridShader.complieShader("grid");

	cam = new Camera();
	cam->armVector.z = -50;

	light = new Light();

	vec3 sidesPos[] = { vec3(1, 0, 0), vec3(-1, 0, 0), vec3(0, 0, 1), vec3(0, 0, -1) };
	vec3 sidesRot[] = { vec3(0, 0, 0), vec3(0, 180, 0), vec3(0, 270, 0), vec3(0, 90, 0) };
	vec3 floorsPos[] = { vec3(0, 0, 0), vec3(0, 2, 0), vec3(0, 0, 0) };
	for (size_t i = 0; i < 4; i++) {
		objs[i].loadObj("../model/opengl_0115/cube_front.obj");
		objs[i].getPos() = sidesPos[i];
		objs[i].getRotation() = sidesRot[i];
		objs[i].setParent(&objs[4]);
	}
	for (size_t i = 4; i < 7; i++) {
		objs[i].loadObj("../model/opengl_0115/cube_top.obj");
		objs[i].getPos() = floorsPos[i - 4];
	}
	objs[5].setParent(&objs[4]);
	for (size_t i = 7; i < 11; i++) {
		objs[i].loadObj("../model/opengl_0115/tri_side.obj");
		objs[i].getPos() = sidesPos[i - 7];
		objs[i].getRotation() = sidesRot[i - 7];
		objs[i].setParent(&objs[6]);
	}

	for (size_t i = 0; i < 11; i++) {
		objs[i].shader = &triShader;
	}

	objs[4].getPos().z = -3;
	objs[6].getPos().z = 3;

	gridVO.drawStyle = GL_LINES;
	gridVO.vertex.push_back(vec3(0, 100, 0));
	gridVO.vertex.push_back(vec3(0, -100, 0));
	gridVO.vertex.push_back(vec3(100, 0, 0));
	gridVO.vertex.push_back(vec3(-100, 0, 0));
	gridVO.vertex.push_back(vec3(0, 0, 100));
	gridVO.vertex.push_back(vec3(0, 0, -100));
	gridVO.bind();

	auto objs = Scene::activeScene->objs;
	for (set<TickObj*>::iterator it = objs.begin(); it != objs.end(); ++it) {
		(*it)->initName();
	}

	onKeyboardEvent = onKeyboard;
}


DWORD prevTime = 0;
DWORD thisTickTime = 0;
float dt;
bool looping = false;

void loop() {
	if (prevTime == 0) {
		prevTime = GetTickCount() - 10;
	}
	thisTickTime = GetTickCount();
	dt = (thisTickTime - prevTime) * 0.001f;
	dt = dt > 0.05f ? dt = 0.05f : dt;
	prevTime = thisTickTime;

	cam->tick(dt);
	cam->getRotation().x += dt * 10 * Input::mouse[EMouse::MOUSE_OFF_Y];
	cam->getRotation().y += dt * 10 * Input::mouse[EMouse::MOUSE_OFF_X];

	Bullet::dynamicsWorld->stepSimulation(dt, 10);
	Scene::activeScene->tick(dt);

	Input::getMousePos();

	glutPostRedisplay();
}

GLvoid drawScene()
{
	float gray = 0.3f;
	glClearColor(gray, gray, gray, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	cam->bind(win);
	Scene::activeScene->render();

	gridShader.use();
	gridVO.render();
	Debug::render();
	Bullet::dynamicsWorld->debugDrawWorld();

	glutSwapBuffers(); // 화면에 출력하기
}

void timerFunc(int v) {
	loop();
	glutPostRedisplay();
	if (!looping)
		glutTimerFunc(10, timerFunc, 0);
}

GLvoid drawScene(GLvoid);
GLvoid Reshape(int w, int h);

void main(int argc, char** argv) // 윈도우 출력하고 콜백함수 설정 
{ //--- 윈도우 생성하기
	glutInit(&argc, argv); // glut 초기화
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // 디스플레이 모드 설정
	win.init(800, 600);
	glutInitWindowPosition(0, 30); // 윈도우의 위치 지정
	glutInitWindowSize(win.w, win.h); // 윈도우의 크기 지정
	glutCreateWindow(__FILE__); // 윈도우 생성(윈도우 이름)

	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);
		//--- GLEW 초기화하기
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "Unable to initialize GLEW" << std::endl;

		exit(EXIT_FAILURE);
	}
	else
		std::cout << "GLEW Initialized\n";

	Shader unlit;
	unlit.complieShader("unlit");
	unlit.addUniform("color", new vec3(1,0,1));
	unlit.addUniform("trans", new mat4(1));
	Bullet::initBullet();
	gDebugDrawer.setDebugMode(~0);
	Bullet::dynamicsWorld->setDebugDrawer(&gDebugDrawer);
	init();

	glutDisplayFunc(drawScene); // 출력 함수의 지정
	glutReshapeFunc(Reshape); // 다시 그리기 함수 지정

	bindInput();

	timerFunc(1);

	glutMainLoop(); // 이벤트 처리 시작 
}


GLvoid Reshape(int w, int h) {
	printf("Reshape\n");
	glViewport(0, 0, w, h);
}