/*
 * Example two meshes and two shaders (could also be used for Program 2)
 * includes modifications to shape and initGeom in preparation to load
 * multi shape objects 
 * CPE 471 Cal Poly Z. Wood + S. Sueda + I. Dunn
 */

#include <iostream>
#include <glad/glad.h>
#include <chrono>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Texture.h"
#include <cfloat>
#include <algorithm>
#include "stb_image.h"
#include "Spline.h"
#include "Bezier.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

class Application : public EventCallbacks {

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> prog;

	std::shared_ptr<Program> lightingProg;

	float lightTrans = 0;	
	vec3 lightPos = glm::vec3(-2.0f, 2.0f, 2.0f);
	bool useAltMaterial = false;

	shared_ptr<Texture> grassTex;
	shared_ptr<Texture> doghouseTex;
	shared_ptr<Texture> cribTex;
	shared_ptr<Texture> shedTex;
	shared_ptr<Texture> bedTex;
	shared_ptr<Program> texProg;
	shared_ptr<Program> skyboxProg;

	GLuint skyboxTex;
	GLuint skyboxVAO;
	GLuint skyboxVBO;

	// Our shader program
	std::shared_ptr<Program> solidColorProg;

	// Shape to be used (from  file) - modify to support multiple
	shared_ptr<Shape> mesh;

	//a different mesh
	//shared_ptr<Shape> bunny;

	// My bunny forest
	struct BunnyInstance {
    glm::vec3 pos;
    glm::vec3 rot;
    float scale;
    int materialID;
	};

	std::vector<BunnyInstance> bunnyForest;

	// float yaw   = -1.5f; // horizontal rotation
	// float pitch = 0.0f; // vertical rotation

	float yaw   = 3.2f; // horizontal rotation
	float pitch = -0.30f; // vertical rotation

	float radius = 5.0f; // distance from camera to lookAt point
	glm::vec3 camPos = glm::vec3(5, 0.0, 4.0);

	glm::vec3 camFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 camRight = glm::vec3(1.0f, 0.0f, 0.0f);

	Spline splinepath[2];
	bool goCamera = false;

	glm::vec3 g_eye = glm::vec3(0, 1, 0); // starting camera position
	glm::vec3 g_lookAt = glm::vec3(0, 1, -4); // fixed look-at point for tour
	float lastTimeCam = 0.0f;

	// meshes struct
	struct MeshSet {
		std::vector<std::shared_ptr<Shape>> parts;
		glm::vec3 bbMin = glm::vec3(FLT_MAX);
		glm::vec3 bbMax = glm::vec3(-FLT_MAX);
		glm::mat4 N = glm::mat4(1.0f);
	};

	MeshSet table, hammer, plank, doghouse, dog, nail, bowl, plane, bunny, crib, bed, shed, shop;

	float gSceneAngleY = 0.2f;
	bool gAnimate = false;

	// Dog animation state
	glm::vec3 dogStart = glm::vec3(-6.5f, -0.50f, 0.0f);
	glm::vec3 dogEnd   = glm::vec3(-3.0f, -0.50f, 0.0f);

	//example data that might be useful when trying to compute bounds on multi-shape
	vec3 gMin;

	//animation data
	float sTheta = 0;
	float gTrans = 0;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		// if (key == GLFW_KEY_A && action == GLFW_PRESS) {
		// 	gSceneAngleY += 0.05f;
		// }
		// if (key == GLFW_KEY_D && action == GLFW_PRESS) {
		// 	gSceneAngleY -= 0.05f;
		// }

