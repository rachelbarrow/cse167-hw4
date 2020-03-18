﻿#include "Window.h"
#include <tgmath.h>


using namespace std;
using namespace glm;

namespace {
	int width, height;
	string windowTitle("MinecraftBlox");

	Robot* playerObject;
	Transform* objects;
	Terrain* terrain;
	TerrainGeometry* terrainGeometry;
	Cloud* cloud;
	PlantGeometry* plantGeometry;
	Transform* plant;


	vec3 playerCenter(0, 0, 250);
	vec3 eye(0, 1, 250);		// Camera position.
	vec3 center(0, 1, 255);		// The point we are looking at.
	vec3 up(0, 1, 0);			// The up direction of the camera.

	// movement flags
	bool holdingW = false, holdingA = false, holdingS = false, holdingD = false, sprinting = false;

	// for camera rotation
	float objYaw = -90.0f;
	float objPitch = 0.0f;

	float fovy = 60;
	float near_a = 0.5;
	float far_a = 2000;
	mat4 view = lookAt(eye, center, up);  // View matrix, defined by eye, center and up.
	mat4 projection;							// Projection matrix.

	GLuint program;
	GLuint skyboxProgram;
	GLuint cloudProgram;

	GLuint projectionLoc;	// Location of projection in shader.
	GLuint viewLoc;			// Location of view in shader.
	GLuint modelLoc;		// Location of model in shader.
	GLuint colorLoc;		// Location of color in shader.
	GLuint sphereColorLoc;
	GLuint modeLoc;
	GLuint dirEnabledLoc;
	GLuint pointEnabledLoc;
	GLuint ambientLoc;
	GLuint diffuseLoc;
	GLuint specularLoc;
	GLuint shininessLoc;

	GLuint pointLightPosLoc;
	GLuint pointLightColorLoc;
	GLuint directionLightDirLoc;
	GLuint directionLightColorLoc;
	GLuint viewPosLoc;

	// lights
	vec3 lightDir;
	vec3 pointLightPos;

	// mouse control
	bool mousePressed_L = false;
	bool noLastPoint = true;
	bool mousePressed_R = false;

	enum CameraType {
		FIRST_PERSON,
		THIRD_PERSON,
		FREE_CAMERA
	};
	CameraType cameraType = FIRST_PERSON;

	vec3 lastPoint;
	vec3 currPoint;

	enum ShaderMode {
		NORMAL,
		PHONG,
		TOON
	};
	ShaderMode mode = TOON; // shading mode

	bool cameraControl = true; // camera trackball mapping

