/*
Reference: http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/
*/
#include <iostream>
#include <fstream>
#include <sstream>

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include "stb_image.h"
#include "objloader.h"
#include "vboindexer.h"

GLFWwindow* window;
int Width = 1024;
int Height = 768;

GLuint depthProgramID;
GLuint ObjectShaderProgramID;

GLuint FramebufferName = 0;
GLuint depthTexture;
GLuint Texture;

float objSize = 1.5f;
float posx = 0.0f;
float posy = 0.0f;
float posz = 0.0f;

GLuint objectloc1;
GLuint objectloc2;
GLuint objectloc3;



// loadOBJ
std::vector<glm::vec3> vertices;
std::vector<glm::vec2> uvs;
std::vector<glm::vec3> normals;
bool res = loadOBJ("c:/Users/lenovo/source/repos/Rendering Assignment-Shadow Mapping/Rendering Assignment-Shadow Mapping/Extra/sample_base.obj", vertices, uvs, normals);

std::vector<unsigned short> indices;
std::vector<glm::vec3> indexed_vertices;
std::vector<glm::vec2> indexed_uvs;
std::vector<glm::vec3> indexed_normals;


std::string readShaderSource(const std::string& fileName)
{
	std::ifstream file(fileName.c_str());
	if (file.fail()) {
		std::cerr << "Error loading shader called " << fileName << std::endl;
		exit(EXIT_FAILURE);
	}

	std::stringstream stream;
	stream << file.rdbuf();
	file.close();

	return stream.str();
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType) {
	GLuint ShaderObj = glCreateShader(ShaderType);
	if (ShaderObj == 0) {
		std::cerr << "Error creating shader type " << ShaderType << std::endl;
		exit(EXIT_FAILURE);
	}

	/* bind shader source code to shader object */
	std::string outShader = readShaderSource(pShaderText);
	const char* pShaderSource = outShader.c_str();
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);

	/* compile the shader and check for errors */
	glCompileShader(ShaderObj);
	GLint success;
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		std::cerr << "Error compiling shader type " << ShaderType << ": " << InfoLog << std::endl;
		exit(EXIT_FAILURE);
	}
	glAttachShader(ShaderProgram, ShaderObj); /* attach compiled shader to shader programme */
}

