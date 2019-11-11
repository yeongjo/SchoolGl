#include "inc/mGlHeader2.h"

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

class TickObj {
	virtual void tick(float dt) = 0;
};

class Camera : public TickObj {
public:
	glm::vec3 pos = glm::vec3(0, 0, -1);
	glm::vec3 target;
	glm::vec3 up = glm::vec3(0, 1, 0);
	float fov = 45;

	float rot_speed = 0;
	float rot_y;
	float arm_length = 3;

	glm::mat4 v = glm::mat4(1.0f), p = glm::mat4(1.0f);
	glm::mat4 vp = glm::mat4(1.0f);

	virtual void tick(float dt) {
		bIsGetTransInThisTick = 0;
		bIsGetVPInThisTick = 0;

		rotateZ(rot_speed * dt);
	}

	glm::mat4& getTrans(Window *win) {
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
			p = glm::perspective(glm::radians(fov), win->ratio, 0.1f, 100.f);
			bIsGetVPInThisTick |= 0b1;
		}

		vp = p * v;
		bIsGetTransInThisTick = 1;
		return vp;
	}

	void rotateZ(float y) {
		rot_y += y;
		pos.z = cos(De2Ra(rot_y)) * arm_length;
		pos.x = sin(De2Ra(rot_y)) * arm_length;
		pos.y = 0.3f;
	}

private:
	uint bIsGetTransInThisTick = 0;
	uint bIsGetVPInThisTick = 0;
};

class Obj : public TickObj {
public:
	VO *vo;

	glm::vec3 pos;
	vec3 scale = vec3(1);
	vec3 rot;

	glm::mat4 trans = glm::mat4(1.0f);
	vec3 color = vec3(0,1,0);

	float scale_speed = 0.1f;
	float value = -1;
	float speed = 0.3f;

	// call after this have verteies
	void loadObj(const char *path) {
		vo = VO::loadObj(path);
		vo->bind();
	}

	void destroy() {
	}

	void setPos(const vec3& p) {
		pos = p;
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
		bIsGetTransInThisTick = true;
		return trans;
	}

	virtual void tick(float dt) {
		bIsGetTransInThisTick = false;
		scale.x = scale.y = scale.z += dt * scale_speed;
		if (scale.x > 1 || scale.x < 0.1) {
			scale_speed = -scale_speed;
		}
		value += dt * speed;
		if (value > 1)
			value -= 2;
		pos = getShapeLinesPos(render_line_idx, value);
	}

	void setColor(GLuint tri_shader) {
		unsigned int loc = glGetUniformLocation(tri_shader, "vcolor");
		glUniform3f(loc, color.x, color.y, color.z);
	}

	virtual void render() {
		vo->render();
	}

private:
	bool bIsGetTransInThisTick = false;

};

Window win;
Camera cam;
GLuint tri_shader, grid_shader;
Obj objs[5];
Obj tri;

VO gridVO;

GLUquadricObj* qobj;

bool isTimerEnd = false;

// -1 < z < 1
// case 0: 싸인
// case 1: 스프링
// case 2: 지그재그
vec3 getShapeLinesPos(int type, float z) {
	switch (type) {
	case 0:
	{
		float t = z * 180.f * 3;
		return vec3(0, sin(De2Ra(t)), z);
	}
		break;
	case 1:
	{
		float t = z * 180.f * 3;
		return vec3(cos(De2Ra(t)), sin(De2Ra(t)), z);
	}
		break;
	case 2:
	{
		float t = z * 180.f * 0.03f;
		float div_num = 2;
		float tem_remainder = remainder(t, div_num);
		//float y = (int)(t / div_num) ? tem_remainder : -tem_remainder;
		float y = abs(tem_remainder);
		return vec3(0, y, z);
	}
		break;
	}
}

int render_model_idx = 0;

void init() {

	//for (size_t i = 0; i < 6; i++)
	//{
	//	rect.vertexList.push_back(t_vertex_rect[i]);
	//}
	//rect.init();
	//rect.color = vec3(1, 1, 1);
	//rect.scale = vec3(3);
	glLineWidth(3);
	qobj = gluNewQuadric(); // 객체 생성하기
	gluQuadricDrawStyle(qobj, GLU_LINE); // 도형 스타일

	for (size_t i = 0; i < 3; i++) {
		objs[i].vo = new VO();
		objs[i].vo->drawStyle = GL_LINE_STRIP;
		for (int j = 0; j < 300; j++) {
			objs[i].vo->vertex.push_back(getShapeLinesPos(i, (j / 150.f) - 1));
		}
		objs[i].vo->bind();
	}

	cam.rotateZ(10);

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
		prevTime = GetTickCount() - 10;
	}
	thisTickTime = GetTickCount();
	dt = (thisTickTime - prevTime) * 0.001f;
	dt = dt > 0.05f ? dt = 0.05f : dt;
	prevTime = thisTickTime;

	cam.tick(dt);
	objs[3].tick(dt);
	objs[4].tick(dt);

	glutPostRedisplay();
}

GLvoid drawScene() // 콜백 함수: 출력
{
	//printf("drawScene\n");
	glClearColor(0, 0, 0, 1.0f); // 바탕색을 ‘blue’로 지정
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 설정된 색으로 전체를 칠하기

	glUseProgram(tri_shader);
	unsigned int vpLoc = glGetUniformLocation(tri_shader, "vp");
	glUniformMatrix4fv(vpLoc, 1, GL_FALSE, glm::value_ptr(cam.getTrans(&win)));
	unsigned int transformLoc = glGetUniformLocation(tri_shader, "trans");
	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(objs[0].getTrans()));

	objs[render_line_idx].render();

	transformLoc = glGetUniformLocation(tri_shader, "trans");
	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(objs[3].getTrans()));
	if(render_model_idx)
		gluSphere(qobj, 1.0, 6, 4);
	else
		gluCylinder(qobj, 1.0, 0.0, 0.0, 4, 1);

	glUseProgram(grid_shader);
	transformLoc = glGetUniformLocation(tri_shader, "trans");
	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(mat4(1)));
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
void Mouse(int button, int state, int x, int y);
void specialInput(int key, int x, int y);
void keyboardUp(unsigned char key, int x, int y);

void main(int argc, char** argv) // 윈도우 출력하고 콜백함수 설정 
{ //--- 윈도우 생성하기
	glutInit(&argc, argv); // glut 초기화
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // 디스플레이 모드 설정
	win.init(800, 600);
	glutInitWindowPosition(1920, 0); // 윈도우의 위치 지정
	glutInitWindowSize(win.w, win.h); // 윈도우의 크기 지정
	glutCreateWindow("Example1"); // 윈도우 생성(윈도우 이름)

	//glEnable(GL_DEPTH_TEST);
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
	grid_shader = complieShader("grid");

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
void Keyboard(unsigned char key, int x, int y)
{
	printf("Keyboard down: %c [%d, %d]\n", key, x, y);
	
	switch (key) {
	case '1':
		render_line_idx = 0;
		break;
	case '2':
		render_line_idx = 1;
		break;
	case '3':
		render_line_idx = 2;
		break;
	case '4':
		render_model_idx = 0;
		break;
	case '5':
		render_model_idx = 1;
	break;
	case '6': {
		rot_speed = -rot_speed;
		cam.rot_speed = rot_speed;
	}
		break;
	case 'q': isTimerEnd = true;  glutLeaveMainLoop(); break;
	}
}

void keyboardUp(unsigned char key, int x, int y) {
	printf("Keyboard up: %c [%d, %d]\n", key, x, y);

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