	GLfloat SunLight_Ambient[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat SunLight_Diffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat SunLight_Specular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat SunLight_Position[4] = { 10.0f, 10.0f, 0.0f, 0.0f };

	//// skybox files
	//vector<string> faces = {
	//	"skybox/right.jpg",
	//	"skybox/left.jpg",
	//	"skybox/top.jpg",
	//	"skybox/bottom.jpg",
	//	"skybox/front.jpg",
	//	"skybox/back.jpg"
	//};

};

bool Window::initializeProgram() {
	// Create a shader program with a vertex shader and a fragment shader.
	program = LoadShaders("shaders/shader.vert", "shaders/shader.frag");
	//skyboxProgram = LoadShaders("shaders/skybox_shader.vert", "shaders/skybox_shader.frag");
	cloudProgram = LoadShaders("shaders/cloud_shader.vert", "shaders/cloud_shader.frag");

	// Check the shader programs.
	if (!program)
	{
		cerr << "Failed to initialize shader program" << endl;
		return false;
	}

	// Activate the shader program.
	glUseProgram(program);
	// Get the locations of uniform variables.
	projectionLoc = glGetUniformLocation(program, "projection");
	viewLoc = glGetUniformLocation(program, "view");
	modelLoc = glGetUniformLocation(program, "model");
	colorLoc = glGetUniformLocation(program, "normal");
	modeLoc = glGetUniformLocation(program, "mode");
	sphereColorLoc = glGetUniformLocation(program, "color");

	ambientLoc = glGetUniformLocation(program, "materialAmbient");
	diffuseLoc = glGetUniformLocation(program, "materialDiffuse");
	specularLoc = glGetUniformLocation(program, "materialSpecular");
	shininessLoc = glGetUniformLocation(program, "materialShininess");

	pointLightPosLoc = glGetUniformLocation(program, "pointLightPosition");
	pointLightColorLoc = glGetUniformLocation(program, "pointLightColor");
	directionLightDirLoc = glGetUniformLocation(program, "dirLightDirection");
	directionLightColorLoc = glGetUniformLocation(program, "dirLightColor");

	viewPosLoc = glGetUniformLocation(program, "viewPos");

	return true;
}

bool Window::initializeObjects() {

	printf("begin initializing objects >>>>");

	objects = new Transform(mat4(1));
	cloud = new Cloud();
	cloud->makeEntity(0, NULL, NULL, 80, 0, 0, 0, 0, 50);
	terrain = new Terrain(256, 256, 4.0f, 32, 32);

	createScene();

	playerCenter.y = terrain->GetPointHeight(playerCenter.x, playerCenter.z) + 1.75f;
	playerObject = new Robot(mat4(1));
	playerObject->translate(playerCenter.x, playerCenter.y, playerCenter.z);

	eye = playerObject->getEyePosition();
	center = eye;
	center.z += 5.0f;

	view = lookAt(eye, center, up);
	objects->addChild(playerObject);

	objects->addChild(terrainGeometry);

	plantGeometry = new PlantGeometry();
	plant = new Transform(mat4(1));

	//objects->addChild(plantGeometry);

	printf("done initializing objects >>>>");

	return true;
}

void Window::cleanUp() {

	delete(objects);
	delete(cloud);
	//delete(terrain);

	// Delete the shader programs.
	glDeleteProgram(program);
	glDeleteProgram(skyboxProgram);
	glDeleteProgram(cloudProgram);
}

GLFWwindow* Window::createWindow(int width, int height) {
	// Initialize GLFW.
	if (!glfwInit())
	{
		cerr << "Failed to initialize GLFW" << endl;
		return NULL;
	}

	// 4x antialiasing.
	glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__ 

	// Ensure that minimum OpenGL version is 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Enable forward compatibility and allow a modern OpenGL context
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// Create the GLFW window.
	GLFWwindow* window = glfwCreateWindow(width, height, windowTitle.c_str(), NULL, NULL);

	// Check if the window could not be created.
	if (!window)
	{
		cerr << "Failed to open GLFW window." << endl;
		glfwTerminate();
		return NULL;
	}

	// Make the context of the window.
	glfwMakeContextCurrent(window);

#ifndef __APPLE__
	// On Windows and Linux, we need GLEW to provide modern OpenGL functionality.

	// Initialize GLEW.
	if (glewInit())
	{
		cerr << "Failed to initialize GLEW" << endl;
		return NULL;
	}
#endif

	// Set swap interval to 1.
	glfwSwapInterval(0);

	// Call the resize callback to make sure things get drawn immediately.
	Window::resizeCallback(window, width, height);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	return window;
}

void Window::resizeCallback(GLFWwindow* window, int w, int h) {
#ifdef __APPLE__
	// In case your Mac has a retina display.
	glfwGetFramebufferSize(window, &width, &height);
#endif
	width = w;
	height = h;

	// Set the viewport size.
	glViewport(0, 0, width, height);

	// Set the projection matrix.
	projection = perspective(radians(fovy),
		(float)width / (float)height, near_a, far_a);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
}

void Window::idleCallback() {
	objects->update();
}

void Window::displayCallback(GLFWwindow* window) {

	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.4f, 0.4f, 0.0f);
	//glEnable(GL_DEPTH_TEST);
	handleMovement();

