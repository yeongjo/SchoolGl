#pragma once
#include "inc/mGlHeader2.h"
#include "../glm/gtx/string_cast.hpp"
#include <map>
#include <set>
#include <queue>


GLclampf f() {
	return rand() % 255 / 255.0f;
}

vec3 randColor() {
	return vec3(f(), f(), f());
}



class Window {
	static Window* active;
public:
	uint w, h;
	uint halfWidth, halfHeight;
	float ratio;

	Window() {
		active = this;
	}

	static Window& get() {
		return *active;
	}

	void init(int w, int h) {
		this->w = w;
		this->h = h;
		ratio = w / (float)h;
		halfWidth = w / 2, halfHeight = h / 2;
	}
};


Window* Window::active = nullptr;


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

	queue<TickObj*> removeObjs;
	static void removeObj(TickObj* obj) {
		Scene::self().removeObjs.push(obj);
	}
};


Scene* Scene::activeScene = nullptr;


class TickObj {
public:
	TickObj() { Scene::addObj(this); }
	virtual ~TickObj() {  }
	virtual void tick(float dt) = 0;
	virtual void render() = 0;
};


void Scene::tick(float dt) {
	for (set<TickObj*>::iterator it = objs.begin(); it != objs.end();++it) {
		(*it)->tick(dt);
	}
	while(Scene::self().removeObjs.size()) {
		auto t = removeObjs.front();
		objs.erase(t);
		delete t;
		removeObjs.pop();
	}
}

void Scene::render() {
	for (set<TickObj*>::iterator it = objs.begin(); it != objs.end(); ++it) {
		(*it)->render();
	}
}


class Camera;

class CameraShaderUniformBuffer {
public:
	unsigned int UBO;
	mat4* p, * v, * vp;
	vec3* pos;

	void create() {
		glGenBuffers(1, &UBO);
		glBindBuffer(GL_UNIFORM_BUFFER, UBO);
		//https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL
		glBufferData(GL_UNIFORM_BUFFER,
			sizeof(mat4) * 3 + sizeof(vec4),
			NULL, GL_STATIC_DRAW); // mat4 3개, vec3 1개 할당
	}

	void setData(Camera& cam, Window& win);

	// 값변경될때마다 한번만 호출하면되나
	void bindBuffer() {
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), p);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), v);
		glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), sizeof(glm::mat4), vp);
		glBufferSubData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4), sizeof(glm::vec3), pos); // 카메라 위치
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, UBO);
	}
};


class Camera {
public:
	glm::vec3 pos = glm::vec3(0, 0, -4);
	glm::vec3 target;
	glm::vec3 up = glm::vec3(0, 1, 0);
	float fov = 45;

	bool isPerspective = true;

	float rot_speed = 0;
	vec3 rot;
	vec3 armRot;
	float armLength = 1.5f;

	glm::mat4 v = glm::mat4(1.0f), p = glm::mat4(1.0f);
	glm::mat4 vp = glm::mat4(1.0f);

	static CameraShaderUniformBuffer *UBO;

	void init() {
		if (!UBO) {
			UBO = new CameraShaderUniformBuffer();
			UBO->create();
		}
	}

	virtual void tick(float dt) {
		bIsGetTransInThisTick = 0;
		bIsGetVPInThisTick = 0;
	}

	virtual void bind(Window& win) {
		UBO->setData(*this, win);
		UBO->bindBuffer();
	}

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
			if (isPerspective)
				p = glm::perspective(glm::radians(fov), win.ratio, 0.1f, 1000.f);
			else {
				float size = 5;
				p = glm::ortho<float>(-size * win.ratio, size * win.ratio, -size, size, 0.1, 50.f);
			}
			bIsGetVPInThisTick |= 0b1;
		}
		mat4 m(1);
		m = glm::rotate(m, glm::radians(rot.y), vec3(0, 1, 0));
		vp = p * m * v;
		bIsGetTransInThisTick = 1;
		return vp;
	}

	void translate(vec3 off) {
		vec3 right = glm::normalize(glm::cross(target- pos, up));
		vec3 forward = glm::normalize(glm::cross(up, right));
		vec3 moveOff = forward * off.z + right * off.x;
		pos += moveOff;
		target += moveOff;
	}

	void rotateArmY(float y) {
		armRot.y += y;
		pos.z = cos(De2Ra(armRot.y)) * armLength;
		pos.x = sin(De2Ra(armRot.y)) * armLength;
		pos.y = armLength * 0.3f;
	}

	void rotateY(float y) {
		rot.y += y;
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


CameraShaderUniformBuffer* Camera::UBO = nullptr;


void CameraShaderUniformBuffer::setData(Camera& cam, Window& win) {
	this->p = &cam.p;
	this->v = &cam.v;
	this->vp = &cam.getTrans(win);
}


class IShaderUniform {
public:
	virtual void setName(unsigned shaderIdx, const string& name) = 0;
	virtual void setValuePtr(void* ptr) = 0;
	virtual void setValueUniquePtr(void* ptr) = 0;
	virtual void setUniform(void* ptr = nullptr) = 0;
	virtual void log() = 0;
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
			if (ptr)
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

	void log() {
		debug("[%d]%s: %s", uniformLocation, uniformName.c_str(), glm::to_string(*valuePtr).c_str());
	}
};


template<>
void ShaderUniform<mat4>::setUniformWithType(void* ptr) {
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, (const float*)ptr);
}

