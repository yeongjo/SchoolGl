#include "ToolModule.h"
#include "bullet.h"
#include "../GL/GLDebugDrawer.h"

GLDebugDrawer	gDebugDrawer;

float stageSizeX = 40;
float stageSizeZ = 40;

void quat_2_euler_ogl(const quat& q, float& yaw, float& pitch, float& roll) {
	float sqw = q.w * q.w;
	float sqx = q.x * q.x;
	float sqy = q.y * q.y;
	float sqz = q.z * q.z;
	pitch = asinf(2.0f * (q.y * q.z + q.w * q.x)); // rotation about x-axis
	roll = atan2f(2.0f * (q.w * q.y - q.x * q.z), (-sqx - sqy + sqz + sqw)); // rotation about y-axis
	yaw = atan2f(2.0f * (q.w * q.z - q.x * q.y), (-sqx + sqy - sqz + sqw)); // rotation about z-axis
}

const vec3 toVec3(btVector3 vec) {
	return vec3(vec.getX(), vec.getY(), vec.getZ());
}

const quat toQuat(btQuaternion quaternion) {
	quat t(quaternion.w(), quaternion.x(), quaternion.y(), quaternion.z());
	return t;
}

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


class RobotPart : public Obj {
public:
	vec3 rotSpeed = vec3(0, 0, 60);

	virtual void initName() {
		name = typeid(this).name();
	}

	void tick(float dt) {
		float z = rotSpeed.z * dt;

		Obj::rotateZ(z);
		if (rot.z < -45 || rot.z > 45) rotSpeed.z = -rotSpeed.z;
	}
};


class StageDoor : public Obj {
public:
	bool isOpen = false;

	void tick(float dt) {
		float speed = 3.0f;

		if (isOpen) {
			pos.y += dt * speed;
			if (pos.y > 5) pos.y = 5;
		} else {
			pos.y -= dt * speed;
			if (pos.y < 0) pos.y = 0;
		}
	}
};


class Player : public PhysicsObj {
public:
	Camera* cam;
	vec2 moveVec;
	float speed = 20;
	float rotSpeed = 10;
	float rotY = 0;

	static Player* self;

	Player(Camera* cam) : PhysicsObj(), cam(cam) {
		loadObj("../model/numbers/9.obj");
		setShader(Shader::shaders["tri"]);
		cam->setParent(this);
		self = this;
	}

	void tick(float dt) {
		PhysicsObj::tick(dt);

		if (Input::mouse[EMouse::MOUSE_L_BUTTON]) {
			//debug("%d %d", Input::mouse[EMouse::MOUSE_OFF_X], Input::mouse[EMouse::MOUSE_OFF_Y]);
			rotY += dt * rotSpeed * 0.03f * Input::mouse[EMouse::MOUSE_OFF_X];
			rotate(vec3(0, rotY, 0));
			if(cam->GetParent() != NULL)
			cam->getRotation().x += dt * rotSpeed * Input::mouse[EMouse::MOUSE_OFF_Y];
			//cam->getRotation().y += dt * rotSpeed * Input::mouse[EMouse::MOUSE_OFF_X];
		}

		auto forward = getForward();
		auto right = glm::cross(forward, cam->up);
		if (cam->GetParent() == NULL) {
			cam->setPos(pos);
			forward = vec3(0, 0, 1);
			right = vec3(-1, 0, 0);
		}
		move((speed * Input::moveVec.y *dt) * forward +
			(speed * Input::moveVec.x * dt) * right);

		Debug::drawLine(getForward()*3.0f+ pos, pos);

		
	}

	void toggleQuaterView() {
		if (cam->GetParent() == NULL) {
			cam->setParent(this);
			cam->armVector.z = -50;
			cam->armVector.y = 0;
			cam->setPos(vec3(0));
		}
		else {
			cam->setParent(NULL);
			cam->armVector.z = -100;
			cam->armVector.y = 40;
		}
	}

	virtual void render() {
		if (cam->armVector.z < -1.f) {
			Obj::render();
		}
	}

	virtual void initName() {
		name = "Player";
	}
};

Player* Player::self;

class Robot : public PhysicsObj {
public:
	vec3 moveSpeed;
	RobotPart leg[2];
	RobotPart arm[2];

	float accel = 0;
	float gravity = -3.f;

	Obj sightObj;

	float sightLen = 15;

	void initParts() {
		moveSpeed = vec3(f()-0.5f, f() - 0.5f, f() - 0.5f);
		for (size_t i = 0; i < 2; i++) {
			arm[i].loadObj("../model/opengl_0118_robot/robot_arm.obj");
			arm[i].setParent(this);
			arm[i].setShader(shader);
			arm[i].color = randColor();
		}
		arm[1].rotateY(180);

		for (size_t i = 0; i < 2; i++) {
			leg[i].loadObj("../model/opengl_0118_robot/robot_leg.obj");
			leg[i].setParent(this);
			leg[i].setShader(shader);
			leg[i].color = randColor();
			leg[i].setPos(vec3(0, -0.7f, 0));
		}
		leg[1].rotateY(180);

		sightObj.name = "sightObj";
		sightObj.loadObj("../model/sphere.obj");
		sightObj.setParent(this);
		sightObj.setScale(vec3(sightLen));
		sightObj.setShader(Shader::shaders["unlit"]);
		sightObj.vo->drawStyle = GL_LINES;
	}