	// ------------CLOUD----------------
	glUseProgram(cloudProgram);
    glUniformMatrix4fv(glGetUniformLocation(cloudProgram, "P"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(cloudProgram, "V"), 1, GL_FALSE, glm::value_ptr(view));
    glUniform1f(glGetUniformLocation(cloudProgram, "time"), (float)glfwGetTime() * 0.2f - 0.0f);
	cloud->draw();

	// ----------- OBJECTS -------------
	glUseProgram(program);

	//mat4 model = currentObj->getModel();
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, value_ptr(projection));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
	glUniform1f(modeLoc, (float) mode);

	// direction lighting
	glUniform3fv(directionLightDirLoc, 1, glm::value_ptr(glm::vec3(0.0f, -1.0f, -15.0f)));
	glUniform3fv(directionLightColorLoc, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));

	glUniform3fv(viewPosLoc, 1, glm::value_ptr(eye));

	glUniform1f(dirEnabledLoc, 1);
	glUniform1f(pointEnabledLoc, 0);
	glUniform3fv(colorLoc, 1, glm::value_ptr(glm::vec3(1, 1, 1)));

	objects->draw(mat4(1), program, modelLoc, ambientLoc, diffuseLoc, specularLoc, shininessLoc);

	plant->draw(mat4(1), program, modelLoc, ambientLoc, diffuseLoc, specularLoc, shininessLoc);

	glfwPollEvents();
	glfwSwapBuffers(window);
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	// check for key press or release
	switch (key) {
		case GLFW_KEY_W:
			holdingW = action != GLFW_RELEASE;
			break;
		case GLFW_KEY_A:
			holdingA = action != GLFW_RELEASE;
			break;
		case GLFW_KEY_S:
			holdingS = action != GLFW_RELEASE;
			break;
		case GLFW_KEY_D:
			holdingD = action != GLFW_RELEASE;
			break;
		case GLFW_KEY_LEFT_CONTROL:
			sprinting = action != GLFW_RELEASE;
			break;
	}

	playerObject->setMoving((holdingW || holdingA || holdingS || holdingD) && cameraType != FREE_CAMERA);
	
	// Check for a key press.
	if (action == GLFW_PRESS)
	{
		// Uppercase key presses (shift held down + key press)
		if (mods == GLFW_MOD_SHIFT) {
			switch (key) {

			default:
				break;
			}
		}

		else {
			// Deals with lowercase key presses
			switch (key)
			{
				case GLFW_KEY_0:
					cameraControl = cameraControl == 0.0f ? 1.0f : 0.0f;
					break;

				case GLFW_KEY_F:
					cameraType = FREE_CAMERA;
					break;

				case GLFW_KEY_1:
					cameraType = FIRST_PERSON;
					break;

				case GLFW_KEY_3:
					cameraType = THIRD_PERSON;
					break;

				case GLFW_KEY_ESCAPE:
					glfwSetWindowShouldClose(window, GL_TRUE);
					break;

				default:
					break;
			}
		}
	}
}

vec3 Window::trackballMapping(double x, double y) {
	vec3 v;
	double d;

	v.x = (2 * x - width) / width;
	v.y = (height - 2 * y) / height;
	v.z = 0;

	d = length(v);
	d = d < 1.0 ? d : 1.0;
	v.z = sqrtf(1.001 - d * d);

	v = normalize(v);
	return v;
}