GLuint CompileShaders(const char* pVShaderText, const char* pFShaderText)
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	GLuint ShaderProgramID = glCreateProgram();
	if (ShaderProgramID == 0) {
		std::cerr << "Error creating shader program" << std::endl;
		exit(EXIT_FAILURE);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(ShaderProgramID, pVShaderText, GL_VERTEX_SHADER);
	AddShader(ShaderProgramID, pFShaderText, GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { 0 };


	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(ShaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(ShaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(ShaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
		exit(EXIT_FAILURE);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(ShaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(ShaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(ShaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
		exit(EXIT_FAILURE);
	}
	return ShaderProgramID;
}


void generateObjectBuffer(GLuint temp)
{
	objectloc1 = glGetAttribLocation(temp, "vertexPosition_modelspace");
	objectloc2 = glGetAttribLocation(temp, "vertexUV");
	objectloc3 = glGetAttribLocation(temp, "vertexNormal_modelspace");


	glDisableVertexAttribArray(objectloc1);
	glDisableVertexAttribArray(objectloc2);
	glDisableVertexAttribArray(objectloc3);

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

	GLuint normalbuffer;
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

	GLuint elementbuffer;
	glGenBuffers(1, &elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);


	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	glEnableVertexAttribArray(objectloc1);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(objectloc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(objectloc2);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glVertexAttribPointer(objectloc2, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(objectloc3);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glVertexAttribPointer(objectloc3, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
}


void init(void) {
	// Shader for Depth
	depthProgramID = CompileShaders("c:/Users/lenovo/source/repos/Rendering Assignment-Shadow Mapping/Rendering Assignment-Shadow Mapping/Shader/DepthVS.txt",
		"c:/Users/lenovo/source/repos/Rendering Assignment-Shadow Mapping/Rendering Assignment-Shadow Mapping/Shader/DepthFS.txt");
	// Shader for Object
	ObjectShaderProgramID = CompileShaders("c:/Users/lenovo/source/repos/Rendering Assignment-Shadow Mapping/Rendering Assignment-Shadow Mapping/Shader/ObjectVS.txt",
		"c:/Users/lenovo/source/repos/Rendering Assignment-Shadow Mapping/Rendering Assignment-Shadow Mapping/Shader/ObjectFS.txt");
}

GLuint TextureFromFile(const char* path)
{
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	// all upcoming GL_TEXTURE_2D operations now have effect on this texture object
	// set the texture wrapping parameters

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps

	int width, height, nrChannels;

	unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
	if (data)
	{
		if (nrChannels == 1) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
		}
		else if (nrChannels == 3) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else {
			fprintf(stderr, "Error: Unexpected image format.\n");
			exit(EXIT_FAILURE);
		}
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
	std::cout << "loaded texture" << std::endl;
	return texture;

}

void createDepthTexture() {
	// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	glGenFramebuffers(1, &FramebufferName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

	// Depth texture. Slower than a depth buffer, but you can sample it later in your shader
	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);

	// No color output in the bound framebuffer, only depth.
	glDrawBuffer(GL_NONE);

	// Always check that our framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "createDepthTexture False";
	}
}

void display(void) {
	
	glClearColor(0.8f, 0.8f, 0.8f, 1.0f);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Depth
	glUseProgram(depthProgramID);
	generateObjectBuffer(depthProgramID);

	GLuint depthMatrixID = glGetUniformLocation(depthProgramID, "depthMVP");

	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

	glViewport(0, 0, 1024, 1024);
	glm::vec3 lightInvDir = glm::vec3(0.5f, 2, 2);
	glm::mat4 depthProjectionMatrix = glm::ortho<float>(-10, 10, -10, 10, -10, 20);
	glm::mat4 depthViewMatrix = glm::lookAt(lightInvDir, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 depthModelMatrix = glm::mat4(1.0);
	glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

	glUniformMatrix4fv(depthMatrixID, 1, GL_FALSE, &depthMVP[0][0]);

	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, NULL);

	// Scene
	glUseProgram(ObjectShaderProgramID);
	generateObjectBuffer(ObjectShaderProgramID);

	GLuint shader_myTextureSampler_location = glGetUniformLocation(ObjectShaderProgramID, "myTextureSampler");
	GLuint shader_MVPr_location = glGetUniformLocation(ObjectShaderProgramID, "MVP");
	GLuint shader_DepthBiasMVP_location = glGetUniformLocation(ObjectShaderProgramID, "DepthBiasMVP");
	GLuint shader_shadowMapr_location = glGetUniformLocation(ObjectShaderProgramID, "shadowMap");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, Width, Height);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 ProjMatrix = glm::perspective(objSize, (float)(Width) / (float)Height, 0.5f, 50.0f);
	glm::mat4 ViewMatrix = glm::lookAt(glm::vec3(14, 6, 4), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0));
	glm::mat4 ModelMatrix = glm::mat4(1.0);
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(posx,posy,posz));
	glm::mat4 MVP = ProjMatrix * ViewMatrix * ModelMatrix;
	glm::mat4 biasMatrix(
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0
	);
	glm::mat4 depthBiasMVP = biasMatrix * depthMVP;

	glUniformMatrix4fv(shader_MVPr_location, 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(shader_DepthBiasMVP_location, 1, GL_FALSE, &depthBiasMVP[0][0]);

	// Texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture);
	glUniform1i(shader_myTextureSampler_location, 0);

	// Shadow
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glUniform1i(shader_shadowMapr_location, 1);

	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, NULL);
}

void keyPress(GLFWwindow* window, int key, int scancode, int action, int mods) {
	switch (key) {
	case(GLFW_KEY_Q):
		objSize += 0.5;
		std::cout << objSize << "objSize" << std::endl;
		break;
	case(GLFW_KEY_A):
		objSize -= 0.5;
		std::cout << objSize << "objSize" << std::endl;
		break;
	case (GLFW_KEY_W):
		posx += 0.1;
		std::cout << posx << "Keypress: " << key << std::endl;
		break;
	case (GLFW_KEY_S):
		posx -= 0.1;
		std::cout << posx << "Keypress: " << key << std::endl;
		break;
	case (GLFW_KEY_E):
		posy += 0.1;
		std::cout << posy << "Keypress: " << key << std::endl;
		break;
	case (GLFW_KEY_D):
		posy -= 0.1;
		std::cout << posy << "Keypress: " << key << std::endl;
		break;
	case (GLFW_KEY_R):
		posz += 0.1;
		std::cout << posz << "Keypress: " << key << std::endl;
		break;
	case (GLFW_KEY_F):
		posz -= 0.1;
		std::cout << posz << "Keypress: " << key << std::endl;
		break;

	}
};


int main(void)
{
	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	window = glfwCreateWindow(Width, Height, "Shadow Mapping", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);

	glfwGetFramebufferSize(window, &Width, &Height);

	glewExperimental = true; 
	GLenum err = glewInit();

	if (err != GLEW_OK) {
		fprintf(stderr, "Error: %s.\n", glewGetErrorString(err));
		exit(EXIT_FAILURE);
	}

	// Removing Duplicates
	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);
	Texture = TextureFromFile("c:/Users/lenovo/source/repos/Rendering Assignment-Shadow Mapping/Rendering Assignment-Shadow Mapping/Extra/RedColor.png");

	init();
	createDepthTexture();
	glfwSetKeyCallback(window, keyPress);

	do{
		display();
		glfwSwapBuffers(window);
		glfwPollEvents();

	}
	while (!glfwWindowShouldClose(window));

	glfwTerminate();

	return 0;
}