template<>
void ShaderUniform<vec3>::setUniformWithType(void* ptr) {
	glUniform3fv(uniformLocation, 1, (GLfloat*)ptr);
}

//template<>
//void ShaderUniform<mat4>::log() {
//	debug("[%d]%s: %s", uniformLocation, uniformName, glm::to_string(*valuePtr));
//}
//
//template<>
//void ShaderUniform<vec3>::log() {
//	debug("[%d]%s: %s", uniformLocation, uniformName, glm::to_string(*valuePtr));
//}


class Shader {
public:
	unsigned int id;
	map<string, IShaderUniform*> shaderUniforms;
	string shaderPath;

	void complieShader(const char* shaderPath) {
		id = ::complieShader(shaderPath);
		this->shaderPath = shaderPath;
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
		if (uniform)
			uniform->setValuePtr(ptr);
	}

	void use() {
		glUseProgram(id);
		if (!shaderUniforms.empty())
			for (auto it = shaderUniforms.begin(); it != shaderUniforms.end(); it++) {
				it->second->setUniform();
			}
	}

	void logAllUniforms() {
		if (!shaderUniforms.empty())
			for (auto it = shaderUniforms.begin(); it != shaderUniforms.end(); it++) {
				it->second->log();
			}
		else
			debug("%s has no uniforms", shaderPath);
	}

	void logUniform(string name) {
		if (!shaderUniforms.empty())
			getUniform(name.c_str())->log();
		else
			debug("%s has no uniforms", shaderPath);
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



class Obj : public TickObj {
public:
	VO* vo;

	Obj* parentObj = nullptr;

	Shader* shader = nullptr;

	glm::mat4 trans = glm::mat4(1.0f);
	vec3 color = vec3(1,1,1);

	string name;


	// call after this have verteies
	void loadObj(const char* path) {
		vo = VO::loadObj(path);
		vo->bind();
	}

	void destroy() {
		Scene::removeObj(this);
	}

	vec3& getPos(){
		return pos;
	}

	vec3& getRotation(){
		return rot;
	}

	vec3& getScale(){
		return scale;
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

	void setShader(Shader* shader) {
		this->shader = shader;
		shader->addUniform("trans", &getTrans()); // TODO 낭비됨 계속 똑같은거 추가함
	}

	void rotateX(float x) {
		bIsGetTransInThisTick = false; rot.x += x;
	}
	void rotateY(float y) {
		bIsGetTransInThisTick = false; rot.y += y;
	}
	void rotateZ(float z) {
		bIsGetTransInThisTick = false; rot.z += z;
	}

	void updateTransform() {
		bIsGetTransInThisTick = false;
	}

	glm::mat4& getTrans() {
		if (bIsGetTransInThisTick) return trans;
		trans = glm::mat4(1.0f);
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
	}

	void applyColor(GLuint shaderId) {
		unsigned int loc = glGetUniformLocation(shaderId, "vcolor");
		glUniform3f(loc, color.x, color.y, color.z);
	}

	virtual void render() {
		if (shader) {
			shader->changeUniformValue("trans", &getTrans());
			shader->use();
			applyColor(shader->id);
		} else {
			assert(0); // shader 없음
		}
		vo->render();
	}

private:
	bool bIsGetTransInThisTick = false;
protected:
	vec3 pos = vec3(0);
	vec3 scale = vec3(1);
	vec3 rot = vec3(0);
};