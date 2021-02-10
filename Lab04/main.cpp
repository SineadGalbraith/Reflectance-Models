// This code originally was the starter code provided for the CS7GV6 - Computer Graphics module project.
// I have used this code and modified it for the purposes of CS7GV3 - Real-Time Rendering Assignment 1.
// The Stanford Bunny model has been downloaded from: https://clara.io/view/a56a852d-08e4-402d-b4df-25ee1f798eb0
// For the Phong Shader, I used https://learnopengl.com/Lighting/Basic-Lighting for guidance and help.
// For the Toon Shader, I used a combination of https://www.lighthouse3d.com/tutorials/glsl-12-tutorial/toon-shading-version-i/ and https://github.com/mchamberlain/Cel-Shader to make both Toon Shaders.
// For the Cook-Torrance Shader, I used a combination of http://www.codinglabs.net/article_physically_based_rendering_cook_torrance.aspx and the source code for https://garykeen27.wixsite.com/portfolio/cook-torrance-shading.
// The link to my video demonstration is: https://youtu.be/CufuFn2IpGA
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <math.h>
#include <vector> 
#include <map>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/cimport.h>
#include <assimp/scene.h> 
#include <assimp/postprocess.h> 

#include "maths_funcs.h"

#include <ft2build.h>
#include FT_FREETYPE_H 

/*----------------------------------------------------------------------------
MESH
----------------------------------------------------------------------------*/
#define MODEL_MESH "stanford-bunny.dae"
/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

#pragma region SimpleTypes
typedef struct
{
	size_t mPointCount = 0;
	std::vector<vec3> mVertices;
	std::vector<vec3> mNormals;
	std::vector<vec2> mTextureCoords;
} ModelData;
#pragma endregion SimpleTypes

using namespace std;
GLuint phongShaderProgram, toonShaderProgram, toonShaderProgram2, cookTorranceShaderProgram, textShaderProgram;

ModelData mesh_data;
int width = 800;
int height = 600;

GLuint loc1, loc2, loc3;
GLfloat rotate_y = 0.0f;

unsigned int modelVBO = 0;
unsigned int modelNormalVBO = 0;
unsigned int modelVAO = 0;

glm::vec3 lightPos;
float specStrength;
float diffuseCo;
float specCo;
float roughness;

bool keyA = true;
bool keyC = false;
bool keyP = true;
bool keyT = false;

// ------------ FREETYPE/TEXT/FONT ------------
// The implementation of Freetype has been taken from https://learnopengl.com/In-Practice/Text-Rendering 
// for the puproses of displaying details about the Reflectance Models on the screen.
#pragma region TEXT
unsigned int textVAO, textVBO;

struct Character {
	unsigned int TextureID;
	glm::ivec2   Size;
	glm::ivec2   Bearing;
	unsigned int Advance;
};

std::map<char, Character> Characters;

int createFont() {
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
	{
		std::cout << "Error loading FreeType Library" << std::endl;
		return -1;
	}

	FT_Face face;
	if (FT_New_Face(ft, "./arial.ttf", 0, &face))
	{
		std::cout << "Error loading Font" << std::endl;
		return -1;
	}
	else {
		FT_Set_Pixel_Sizes(face, 0, 30);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		for (unsigned char c = 0; c < 128; c++)
		{
			if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			{
				std::cout << "Error loading Glyph" << std::endl;
				continue;
			}
			unsigned int texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				face->glyph->bitmap.width,
				face->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				face->glyph->bitmap.buffer
			);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			Character character = {
				texture,
				glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
				glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
				face->glyph->advance.x
			};
			Characters.insert(std::pair<char, Character>(c, character));
		}
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
}