void Window::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) { 

	// for rotation 
	currPoint = trackballMapping(xpos, ypos);
	if (noLastPoint) {
		lastPoint = currPoint;
		noLastPoint = false;
		return;
	}

	vec3 pointDirection = currPoint - lastPoint;
	float velocity = length(vec3(pointDirection.x, 0, 0));
	if (velocity > 0.0001) {
		vec3 rotAxis = cross(lastPoint, currPoint);
		float rot_angle = velocity * 100.0f;

		if (cameraControl) {
			objYaw += pointDirection.x * 100;
			objPitch += pointDirection.y * 100;

			if (objPitch > 89.9) objPitch = 89.9;
			if (objPitch < -89.9) objPitch = -89.9;

			if (cameraType == FIRST_PERSON || cameraType == THIRD_PERSON) {
				objPitch = objPitch > 44.9 ? 44.9 : objPitch;
				objPitch = objPitch < -44.9 ? -44.9 : objPitch;
			}

			vec3 directionCamera;
			directionCamera.x = cos(radians(objYaw)) * cos(radians(objPitch));
			directionCamera.y = sin(radians(objPitch));
			directionCamera.z = sin(radians(objYaw)) * cos(radians(objPitch));
			directionCamera = normalize(directionCamera);

			up = normalize(cross(normalize(cross(directionCamera, vec3(0, 1, 0))), directionCamera));

			if (cameraType == FREE_CAMERA) center = eye + directionCamera;
			
			if (cameraType == FIRST_PERSON || cameraType == THIRD_PERSON) {
				mat4 prevModel = playerObject->getModel();
				mat4 newModel = rotate(mat4(1), radians(-rot_angle), vec3(0, rotAxis.y, 0));
				newModel = prevModel * newModel;
				playerObject->setModel(newModel);
				eye = playerObject->getEyePosition();
				center = eye + vec3(-directionCamera.x, directionCamera.y, -directionCamera.z);
				if (cameraType == THIRD_PERSON) {
					vec4 newEye = translate(mat4(1.0f), 15.0f * vec3(directionCamera.x, -directionCamera.y, directionCamera.z)) * vec4(eye, 1.0f);
					eye = vec3(newEye.x, newEye.y, newEye.z);
				}
			}
			
			view = lookAt(eye, center, up);
		}
	}
	glfwSetCursorPos(window, height/2, width/2);
	noLastPoint = 1;
	//lastPoint = vec3(0, 0, 0); //currPoint;

	// for translation in xy plane
	if (mousePressed_R) {
		vec3 currPoint = vec3(xpos / 15, -1 * ypos / 15, 0);
		if (noLastPoint) {
			lastPoint = currPoint;
			noLastPoint = false;
			return;
		}

		vec3 difference = currPoint - lastPoint;

		objects->translate(difference.x, difference.y, 0);

		lastPoint = currPoint;
	}
}

void Window::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		mousePressed_R = true;
		noLastPoint = true;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT) mousePressed_R = false;
	
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		mousePressed_L = true;
		noLastPoint = true;
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT) mousePressed_L = false;
}

void Window::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {

}

vec3 Window::checkInSkybox(vec4 point) {
	if (cameraType == FREE_CAMERA) {
		if (point.x > 95 || point.x < -95) {
			point.x = eye.x;
		}
		if (point.y > 95 || point.y < -95) {
			point.y = eye.y;
		}
		if (point.z > 95 || point.z < -95) {
			point.z = eye.z;
		}
	} 

	return vec3(point.x, point.y, point.z);
}

bool checkCollision(vec3 objOne, vec3 objTwo){
	if(playerObject->maxX >= 100 && playerObject->minX <= -100 &&
	playerObject->maxY >= 100 && playerObject->minY <= -100 &&
	playerObject->maxZ >= 100 && playerObject->minZ <= -100 ){
		//currentPosition = previews
	}
	return false;
}

void Window::move(vec3 translation) {
	
	vec4 newCenter;
	vec4 newEye;

	if (cameraType == FREE_CAMERA) {

		newEye = translate(mat4(1), translation) * vec4(eye, 1);
		eye = checkInSkybox(newEye);
		newCenter = translate(mat4(1), translation) * vec4(center, 1);
		center = vec3(newCenter.x, newCenter.y, newCenter.z);
	}
	
	if (cameraType != FREE_CAMERA) {
		playerObject->translate(translation.x, -playerObject->currentY, translation.z);
		playerObject->translate(0, terrain->GetPointHeight(playerObject->currentX, playerObject->currentZ) + 1.75f, 0);

		vec3 cameraDirection = normalize(center - eye);
		eye = playerObject->getEyePosition();
		newCenter = translate(mat4(1), cameraDirection) * vec4(eye, 1);
		center = vec3(newCenter.x, newCenter.y, newCenter.z);

		if (cameraType == THIRD_PERSON) {
			newEye = translate(mat4(1), -15.0f * vec3(cameraDirection.x, cameraDirection.y, cameraDirection.z)) * vec4(eye, 1);
			eye = vec3(newEye.x, newEye.y, newEye.z);
		}
	}

	view = lookAt(eye, center, up);
}

