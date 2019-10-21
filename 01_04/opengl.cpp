#include "../inc/mGlHeader.h"
#include <vector>

glm::vec3 t_vertex[] = {
	glm::vec3(0.5, .6, 0.0),
	glm::vec3(0.3, 0.0, 0.0),
	glm::vec3(0.7, 0.0, 0.0)
};

glm::vec2 t_vertexOffset[] = {
	glm::vec2(-1, 0),
	glm::vec2(-1, -1),
	glm::vec2(0, -1),
	glm::vec2(0, 0)
};

glm::vec3 t_vertex_color[] = {
	glm::vec3(1, .6, 0.0),
	glm::vec3(0, 1, 0.0),
	glm::vec3(0, 0.0, 1),
	glm::vec3(1, 0, 0.0)
};


glm::vec2 t_vertex_moveDirec[] = {
	glm::vec2(0, -1),
	glm::vec2(1, 0),
	glm::vec2(0, 1),
	glm::vec2(-1, 0) };

GLuint tri;
//GLuint VAO[4], VBO[4];
VO vo;
std::vector<glm::vec3> vertexList;
std::vector<glm::vec3> colorList;

bool isTimerEnd = false;

void init() {
	float tt[3 * 2 * 3 * 4] = { 0 };
	glm::vec3 t;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 3; j++) {
			t = t_vertex[j] + t_vertexOffset[i];
			vertexList.push_back(t);
			colorList.push_back(t_vertex_color[i]);
			vec3ToFloat(t,				tt + (6 * j) + (i * 6 * 3));
			vec3ToFloat(t_vertex_color[i],	tt + (6 * j) + (i * 6 * 3) + 3);
			//printf("%d %d \n", (6 * j) + (i * 6 * 3), (6 * j) + (i * 6 * 3) + 3);
		}
		
	}
	vo.create(tt, 3 * 3 * 4 * 2);

}
float checkTime = 0;
float direction = 1;
bool stop = false;
void loop() {
	if (stop) return;
	if (checkTime >= 4) {
		glm::vec2 tt(t_vertex_moveDirec[0]);
		for (int i = 0; i < 3; i++)
		{
			t_vertex_moveDirec[i] = t_vertex_moveDirec[i + 1];
		}
		t_vertex_moveDirec[3] = tt;

		checkTime = 0;
	}
	else if (direction == -1 && checkTime <= 0) {
		glm::vec2 tt(t_vertex_moveDirec[3]);
		for (int i = 2; i >= 0; i--)
		{
			t_vertex_moveDirec[i + 1] = t_vertex_moveDirec[i];
		}
		t_vertex_moveDirec[0] = tt;
		checkTime = 4;
	}
	for (size_t i = 0; i < 4; i++)
	{
		t_vertexOffset[i] += t_vertex_moveDirec[i] * 0.01f * direction;
		checkTime += glm::abs(t_vertex_moveDirec[i].x + t_vertex_moveDirec[i].y) * 0.01f * direction;
	}
	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < 3; i++)
		{
			vertexList[i + j * 3] = t_vertex[i] + t_vertexOffset[j];
		}
	}

	float tt[3 * 2 * 3 * 4] = { 0 };
	int ii = 0;
	for (auto i = vertexList.begin(), j = colorList.begin(); i != vertexList.end(); i++, j++, ii++)
	{
		vec3ToFloat(*i, tt + ii * 6);
		vec3ToFloat(*j, tt + ii * 6 + 3);
	}
	vo.change(tt, 3 * 2 * 3 * 4);

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
	tri = complieShader("tri");

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
	
	/*glColor3f(x, y, z);

	glRectf(x1, g_y1, x2, y2);*/

	glUseProgram(tri);
	glBindVertexArray(vo.VAO);
	glDrawArrays(GL_TRIANGLES, 0, 12);
	//glDrawArrays(GL_TRIANGLES, 6, 3 * 2);

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
	case 'f': glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
	case 'm': direction = -1; break;
	case 's': stop = true; break;
	case 'c': {
		for (size_t i = 0; i < colorList.size(); i+=3)
		{
			float randColor = rand() % 255 / 255.f;
			float randColor1 = rand() % 255 / 255.f;
			float randColor2 = rand() % 255 / 255.f ;
			colorList[i] = glm::vec3(randColor, randColor1, randColor2);
			colorList[i+1] = glm::vec3(randColor, randColor1, randColor2);
			colorList[i+2] = glm::vec3(randColor, randColor1, randColor2);
		}
		float tt[3 * 2 * 3 * 4] = { 0 };
		int ii = 0;
		for (auto i = vertexList.begin(), j = colorList.begin(); i != vertexList.end(); i++, j++, ii++)
		{
			vec3ToFloat(*i, tt + ii * 6);
			vec3ToFloat(*j, tt + ii * 6 + 3);
		}
		vo.change(tt, 3 * 2 * 3 * 4);
		break; }
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