void RenderText(GLuint shaderProgramID, std::string text, float x, float y, float scale, glm::vec3 color)
{
	glUseProgram(shaderProgramID);
	glUniform3f(glGetUniformLocation(shaderProgramID, "textColor"), color.x, color.y, color.z);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(textVAO);

	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++)
	{
		Character ch = Characters[*c];

		float xpos = x + ch.Bearing.x * scale;
		float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		float w = ch.Size.x * scale;
		float h = ch.Size.y * scale;
		float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },

			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }
		};
		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		glBindBuffer(GL_ARRAY_BUFFER, textVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		x += (ch.Advance >> 6) * scale;
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
#pragma endregion TEXT

#pragma region MESH LOADING
/*----------------------------------------------------------------------------
MESH LOADING FUNCTION
----------------------------------------------------------------------------*/
ModelData load_mesh(const char* file_name) {
	ModelData modelData;

	/* Use assimp to read the model file, forcing it to be read as    */
	/* triangles. The second flag (aiProcess_PreTransformVertices) is */
	/* relevant if there are multiple meshes in the model file that   */
	/* are offset from the origin. This is pre-transform them so      */
	/* they're in the right position.                                 */
	const aiScene* scene = aiImportFile(
		file_name, 
		aiProcess_Triangulate | aiProcess_PreTransformVertices
	); 

	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return modelData;
	}

	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);
		modelData.mPointCount += mesh->mNumVertices;
		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				modelData.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				modelData.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
			}
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				modelData.mTextureCoords.push_back(vec2(vt->x, vt->y));
			}
			if (mesh->HasTangentsAndBitangents()) {
				/* You can extract tangents and bitangents here              */
				/* Note that you might need to make Assimp generate this     */
				/* data for you. Take a look at the flags that aiImportFile  */
				/* can take.                                                 */
			}
		}
	}

	aiReleaseImport(scene);
	return modelData;
}

#pragma endregion MESH LOADING

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
char* readShaderSource(const char* shaderFile) {
	FILE* fp;
	fopen_s(&fp, shaderFile, "rb");

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		std::cerr << "Error creating shader..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024] = { '\0' };
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		std::cerr << "Error compiling "
			<< (ShaderType == GL_VERTEX_SHADER ? "vertex" : "fragment")
			<< " shader program: " << InfoLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders(GLuint shaderProgramID, const char *vertexShaderName, const char *fragmentShaderName)
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		std::cerr << "Error creating shader program..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, vertexShaderName, GL_VERTEX_SHADER);
	AddShader(shaderProgramID, fragmentShaderName, GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { '\0' };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS
void generateObjectBufferMesh(GLuint shaderProgramID) {
	/*----------------------------------------------------------------------------
	LOAD MESH HERE AND COPY INTO BUFFERS
	----------------------------------------------------------------------------*/

	//Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
	//Might be an idea to do a check for that before generating and binding the buffer.

	mesh_data = load_mesh(MODEL_MESH);

	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");

	glGenBuffers(1, &modelVBO);
	glBindBuffer(GL_ARRAY_BUFFER, modelVBO);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mVertices[0], GL_STATIC_DRAW);
	glGenBuffers(1, &modelNormalVBO);
	glBindBuffer(GL_ARRAY_BUFFER, modelNormalVBO);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mNormals[0], GL_STATIC_DRAW);

	glBindVertexArray(modelVAO);

	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, modelVBO);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, modelNormalVBO);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}