void Window::handleMovement() {

	float speedModifier = sprinting ? 10.00f : 5.0f;
	vec3 direction = 0.01f * speedModifier * normalize((vec3(center.x, 0, center.z) - vec3(eye.x, 0, eye.z)));
	playerObject->animationSpeed = sprinting ? 3.0f : 0.2f;

	// forward
	if (holdingW) {
		vec3 translation = direction;
		move(translation);
	}

	// backward
	else if (holdingS) {
		vec3 translation = -direction;
		move(translation);
	}

	// right
	if (holdingA) {
		vec3 translation = cross(-direction, up);
		move(translation);
	}

	// left
	else if (holdingD) {
		vec3 translation = cross(direction, up);
		move(translation);
	}
}

void Window::createScene() {
	GLuint i;
	float height;
	vec3 terrainsize, norm;

	srand(20202);

	terrain->reset();
	terrain->fault(90, 4.0f, 0.999f);
	terrain->randomNoise(3.0f);
	terrain->smooth(1, 1);

	vector<vec3> positions;
	vector<vec3> normals;
	vector<vec3> textures;
	vector<unsigned int> faces;

	float maxX = -10000.0f, maxY = -10000.0f, maxZ = -10000.0f;
	float minX = 10000.0f, minY = 10000.0f, minZ = 10000.0f;

	VertexStr* vertices = terrain->getVertices();
	for (int i = 0; i < terrain->m_iWidth; i++) {
		for (int j = 0; j < terrain->m_iHeight; j++) {
			VertexStr currVertex = vertices[j + i * terrain->m_iWidth];

			// gets max/min
			if (currVertex.position.x > maxX) maxX = currVertex.position.x;
			if (currVertex.position.x < minX) minX = currVertex.position.x;
			if (currVertex.position.y > maxY) maxY = currVertex.position.y;
			if (currVertex.position.y < minY) minY = currVertex.position.y;
			if (currVertex.position.z > maxZ) maxZ = currVertex.position.z;
			if (currVertex.position.z < minZ) minZ = currVertex.position.z;

			positions.push_back(currVertex.position);
			normals.push_back(currVertex.normal);
			textures.push_back(currVertex.texture);
			//printf("\nVertex (%d,%d)\n", i, j);
			//printf("position: [%f, %f, %f]\n", currVertex.position.x, currVertex.position.y, currVertex.position.z);
			//printf("normal: [%f, %f, %f]\n", currVertex.normal.x, currVertex.normal.y, currVertex.normal.z);
			//printf("texture: [%f, %f, %f]\n\n", currVertex.texture.x, currVertex.texture.y, currVertex.texture.z);
		}
	}

	float centerX = (float)((double)maxX + minX) / 2.0;
	float centerY = (float)((double)maxY + minY) / 2.0;
	float centerZ = (float)((double)maxZ + minZ) / 2.0;

	// center points
	/*for (int i = 0; i < positions.size(); i++) {
		positions.at(i) = glm::vec3(positions.at(i).x - centerX, positions.at(i).y - centerY, positions.at(i).z - centerZ);
	}*/

	SubsetStr* subsets = terrain->m_pSubsets;
	for (int i = 0; i < subsets->NumTriangles; i++) {
		faces.push_back(subsets->pIndices[i]);
	}

	terrainGeometry = new TerrainGeometry(positions, normals, textures, faces);
	//terrainGeometry->addTextures(terrain->m_hTex);

	terrainsize = terrain->getGridSize();
	height = terrain->getMedianHeight();
}