		if (action == GLFW_PRESS || action == GLFW_REPEAT) {
			float speed = 0.2f;

			if (key == GLFW_KEY_W) camPos += camFront * speed;
			if (key == GLFW_KEY_S) camPos -= camFront * speed;
			if (key == GLFW_KEY_A) camPos -= camRight * speed;
			if (key == GLFW_KEY_D) camPos += camRight * speed;
		}

		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		gAnimate = !gAnimate;

		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}

		if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
			lightTrans += 0.25;
		}
		if (key == GLFW_KEY_E && action == GLFW_PRESS) {
			lightTrans -= 0.25;
		}

		if (key == GLFW_KEY_M && action == GLFW_PRESS) {
			useAltMaterial = !useAltMaterial;
		}

		if (key == GLFW_KEY_G && action == GLFW_PRESS) {
			goCamera = !goCamera;
			lastTimeCam = glfwGetTime();

			splinepath[0] = Spline(
				glm::vec3(-6,1.5,7),
				glm::vec3(-1,0,7),
				glm::vec3(1,2,7),
				glm::vec3(2,1.5,7),
				5.0f
			);

			splinepath[1] = Spline(
				glm::vec3(2,1.5,7),
				glm::vec3(3,0.5,7),
				glm::vec3(-0.25, 0.25, 7),
				glm::vec3(0,0,7),
				5.0f
			);
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			 glfwGetCursorPos(window, &posX, &posY);
			 cout << "Pos X " << posX <<  " Pos Y " << posY << endl;
		}
	}

	void scrollCallback(GLFWwindow* window, double deltaX, double deltaY) override 
	{
    float sens = 0.05f;

    yaw   -= deltaX * sens;
    pitch += deltaY * sens;

    // clamp pitch so camera never flips
    float maxPitch = glm::radians(80.0f);
    if (pitch > maxPitch) pitch = maxPitch;
    if (pitch < -maxPitch) pitch = -maxPitch;
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	GLuint loadCubemap(vector<string> faces) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0, GL_RGB,
                         GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            cout << "Failed to load: " << faces[i] << endl;
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
	}

	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(.12f, .34f, .56f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program.
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");

		// Initialize the GLSL program.
		solidColorProg = make_shared<Program>();
		solidColorProg->setVerbose(true);
		solidColorProg->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/solid_frag.glsl");
		solidColorProg->init();
		solidColorProg->addUniform("P");
		solidColorProg->addUniform("V");
		solidColorProg->addUniform("M");
		solidColorProg->addUniform("solidColor");
		solidColorProg->addAttribute("vertPos");
		solidColorProg->addAttribute("vertNor");

		lightingProg = make_shared<Program>();
		lightingProg->setVerbose(true);
		lightingProg->setShaderNames(resourceDirectory + "/phong_vert.glsl", resourceDirectory + "/phong_frag.glsl");
		lightingProg->init();
		lightingProg->addUniform("P");
		lightingProg->addUniform("V");
		lightingProg->addUniform("M");
		lightingProg->addUniform("lightPos");
		lightingProg->addUniform("MatAmb");
		lightingProg->addUniform("MatDif");
		lightingProg->addUniform("MatSpec");
		lightingProg->addUniform("MatShine");
		lightingProg->addUniform("LightColor");
		lightingProg->addAttribute("vertPos");
		lightingProg->addAttribute("vertNor");

		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl",
								resourceDirectory + "/tex_frag.glsl");
		texProg->init();
		texProg->addUniform("P");
		texProg->addUniform("V");
		texProg->addUniform("M");
		texProg->addUniform("Texture0");
		texProg->addUniform("lightPos");
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex");

		skyboxProg = make_shared<Program>();
		skyboxProg->setVerbose(true);
		skyboxProg->setShaderNames(
			resourceDirectory + "/cube_vert.glsl", 
			resourceDirectory + "/cube_frag.glsl"
		);
		skyboxProg->init();
		skyboxProg->addUniform("P");
		skyboxProg->addUniform("V");
		skyboxProg->addUniform("M");
		skyboxProg->addUniform("skybox");

		// Skybox
		skyboxVAO = 0;
		skyboxVBO = 0;

		float skyboxVertices[] = {
			// Each vertex: position (x,y,z) + dummy normal (nx,ny,nz)
			// 6 floats per vertex, 36 vertices total

			// Back face
			-1.0f, -1.0f, -1.0f, 0.f,0.f,0.f,
			1.0f, -1.0f, -1.0f, 0.f,0.f,0.f,
			1.0f,  1.0f, -1.0f, 0.f,0.f,0.f,
			1.0f,  1.0f, -1.0f, 0.f,0.f,0.f,
			-1.0f,  1.0f, -1.0f, 0.f,0.f,0.f,
			-1.0f, -1.0f, -1.0f, 0.f,0.f,0.f,

			// Front face
			-1.0f, -1.0f,  1.0f, 0.f,0.f,0.f,
			1.0f, -1.0f,  1.0f, 0.f,0.f,0.f,
			1.0f,  1.0f,  1.0f, 0.f,0.f,0.f,
			1.0f,  1.0f,  1.0f, 0.f,0.f,0.f,
			-1.0f,  1.0f,  1.0f, 0.f,0.f,0.f,
			-1.0f, -1.0f,  1.0f, 0.f,0.f,0.f,

			// Left face
			-1.0f,  1.0f,  1.0f, 0.f,0.f,0.f,
			-1.0f,  1.0f, -1.0f, 0.f,0.f,0.f,
			-1.0f, -1.0f, -1.0f, 0.f,0.f,0.f,
			-1.0f, -1.0f, -1.0f, 0.f,0.f,0.f,
			-1.0f, -1.0f,  1.0f, 0.f,0.f,0.f,
			-1.0f,  1.0f,  1.0f, 0.f,0.f,0.f,

			// Right face
			1.0f,  1.0f,  1.0f, 0.f,0.f,0.f,
			1.0f,  1.0f, -1.0f, 0.f,0.f,0.f,
			1.0f, -1.0f, -1.0f, 0.f,0.f,0.f,
			1.0f, -1.0f, -1.0f, 0.f,0.f,0.f,
			1.0f, -1.0f,  1.0f, 0.f,0.f,0.f,
			1.0f,  1.0f,  1.0f, 0.f,0.f,0.f,

			// Bottom face
			-1.0f, -1.0f, -1.0f, 0.f,0.f,0.f,
			1.0f, -1.0f, -1.0f, 0.f,0.f,0.f,
			1.0f, -1.0f,  1.0f, 0.f,0.f,0.f,
			1.0f, -1.0f,  1.0f, 0.f,0.f,0.f,
			-1.0f, -1.0f,  1.0f, 0.f,0.f,0.f,
			-1.0f, -1.0f, -1.0f, 0.f,0.f,0.f,

			// Top face
			-1.0f,  1.0f, -1.0f, 0.f,0.f,0.f,
			1.0f,  1.0f, -1.0f, 0.f,0.f,0.f,
			1.0f,  1.0f,  1.0f, 0.f,0.f,0.f,
			1.0f,  1.0f,  1.0f, 0.f,0.f,0.f,
			-1.0f,  1.0f,  1.0f, 0.f,0.f,0.f,
			-1.0f,  1.0f, -1.0f, 0.f,0.f,0.f
		};

		glGenVertexArrays(1, &skyboxVAO);
		glGenBuffers(1, &skyboxVBO);
		glBindVertexArray(skyboxVAO);
		glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

		// vertPos = location 0
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);

		// vertNor = location 1 (dummy)
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));

		glBindVertexArray(0);

		vector<string> faces {
		resourceDirectory + "/right.jpg",
		resourceDirectory + "/left.jpg",
		resourceDirectory + "/top.jpg",
		resourceDirectory + "/bottom.jpg",
		resourceDirectory + "/front.jpg",
		resourceDirectory + "/back.jpg",
		};

		skyboxTex = loadCubemap(faces);

		splinepath[0] = Spline(glm::vec3(-6,0,5),glm::vec3(-1,-5,5),glm::vec3(1, 5, 5),glm::vec3(2,0,5),5.0f);
		splinepath[1] = Spline(glm::vec3(2,0,5),glm::vec3(3,-2,5),glm::vec3(-0.25, 0.25, 5),glm::vec3(0,0,5),5.0f);
	}

	void initGeom(const std::string& resourceDirectory)
	{
		table = loadOBJAsMeshSet(resourceDirectory + "/table.obj");
		hammer = loadOBJAsMeshSet(resourceDirectory + "/hammer.obj");
		plank = loadOBJAsMeshSet(resourceDirectory + "/woodplank.obj");
		doghouse = loadOBJAsMeshSet(resourceDirectory + "/doghouse.obj");
		dog = loadOBJAsMeshSet(resourceDirectory + "/dog.obj");
		nail = loadOBJAsMeshSet(resourceDirectory + "/nail.obj");
		bowl = loadOBJAsMeshSet(resourceDirectory + "/bowl.obj");
		plane = loadOBJAsMeshSet(resourceDirectory + "/plane.obj");
		bunny = loadOBJAsMeshSet(resourceDirectory + "/phorium.obj");
		crib = loadOBJAsMeshSet(resourceDirectory + "/cribuv.obj");
		bed = loadOBJAsMeshSet(resourceDirectory + "/bed.obj");
		shed = loadOBJAsMeshSet(resourceDirectory + "/shed.obj");
		shop = loadOBJAsMeshSet(resourceDirectory + "/shop1.obj");
	}

	void initTex(const string &resourceDirectory)
	{
		grassTex = make_shared<Texture>();
		grassTex->setFilename(resourceDirectory + "/grass.jpg"); 
		grassTex->init();
		grassTex->setUnit(0);
		grassTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		doghouseTex = make_shared<Texture>();
		doghouseTex->setFilename(resourceDirectory + "/woodtex.jpg");
		doghouseTex->init();
		doghouseTex->setUnit(0);
		doghouseTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		cribTex = make_shared<Texture>();
		cribTex->setFilename(resourceDirectory + "/pine.jpg");
		cribTex->init();
		cribTex->setUnit(0);
		cribTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		bedTex = make_shared<Texture>();
		bedTex->setFilename(resourceDirectory + "/walnut.jpg");
		bedTex->init();
		bedTex->setUnit(0);
		bedTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		shedTex = make_shared<Texture>();
		shedTex->setFilename(resourceDirectory + "/oak.jpg");
		shedTex->init();
		shedTex->setUnit(0);
		shedTex->setWrapModes(GL_REPEAT, GL_REPEAT);
	}

	// random bunny forest lmao
	void generateBunnyForest() {
    int N = 15;  
    bunnyForest.reserve(N);

    for (int i = 0; i < N; i++) {
        BunnyInstance b;

        // random position
        b.pos = glm::vec3(
            (rand() % 200 - 100) / 10.0f,
            -2.3f,
            -5.0f - (rand() % 100) / 10.0f
        );

        b.rot = glm::vec3(0, glm::radians((float)(rand() % 360)),0);

        b.scale = 0.45f + (rand() % 40) / 100.0f;
        b.materialID = rand() % 10;

        bunnyForest.push_back(b);
    }
	}

	/* helper for sending top of the matrix strack to GPU */
	void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M) {
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
   }

	/* helper function to set model trasnforms */
  	void setModel(shared_ptr<Program> curS, vec3 trans, float rotY, float rotX, float sc) {
  		mat4 Trans = glm::translate( glm::mat4(1.0f), trans);
  		mat4 RotX = glm::rotate( glm::mat4(1.0f), rotX, vec3(1, 0, 0));
  		mat4 RotY = glm::rotate( glm::mat4(1.0f), rotY, vec3(0, 1, 0));
  		mat4 ScaleS = glm::scale(glm::mat4(1.0f), vec3(sc));
  		mat4 ctm = Trans*RotX*RotY*ScaleS;
  		glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
  	}

	MeshSet loadOBJAsMeshSet(const std::string &path){
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string errStr;

		bool rc = tinyobj::LoadObj(shapes, materials, errStr, path.c_str());
		if (!rc) {
			cerr << errStr << endl;
			throw std::runtime_error("Failed to load OBJ: " + path);
		}

		MeshSet ms;
		ms.parts.reserve(shapes.size());

		for (auto &s : shapes) {
			// i only want texture for my ground plane
			bool tex = false;
			if (path == "../resources/plane.obj" || 
				path == "../resources/doghouse.obj" ||
				path == "../resources/woodplank.obj" ||
				path == "../resources/cribuv.obj" ||
				path == "../resources/bed.obj" ||
				path == "../resources/shed.obj"){
				tex = true;
			}
			auto shp = make_shared<Shape>(tex);
			shp->createShape(s);
			shp->measure();
			shp->init();
			ms.parts.push_back(shp);
			ms.bbMin = glm::min(ms.bbMin, shp->min);
			ms.bbMax = glm::max(ms.bbMax, shp->max);
		}

		glm::vec3 center = 0.5f * (ms.bbMin + ms.bbMax);
		glm::vec3 extent = ms.bbMax - ms.bbMin;
		float largest = std::max(extent.x, std::max(extent.y, extent.z));
		float s = (largest > 0.0f) ? (2.0f / largest) : 1.0f;
		ms.N = glm::scale(glm::mat4(1.0f), glm::vec3(s)) *
			glm::translate(glm::mat4(1.0f), -center);
		return ms;
	}

	void drawMeshSet(std::shared_ptr<Program> prog, const MeshSet &ms){
		for (const auto &p : ms.parts)
			p->draw(prog);
	}

	void SetMaterial(shared_ptr<Program> curS, int i)
	{
    switch (i)
    {
    case 0: // red plastic
        glUniform3f(curS->getUniform("MatAmb"), 0.1, 0.1, 0.1);
        glUniform3f(curS->getUniform("MatDif"), 0.8, 0.1, 0.1);
        glUniform3f(curS->getUniform("MatSpec"), 0.8, 0.8, 0.8);
        glUniform1f(curS->getUniform("MatShine"), 32.0);
        break;

    case 1: // green plastic
        glUniform3f(curS->getUniform("MatAmb"), 0.1, 0.1, 0.1);
        glUniform3f(curS->getUniform("MatDif"), 0.1, 0.8, 0.1);
        glUniform3f(curS->getUniform("MatSpec"), 0.8, 0.8, 0.8);
        glUniform1f(curS->getUniform("MatShine"), 16.0);
        break;

    case 2: // blue plastic
        glUniform3f(curS->getUniform("MatAmb"), 0.05, 0.05, 0.1);
        glUniform3f(curS->getUniform("MatDif"), 0.1, 0.1, 0.8);
        glUniform3f(curS->getUniform("MatSpec"), 1.0, 1.0, 1.0);
        glUniform1f(curS->getUniform("MatShine"), 64.0);
        break;

    case 3: // wood
        glUniform3f(curS->getUniform("MatAmb"), 0.20f, 0.12f, 0.05f);
        glUniform3f(curS->getUniform("MatDif"), 0.54f, 0.36f, 0.21f);
        glUniform3f(curS->getUniform("MatSpec"), 0.08f, 0.05f, 0.04f);
        glUniform1f(curS->getUniform("MatShine"), 8.0f);
        break;

    case 4: //more metal
        glUniform3f(curS->getUniform("MatAmb"), 0.25f, 0.25f, 0.28f);
        glUniform3f(curS->getUniform("MatDif"), 0.40f, 0.40f, 0.45f);
        glUniform3f(curS->getUniform("MatSpec"), 0.77f, 0.77f, 0.78f);
        glUniform1f(curS->getUniform("MatShine"), 90.0f);
        break;

    case 5: // stone
        glUniform3f(curS->getUniform("MatAmb"), 0.12f, 0.12f, 0.12f);
        glUniform3f(curS->getUniform("MatDif"), 0.35f, 0.35f, 0.35f);
        glUniform3f(curS->getUniform("MatSpec"), 0.05f, 0.05f, 0.05f);
        glUniform1f(curS->getUniform("MatShine"), 5.0f);
        break;

    case 6: // goold
        glUniform3f(curS->getUniform("MatAmb"), 0.247f, 0.199f, 0.074f);
        glUniform3f(curS->getUniform("MatDif"), 0.751f, 0.606f, 0.226f);
        glUniform3f(curS->getUniform("MatSpec"), 0.628f, 0.556f, 0.366f);
        glUniform1f(curS->getUniform("MatShine"), 32.0f);
        break;

    case 7: // silver
        glUniform3f(curS->getUniform("MatAmb"), 0.19225f, 0.19225f, 0.19225f);
        glUniform3f(curS->getUniform("MatDif"), 0.50754f, 0.50754f, 0.50754f);
        glUniform3f(curS->getUniform("MatSpec"), 0.508273f, 0.508273f, 0.508273f);
        glUniform1f(curS->getUniform("MatShine"), 51.2f);
        break;

	case 8: // weathered wood?
		glUniform3f(curS->getUniform("MatAmb"), 0.15f, 0.10f, 0.04f);
		glUniform3f(curS->getUniform("MatDif"), 0.55f, 0.30f, 0.12f);
		glUniform3f(curS->getUniform("MatSpec"), 0.20f, 0.20f, 0.20f);
		glUniform1f(curS->getUniform("MatShine"), 8.0f);
		break;

	case 9: // painted red wood?
		glUniform3f(curS->getUniform("MatAmb"), 0.15f, 0.05f, 0.05f);
		glUniform3f(curS->getUniform("MatDif"), 0.80f, 0.15f, 0.15f);
		glUniform3f(curS->getUniform("MatSpec"), 0.3f, 0.3f, 0.3f);
		glUniform1f(curS->getUniform("MatShine"), 20.0f);
		break;

	case 10: // Doghouse wood
    glUniform3f(curS->getUniform("MatAmb"), 0.25f, 0.15f, 0.10f);
    glUniform3f(curS->getUniform("MatDif"),  0.70f, 0.40f, 0.20f);
    glUniform3f(curS->getUniform("MatSpec"), 0.2f, 0.15f, 0.1f);
    glUniform1f(curS->getUniform("MatShine"), 8.0f);
    break;

	case 11: // chat dog
	glUniform3f(curS->getUniform("MatAmb"), 0.25f, 0.20f, 0.12f);
	glUniform3f(curS->getUniform("MatDif"), 0.65f, 0.55f, 0.30f);
	glUniform3f(curS->getUniform("MatSpec"), 0.10f, 0.08f, 0.05f);
	glUniform1f(curS->getUniform("MatShine"), 4.0f);
	break;
    }
	}

	void updateUsingCameraPath(float deltaTime) {
		if (goCamera) {
			if (!splinepath[0].isDone()) {
				splinepath[0].update(deltaTime);
				g_eye = splinepath[0].getPosition();
			} else {
				splinepath[1].update(deltaTime);
				g_eye = splinepath[1].getPosition();
			}
		}
	}

	void SetView(shared_ptr<Program> shader, shared_ptr<MatrixStack> View) {
    	glm::mat4 Cam = glm::lookAt(g_eye, g_lookAt, glm::vec3(0,1,0));
    	glUniformMatrix4fv(shader->getUniform("V"), 1, GL_FALSE, glm::value_ptr(Cam));
	}

	
	void render(float frametime){
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float aspect = width / (float)height;
		auto Projection = make_shared<MatrixStack>();
		auto View = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();
		

		Projection->pushMatrix();
		Projection->perspective(45.0f, aspect, 0.01f, 100.0f);
		View->pushMatrix();
		View->loadIdentity();
		
		View->translate(vec3(0, 0, -3));
		updateUsingCameraPath(frametime);
		float x = radius * cos(pitch) * cos(yaw);
		float y = radius * sin(pitch);
		float z = radius * cos(pitch) * cos((3.14f/2.0f) - yaw);

		glm::vec3 lookAtPoint = camPos + glm::vec3(x, y, z);

		camFront = glm::normalize(glm::vec3(x, y, z));
		camRight = glm::normalize(glm::cross(camFront, glm::vec3(0,1,0)));

		//View->lookAt(camPos, lookAtPoint, glm::vec3(0,1,0)); 
		if (goCamera) {
			// cinematic spline camera
			View->loadIdentity();
			View->lookAt(g_eye, g_lookAt, glm::vec3(0,1,0));

			camPos = g_eye;
			camFront = normalize(g_lookAt - g_eye);

		} else {
			// your normal camera
			View->lookAt(camPos, lookAtPoint, glm::vec3(0,1,0));
		}
		//View->rotate(glm::radians(10.0f), glm::vec3(1, 0, 0));
		vec3 lightPos = vec3(2.0f + lightTrans, 3.0f, 4.0f);
		

		// dog animation
		static bool animating = false;
		static double startTime = 0.0;
		static glm::vec3 currentDogPos = dogStart;

		double t = glfwGetTime();

		if (gAnimate && !animating) {
			animating = true;
			startTime = t;
		}

		float animSpeed = 0.3f;
		float elapsed = (float)(t - startTime);
		float progress = glm::clamp(elapsed * animSpeed, 0.0f, 1.0f);

		if (animating) {
			currentDogPos = glm::mix(dogStart, dogEnd, progress);
			if (progress >= 1.0f) {
				animating = false;
				gAnimate = false;
			}
		}

		glm::vec3 dogPos = currentDogPos;

		// Skybox render
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);

		skyboxProg->bind();

		// remove translation from view matrix
		glm::mat4 viewNoTranslate = glm::mat4(glm::mat3(View->topMatrix()));

		glUniformMatrix4fv(skyboxProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(skyboxProg->getUniform("V"), 1, GL_FALSE, value_ptr(viewNoTranslate));
		glUniformMatrix4fv(skyboxProg->getUniform("M"), 1, GL_FALSE, value_ptr(glm::mat4(1.0f)));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
		glUniform1i(skyboxProg->getUniform("skybox"), 0);

		glBindVertexArray(skyboxVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);

		skyboxProg->unbind();

		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);

		Model->pushMatrix();
		Model->rotate(gSceneAngleY, vec3(0, -1, 0));

		texProg->bind();
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3fv(texProg->getUniform("lightPos"), 1, value_ptr(lightPos));

		grassTex->bind(texProg->getUniform("Texture0"));

		Model->pushMatrix();
		Model->translate(vec3(0.0f, -2.8f, 0.0f));
		Model->scale(vec3(20.0f, 1.0f, 20.0f));
		Model->multMatrix(plane.N);
		setModel(texProg, Model);
		drawMeshSet(texProg, plane);
		Model->popMatrix();

		grassTex->unbind();
		texProg->unbind();
		
		// lightingProg->bind();
		// glUniformMatrix4fv(lightingProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		// glUniformMatrix4fv(lightingProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		// glUniform3fv(lightingProg->getUniform("lightPos"), 1, value_ptr(lightPos));
		// glUniform3f(lightingProg->getUniform("LightColor"), 1.0f, 1.0f, 1.0f);

		// SetMaterial(lightingProg, 10);

		// Model->pushMatrix();
		// Model->translate(vec3(-3.5f, -1.35f, -2.0f));
		// Model->rotate(radians(20.0f), vec3(0,1,0));
		// Model->scale(vec3(1.8f));
		// Model->multMatrix(doghouse.N);
		// setModel(lightingProg, Model);
		// drawMeshSet(lightingProg, doghouse);
		// Model->popMatrix();

		// lightingProg->unbind();

		texProg->bind();

		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3fv(texProg->getUniform("lightPos"), 1, value_ptr(lightPos));

		// bind doghouse texture
		doghouseTex->bind(texProg->getUniform("Texture0"));

		Model->pushMatrix();
		Model->translate(vec3(-3.5f, -1.35f, -2.0f));
		Model->rotate(radians(20.0f), vec3(0,1,0));
		Model->scale(vec3(1.8f));
		Model->multMatrix(doghouse.N);
		setModel(texProg, Model);
		drawMeshSet(texProg, doghouse);
		Model->popMatrix();

		doghouseTex->unbind();
		texProg->unbind();

		lightingProg->bind();

		glUniformMatrix4fv(lightingProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(lightingProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3fv(lightingProg->getUniform("lightPos"), 1, value_ptr(lightPos));
		glUniform3f(lightingProg->getUniform("LightColor"), 1.0f, 1.0f, 1.0f);

		if (useAltMaterial) {
			SetMaterial(lightingProg, 3);
		} else {
			SetMaterial(lightingProg, 4);
		}

		Model->pushMatrix();
		Model->translate(glm::vec3(-1.9f, -2.7f, 0.0f));
		Model->scale(vec3(0.3f));
		Model->multMatrix(bowl.N);

		setModel(lightingProg, Model);
		drawMeshSet(lightingProg, bowl);
		Model->popMatrix();

		lightingProg->unbind();

		texProg->bind();
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3fv(texProg->getUniform("lightPos"), 1, value_ptr(lightPos));

		doghouseTex->bind(texProg->getUniform("Texture0"));

		Model->pushMatrix();
		Model->translate(glm::vec3(2.2f, -1.25f, -1.2f));
		Model->rotate(glm::radians(-30.0f), glm::vec3(0, 1, 0));
		Model->scale(vec3(1.5f));
		Model->multMatrix(plank.N);
		setModel(texProg, Model);
		drawMeshSet(texProg, plank);
		Model->popMatrix();

		doghouseTex->unbind();
		texProg->unbind();

		texProg->bind();

		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3fv(texProg->getUniform("lightPos"), 1, value_ptr(lightPos));

		// DOGHOUSE
		doghouseTex->bind(texProg->getUniform("Texture0"));

		Model->pushMatrix();
		Model->translate(vec3(-3.5f, -1.35f, -2.0f));
		Model->rotate(radians(20.0f), vec3(0,1,0));
		Model->scale(vec3(1.8f));
		Model->multMatrix(doghouse.N);
		setModel(texProg, Model);
		drawMeshSet(texProg, doghouse);
		Model->popMatrix();

		doghouseTex->unbind();

		// CRIB
		cribTex->bind(texProg->getUniform("Texture0"));

		Model->pushMatrix();
		Model->translate(vec3(-6.2f, -1.7f, 4.0f));
		Model->rotate(radians(20.0f), vec3(0,1,0));
		Model->scale(vec3(1.3f));
		Model->multMatrix(crib.N);
		setModel(texProg, Model);
		drawMeshSet(texProg, crib);
		Model->popMatrix();

		cribTex->unbind();

		// SHED
		shedTex->bind(texProg->getUniform("Texture0"));

		Model->pushMatrix();

		Model->translate(vec3(0.0f, -1.9f, 10.0f));
		Model->rotate(radians(20.0f), vec3(0,1,0));  
		Model->rotate(radians(160.0f), vec3(0,1,0));
		Model->scale(vec3(3.0f, 8.0f, 3.0f));

		Model->multMatrix(shed.N);
		setModel(texProg, Model);
		drawMeshSet(texProg, shed);
		Model->popMatrix();

		shedTex->unbind();

		// BED
		bedTex->bind(texProg->getUniform("Texture0"));

		Model->pushMatrix();
		Model->translate(vec3(0.0f, -2.1f, 8.5f));
		Model->rotate(radians(20.0f), vec3(0,1,0));
		Model->rotate(radians(160.0f), vec3(0,1,0));
		Model->scale(vec3(1.85f));
		Model->multMatrix(bed.N);
		setModel(texProg, Model);
		drawMeshSet(texProg, bed);
		Model->popMatrix();

		bedTex->unbind();

		texProg->unbind();

		texProg->unbind();

		// SHOP
		lightingProg->bind();
		glUniformMatrix4fv(lightingProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(lightingProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3fv(lightingProg->getUniform("lightPos"), 1, value_ptr(lightPos));
		glUniform3f(lightingProg->getUniform("LightColor"), 1.0f, 1.0f, 1.0f);

		SetMaterial(lightingProg, 3);  // or 8 if you want "weathered wood"

		Model->pushMatrix();
		Model->translate(vec3(0.0f, -2.8f, 2.7f));
		Model->rotate(radians(90.0f), vec3(0,1,0));
		Model->rotate(radians(90.0f), vec3(-1,0,0));
		Model->scale(vec3(2.7f));
		Model->multMatrix(shop.N);
		setModel(lightingProg, Model);
		drawMeshSet(lightingProg, shop);
		Model->popMatrix();

		lightingProg->unbind();

		// table and stuff
		lightingProg->bind();
		glUniformMatrix4fv(lightingProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(lightingProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3fv(lightingProg->getUniform("lightPos"), 1, value_ptr(lightPos));
		glUniform3f(lightingProg->getUniform("LightColor"), 1.0f, 1.0f, 1.0f);

		// table
		SetMaterial(lightingProg, 3);

		Model->pushMatrix();
		Model->translate(glm::vec3(2.2f, -2.0f, -1.2f));
		Model->rotate(glm::radians(-30.0f), glm::vec3(0, 1, 0));
		Model->scale(vec3(1.6f));
		Model->multMatrix(table.N);
		setModel(lightingProg, Model);
		drawMeshSet(lightingProg, table);
		Model->popMatrix();

		//hammer
		SetMaterial(lightingProg, 7);

		Model->pushMatrix();
		Model->translate(glm::vec3(2.2f, -1.2f, -1.2f));
		Model->rotate(glm::radians(-35.0f), glm::vec3(0, 1, 0));
		Model->scale(vec3(0.5f));
		Model->multMatrix(hammer.N);
		setModel(lightingProg, Model);
		drawMeshSet(lightingProg, hammer);
		Model->popMatrix();

		// nails
		SetMaterial(lightingProg, 4);

		vec3 tableBase(2.2f, -1.25f, -1.0f);
		float tableTiltY = glm::radians(-30.0f);

		vector<vec3> nailOffsets = {
			vec3( 0.08f, 0.05f,  0.03f),
			vec3(-0.12f, 0.05f, -0.02f),
			vec3( 0.18f, 0.05f, -0.10f),
			vec3(-0.06f, 0.05f,  0.12f),
			vec3( 0.00f, 0.05f,  0.00f),
			vec3( 0.10f, 0.05f, -0.07f)
		};

		vector<vec3> nailRotations = {
			vec3(0, 45, 90),
			vec3(0, 10, 90),
			vec3(0, -30, 90),
			vec3(0, 80, 90),
			vec3(0, 150, 90),
			vec3(0, -60, 90)
		};

		for (size_t i = 0; i < nailOffsets.size(); i++) {
			Model->pushMatrix();
			Model->translate(tableBase);
			Model->rotate(tableTiltY, vec3(0, 1, 0));
			Model->translate(nailOffsets[i]);

			Model->rotate(glm::radians(nailRotations[i].x), vec3(1, 0, 0));
			Model->rotate(glm::radians(nailRotations[i].y), vec3(0, 1, 0));
			Model->rotate(glm::radians(nailRotations[i].z), vec3(0, 0, 1));

			Model->scale(vec3(0.2f));
			Model->multMatrix(nail.N);
			setModel(lightingProg, Model);
			drawMeshSet(lightingProg, nail);
			Model->popMatrix();
		}

		lightingProg->unbind();

		// random bunny forest	
		lightingProg->bind();
		glUniformMatrix4fv(lightingProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(lightingProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3fv(lightingProg->getUniform("lightPos"), 1, value_ptr(lightPos));
		glUniform3f(lightingProg->getUniform("LightColor"), 1.0f, 1.0f, 1.0f);

		for (auto &b : bunnyForest) {
			Model->pushMatrix();
			Model->translate(b.pos);
			Model->rotate(b.rot.y, vec3(0,1,0));
			Model->scale(vec3(b.scale));
			Model->multMatrix(bunny.N);

			SetMaterial(lightingProg, b.materialID);
			setModel(lightingProg, Model);
			drawMeshSet(lightingProg, bunny);

			Model->popMatrix();
		}

		lightingProg->unbind();

		lightingProg->bind();
		glUniformMatrix4fv(lightingProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(lightingProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		glUniform3fv(lightingProg->getUniform("lightPos"), 1, value_ptr(lightPos));
		glUniform3f(lightingProg->getUniform("LightColor"), 1.0f, 1.0f, 1.0f);

		SetMaterial(lightingProg, 11);

		Model->pushMatrix();

		// Dog movement
		Model->translate(dogPos + glm::vec3(0.0f, -1.9f, 0.0f));
		float bob = 0.015f * sin((float)t * 6.0f);
		Model->translate(vec3(0.0f, bob, 0.0f));
		Model->rotate(glm::radians(70.0f), glm::vec3(0, 1, 0));
		Model->scale(vec3(0.5f));
		Model->multMatrix(dog.N);
		setModel(lightingProg, Model);
		drawMeshSet(lightingProg, dog);
		Model->popMatrix();
		lightingProg->unbind();
		Model->popMatrix();
		Projection->popMatrix();
		View->popMatrix();
	}
};

int main(int argc, char *argv[])
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(640, 480);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);
	application->generateBunnyForest();
	application->initTex(resourceDir);

	auto lastTime = chrono::high_resolution_clock::now();
	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// save current time for next frame
		auto nextLastTime = chrono::high_resolution_clock::now();

		// get time since last frame
		float deltaTime =
			chrono::duration_cast<std::chrono::microseconds>(
				chrono::high_resolution_clock::now() - lastTime)
				.count();
		// convert microseconds (weird) to seconds (less weird)
		deltaTime *= 0.000001;

		// reset lastTime so that we can calculate the deltaTime
		// on the next frame
		lastTime = nextLastTime;

		// Render scene.
		application->render(deltaTime);

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