// This method creates the VBO and VAO needed for the FreeType text.
void generateFontBufferObjects() {
	glGenVertexArrays(1, &textVAO);
	glGenBuffers(1, &textVBO);
	glBindVertexArray(textVAO);
	glBindBuffer(GL_ARRAY_BUFFER, textVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
#pragma endregion VBO_FUNCTIONS


void display() {

	glEnable(GL_DEPTH_TEST); 
	glDepthFunc(GL_LESS); 
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// ------------- VIEW AND PROJ UNIFORMS -------------
	glm::mat4 view = glm::mat4(1.0f);
	// Depending on what key is pressed (aka which model you would like to see), the 
	// view and light source will have a different position.
	// Also if the key A is pressed, the models will stop rotating and face to the front.
	if (keyP) {
		keyT = false;
		keyC = false;
		view = glm::translate(view, glm::vec3(0.0, 0.0, -10.0f));
		lightPos = glm::vec3(-4.5f, 1.0f, 3.0f);
	}
	else if (keyT) {
		keyP = false;
		keyC = false;
		view = glm::translate(view, glm::vec3(0.0, 10.0, -12.0f));
		lightPos = glm::vec3(4.5f, -3.0f, 3.0f);
	}
	else if (keyC) {
		keyP = false;
		keyT = false;
		view = glm::translate(view, glm::vec3(0.0, 20.0, -10.0f));
		lightPos = glm::vec3(0.0f, -15.0f, 10.0f);
	}
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);

	// -------------------------------------------------------------------------------------- PHONG --------------------------------------------------------------------------------------
	glUseProgram(phongShaderProgram);

	// ------------- MODEL DETAILS -------------
	glm::vec3 objectColor = glm::vec3(0.0f, 0.6f, 0.6f);
	glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 viewPos = glm::vec3(0.0, 0.0, -10.0f);
	glUniform3fv(glGetUniformLocation(phongShaderProgram, "objectColor"), 1, &objectColor[0]);
	glUniform3fv(glGetUniformLocation(phongShaderProgram, "lightColor"), 1, &lightColor[0]);
	glUniform3fv(glGetUniformLocation(phongShaderProgram, "lightPos"), 1, &lightPos[0]);
	glUniform3fv(glGetUniformLocation(phongShaderProgram, "viewPos"), 1, &viewPos[0]);
	
	// The following three models show the Phong Illumination implementation with varying specular strength
	// values. The Phong Illumination Model is implemented through the use of a combination of Ambient, 
	// Diffuse and Specular lighting. The Ambient and Diffuse Values are set in the shader, but the Specular
	// value is specified within the program so as to show the different effects of different strength values.
	// ------------- PHONG MODEL LEFT (SPEC 0.2) -------------
	glm::mat4 phongLeftModel = glm::mat4(1.0f);
	phongLeftModel = glm::translate(phongLeftModel, glm::vec3(-3.5f, 0.0f, 0.0f));
	phongLeftModel = glm::rotate(phongLeftModel, glm::radians(rotate_y), glm::vec3(0.0f, 1.0f, 0.0f));

	specStrength = 0.2f;
	glUniform1f(glGetUniformLocation(phongShaderProgram, "specStrength"), specStrength);

	glUniformMatrix4fv(glGetUniformLocation(phongShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(phongShaderProgram, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
	glUniformMatrix4fv(glGetUniformLocation(phongShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(phongLeftModel));
	
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

	// ------------- PHONG MODEL MIDDEL (SPEC 0.6) -------------
	glm::mat4 phongMiddleModel = glm::mat4(1.0f);
	phongMiddleModel = glm::rotate(phongMiddleModel, glm::radians(rotate_y), glm::vec3(0.0f, 1.0f, 0.0f));
	
	specStrength = 0.6f;
	glUniform1f(glGetUniformLocation(phongShaderProgram, "specStrength"), specStrength);

	glUniformMatrix4fv(glGetUniformLocation(phongShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(phongMiddleModel));
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

	// ------------- PHONG MODEL RIGHT (SPEC 1.0) -------------
	glm::mat4 phongRightModel = glm::mat4(1.0f);
	phongRightModel = glm::translate(phongRightModel, glm::vec3(3.5f, 0.0f, 0.0f));
	phongRightModel = glm::rotate(phongRightModel, glm::radians(rotate_y), glm::vec3(0.0f, 1.0f, 0.0f));

	specStrength = 1.0f;
	glUniform1f(glGetUniformLocation(phongShaderProgram, "specStrength"), specStrength);

	glUniformMatrix4fv(glGetUniformLocation(phongShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(phongRightModel));
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);


	// -------------------------------------------------------------------------------------- TOON --------------------------------------------------------------------------------------
	glUseProgram(toonShaderProgram);
	
	// ------------- MODEL DETAILS -------------
	objectColor = glm::vec3(0.0f, 0.6f, 0.6f);
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
	viewPos = glm::vec3(0.0, 10.0, -10.0f);
	glUniform3fv(glGetUniformLocation(toonShaderProgram, "objectColor"), 1, &objectColor[0]);
	glUniform3fv(glGetUniformLocation(toonShaderProgram, "lightColor"), 1, &lightColor[0]);
	glUniform3fv(glGetUniformLocation(toonShaderProgram, "lightPos"), 1, &lightPos[0]);
	glUniform3fv(glGetUniformLocation(toonShaderProgram, "viewPos"), 1, &viewPos[0]);

	// ------------- TOON MODEL V1 LEFT  -------------
	// This is the first version of Toon Shading. In this version, the shading effects are determined
	// both by the position of light towards the model but also the position of the viewer. Due to this,
	// when the light is near the viewer, the viewer will be able to see more effects of the shading by 
	// comparison to the Toon version below. This is done through the use of Diffuse and Specular lighting.
	// As with the Phong method, when the user is looking at the model, they will see a specular shading 
	// area where the light hits the model and reflects into the viewers vision. 
	glm::mat4 toonV1Model1 = glm::mat4(1.0f);
	toonV1Model1 = glm::translate(toonV1Model1, glm::vec3(-5.0f, -10.0f, 0.0f));
	if (keyA) {
		toonV1Model1 = glm::rotate(toonV1Model1, glm::radians(rotate_y), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	diffuseCo = 0.4f;
	specCo = 0.6f;
	glUniform1f(glGetUniformLocation(toonShaderProgram, "diffuseCo"), diffuseCo);
	glUniform1f(glGetUniformLocation(toonShaderProgram, "specCo"), specCo);
	glUniformMatrix4fv(glGetUniformLocation(toonShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(toonShaderProgram, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
	glUniformMatrix4fv(glGetUniformLocation(toonShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(toonV1Model1));

	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

	// ------------- TOON MODEL V1 RIGHT  -------------
	glm::mat4 toonV1Model2 = glm::mat4(1.0f);
	toonV1Model2 = glm::translate(toonV1Model2, glm::vec3(-1.5f, -10.0f, 0.0f));
	if (keyA) {
		toonV1Model2 = glm::rotate(toonV1Model2, glm::radians(rotate_y), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	diffuseCo = 0.4f;
	specCo = 1.0f;
	glUniform1f(glGetUniformLocation(toonShaderProgram, "diffuseCo"), diffuseCo);
	glUniform1f(glGetUniformLocation(toonShaderProgram, "specCo"), specCo);
	glUniformMatrix4fv(glGetUniformLocation(toonShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(toonShaderProgram, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
	glUniformMatrix4fv(glGetUniformLocation(toonShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(toonV1Model2));

	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

	// ------------- TOON MODEL V2 -------------
	// This is the second implementation of Toon Shading. Unlike the version above, this shader does not use 
	// specular or diffuse lighting. There are no specular highlights with this version of Toon Shading. The 
	// viewers position is not taken into account, rather the shading values are calculated based only on the 
	// position of the model in terms of the light.
	glUseProgram(toonShaderProgram2);

	glm::mat4 toonV2Model = glm::mat4(1.0f);
	toonV2Model = glm::translate(toonV2Model, glm::vec3(3.5f, -10.0f, 0.0f));
	if (keyA) {
		toonV2Model = glm::rotate(toonV2Model, glm::radians(rotate_y), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glUniform3fv(glGetUniformLocation(toonShaderProgram, "lightPos"), 1, &lightPos[0]);
	glUniformMatrix4fv(glGetUniformLocation(toonShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(toonShaderProgram, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
	glUniformMatrix4fv(glGetUniformLocation(toonShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(toonV2Model));

	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);
	
	// -------------------------------------------------------------------------------------- COOK_TORRANCE --------------------------------------------------------------------------------------
	glUseProgram(cookTorranceShaderProgram);

	// ------------- MODEL DETAILS -------------
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
	viewPos = glm::vec3(0.0, 20.0, -10.0f);
	glUniform3fv(glGetUniformLocation(cookTorranceShaderProgram, "lightColor"), 1, &lightColor[0]);
	glUniform3fv(glGetUniformLocation(cookTorranceShaderProgram, "lightPos"), 1, &lightPos[0]);
	glUniform3fv(glGetUniformLocation(cookTorranceShaderProgram, "viewPos"), 1, &viewPos[0]);
	
	// The following models show the Cook-Torrance Shading implementation with varying roughness
	// values. The Cook-Torrance model is implemented through the use of a Distribution function,
	// a Fresnel function and a Geometric Function to calculate the colour of each fragment/pixel.
	// ------------- COOK-TORRANCE MODEL 1 (ROUGHNESS: 0.2) -------------
	glm::mat4 cookTorranceModel = glm::mat4(1.0f);
	cookTorranceModel = glm::translate(cookTorranceModel, glm::vec3(-2.5f, -20.0f, 0.0f));
	if (keyA) {
		cookTorranceModel = glm::rotate(cookTorranceModel, glm::radians(rotate_y), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	roughness = 0.2;
	glUniform1f(glGetUniformLocation(cookTorranceShaderProgram, "roughness"), roughness);
	glUniformMatrix4fv(glGetUniformLocation(cookTorranceShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(cookTorranceShaderProgram, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
	glUniformMatrix4fv(glGetUniformLocation(cookTorranceShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(cookTorranceModel));

	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);
	
	// ------------- COOK-TORRANCE MODEL 2 (ROUGHNESS: 0.8) -------------
	glm::mat4 cookTorranceModel2 = glm::mat4(1.0f);
	cookTorranceModel2 = glm::translate(cookTorranceModel2, glm::vec3(2.5f, -20.0f, 0.0f));
	if (keyA) {
		cookTorranceModel2 = glm::rotate(cookTorranceModel2, glm::radians(rotate_y), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	roughness = 0.8;
	glUniform1f(glGetUniformLocation(cookTorranceShaderProgram, "roughness"), roughness);
	glUniformMatrix4fv(glGetUniformLocation(cookTorranceShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(cookTorranceShaderProgram, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
	glUniformMatrix4fv(glGetUniformLocation(cookTorranceShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(cookTorranceModel2));

	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

	// -------------------------------------------------------------------------------------- TEXT --------------------------------------------------------------------------------------
	// Display the text on the screen.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glm::mat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);
	glUseProgram(textShaderProgram);
	glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "proj"), 1, GL_FALSE, glm::value_ptr(projection));

	RenderText(textShaderProgram, "P - Phong", 630.0f, 580.0f, 0.6f, glm::vec3(1.0f));
	RenderText(textShaderProgram, "T - Toon", 630.0f, 560.0f, 0.6f, glm::vec3(1.0f));
	RenderText(textShaderProgram, "C - Cook-Torrance", 630.0f, 540.0f, 0.6f, glm::vec3(1.0f));

	if (keyP) {
		// Phong 
		RenderText(textShaderProgram, "Phong", 350.0f, 500.0f, 1.2f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "Specular: 0.2", 70.0f, 200.0f, 0.8f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "Specular: 0.6", 340.0f, 200.0f, 0.8f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "Specular: 1.0", 600.0f, 200.0f, 0.8f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "Light Source: (-4.5f, 1.0f, 3.0f)", 10.0f, 570.0f, 0.6f, glm::vec3(1.0f));
	}

	if (keyT) {
		// Toon
		RenderText(textShaderProgram, "Toon", 360.0f, 500.0f, 1.2f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "Toon V1", 150.0f, 440.0f, 1.0f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "Diffuse: 0.4", 35.0f, 220.0f, 0.7f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "Specular: 0.6", 35.0f, 190.0f, 0.7f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "Diffuse: 0.4", 260.0f, 220.0f, 0.7f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "Specular: 1.0", 260.0f, 190.0f, 0.7f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "Toon V2", 560.0f, 440.0f, 1.0f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "- Not based on viewers position.", 480.0f, 220.0f, 0.7f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "- Calculated based on fragment", 480.0f, 190.0f, 0.7f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "  position from the light source. ", 480.0f, 170.0f, 0.7f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "- No specular highlights. ", 480.0f, 140.0f, 0.7f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "Light Source: (4.5f, -3.0f, 3.0f)", 10.0f, 570.0f, 0.6f, glm::vec3(1.0f));

	}

	if (keyC) {
		// Cook-Torrance
		RenderText(textShaderProgram, "Cook-Torrance", 300.0f, 500.0f, 1.2f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "Roughness: 0.2", 150.0f, 220.0f, 0.65f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "Roughness: 0.8", 520.0f, 220.0f, 0.65f, glm::vec3(1.0f));
		RenderText(textShaderProgram, "Light Source: (0.0f, -15.0f, 10.0f)", 10.0f, 570.0f, 0.6f, glm::vec3(1.0f));
	}

	glDisable(GL_BLEND);

	// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	glutSwapBuffers();
}


void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	// Rotate the model slowly around the y axis at 20 degrees per second
	rotate_y += 40.0f * delta;
	rotate_y = fmodf(rotate_y, 360.0f);

	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Set up the shaders
	phongShaderProgram = CompileShaders(phongShaderProgram, "phongVertexShader.txt", "phongFragmentShader.txt");
	toonShaderProgram = CompileShaders(toonShaderProgram, "toonVertexShader.txt", "toonFragmentShader.txt");
	toonShaderProgram2 = CompileShaders(toonShaderProgram2, "toonVertexShader2.txt", "toonFragmentShader2.txt");
	cookTorranceShaderProgram = CompileShaders(cookTorranceShaderProgram, "cookTorranceVertexShader.txt", "cookTorranceFragmentShader.txt");
	textShaderProgram = CompileShaders(textShaderProgram, "textVertexShader.txt", "textFragmentShader.txt");
	// load mesh into a vertex buffer array
	generateObjectBufferMesh(phongShaderProgram);
	generateObjectBufferMesh(toonShaderProgram);
	generateObjectBufferMesh(toonShaderProgram2);
	generateObjectBufferMesh(cookTorranceShaderProgram);
	createFont();
	generateFontBufferObjects();

}

// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {
	if (key == 'a') {
		if (!keyA) {
			keyA = true;
		}
		else if (keyA) {
			keyA = false;
		}
	}
	else if (key == 'c') {
		if (!keyC) {
			keyC = true;
			keyP = false;
			keyT = false;
		}
	}
	else if (key == 'p') {
		if (!keyP) {
			keyP = true;
			keyC = false;
			keyT = false;
		}
	}
	else if (key == 't') {
		if (!keyT) {
			keyT = true;
			keyC = false;
			keyP = false;
		}
	}
}

int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Lab 1 Reflectance Models");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);

	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
	return 0;
}