	virtual void initName() {
		name = "Robot";
	}

	void jump() {
		accel = 0.6f;
	}

	void tick(float dt) {
		PhysicsObj::tick(dt);



		if (pos.x < -stageSizeX)
			moveSpeed.x = abs(moveSpeed.x);
		if(pos.x > stageSizeX)
			moveSpeed.x = -abs(moveSpeed.x);
		if (pos.z < -stageSizeZ)
			moveSpeed.z = abs(moveSpeed.z);
		if (pos.z > stageSizeZ)
			moveSpeed.z = -abs(moveSpeed.z);

		auto offVec = Player::self->getPos() - pos;
		auto len = glm::length(offVec);
		if (len < sightLen) {
			if (len < 5)
				moveSpeed = vec3(0);
			else {
				auto forward = getForward();
				auto angle = glm::dot(forward, glm::normalize(offVec));
				auto sightAngle = glm::dot(forward, Player::self->getForward());
				if (angle > 0.5f && sightAngle < -0.5f) {
					moveSpeed = normalize(offVec);
					Debug::drawLine(Player::self->getPos(), pos, vec3(0, 0, 1));
				} else {
					if (sightAngle < -0.5f) {
						Debug::drawLine(Player::self->getPos(), pos, vec3(1, 0, 0));
					}
					if (angle > 0.5f) {
						Debug::drawLine(Player::self->getPos(), pos, vec3(0, 1, 0));
					}
				}

				Debug::drawLine(Player::self->getPos(), pos, vec3(0, 1, 1));
			}
		} else if(glm::length(moveSpeed)<0.1f){
			moveSpeed = normalize(-offVec);
		}
		Debug::drawLine(getForward()*5.0f + pos, pos, vec3(1,1,0));
		PhysicsObj::move(moveSpeed * 12.0f * dt);

		//print("%f", rot.y);
		if (glm::length(moveSpeed) > 0.1f) {
			float angle = glm::atan(moveSpeed.x, moveSpeed.z);
			vec3 t;
			t.y = angle; glm::degrees(angle);
			t.x = t.z = 0;
			rotate(t);
		}
	}

	void render() {
		Obj::render();
		sightObj.render();
	}
};

Window win;
Camera* cam;

VO gridVO;

vector<Robot*> robotObj;
Player* player;

PhysicsObj* stage;

vector<PhysicsObj*> obstacle;

Light* light;

Shader triShader, gridShader, stageShader;

bool cameraFirstView = false;

void onKeyboard(unsigned char key, int x, int y, bool isDown) {
	if (isDown) {
		switch (key) {
		case 'r':
			cam->armVector.z = cam->armVector.z < -1 ? -0.001f : -50;
			printf("%f\n", cam->armVector.z);
			break;
		case '2': {
			Player::self->toggleQuaterView();
		}
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
	player = new Player(cam);

	light = new Light();
	stage = new PhysicsObj();
	robotObj.push_back(new Robot());
	robotObj.push_back(new Robot());
	robotObj.push_back(new Robot());
	robotObj.push_back(new Robot());
	obstacle.push_back(new PhysicsObj());
	obstacle.push_back(new PhysicsObj());
	obstacle.push_back(new PhysicsObj());
	obstacle.push_back(new PhysicsObj());

	player->setPos(vec3(4, 0, 2));
	player->color = vec3(1, .5f, .5f);


	stage->loadObj("../model/cube.obj");
	stage->name = "stage";
	stage->setScale(vec3(stageSizeX, 1, stageSizeZ));
	stage->setPos(vec3(0, -3, 0));
	stage->setShader(&triShader);

	for (size_t i = 0; i < robotObj.size(); i++) {
		float randomPos = rand() % ((int)stageSizeX * 2) - stageSizeX;
		robotObj[i]->loadObj("../model/opengl_0118_robot/robot_body.obj");
		robotObj[i]->setShader(&triShader);
		robotObj[i]->color = randColor();
		robotObj[i]->setPos(vec3(randomPos, 0, randomPos));
		robotObj[i]->initParts();

		robotObj[i]->setBoxPhysics(1, vec3(0, -2, 0));
	}
	
	stage->setBoxPhysics(0, vec3(0), stage->getScale());
	player->setBoxPhysics(1, vec3(0, -2, 0));

	for (size_t i = 0; i < obstacle.size(); i++) {
		float randomPos = rand() % ((int)stageSizeX * 2) - stageSizeX;
		obstacle[i]->loadObj("../model/cube.obj");
		obstacle[i]->setPos(vec3(randomPos, 0, randomPos));
		obstacle[i]->setBoxPhysics(3);
		obstacle[i]->setShader(&triShader);
	}

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