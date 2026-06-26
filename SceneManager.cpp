///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/
/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/rock.jpg", "stone"
	);

	bReturn = CreateGLTexture(
		"textures/lightstone.jpg", "light stone"
	);

	bReturn = CreateGLTexture(
		"textures/glass.jpg", "glass"
	);

	bReturn = CreateGLTexture(
		"textures/water.jpg", "water"
	);


	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials() {
	OBJECT_MATERIAL stoneMaterial;
	stoneMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	stoneMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	stoneMaterial.shininess = 0.1;
	stoneMaterial.tag = "stone";

	m_objectMaterials.push_back(stoneMaterial);

	OBJECT_MATERIAL poolMaterial;
	poolMaterial.diffuseColor = glm::vec3(0.0f, 0.0f, 1.0f);
	poolMaterial.specularColor = glm::vec3(0.21f, 0.46f, 0.53f);
	poolMaterial.shininess = 100;
	poolMaterial.tag = "pool";

	m_objectMaterials.push_back(poolMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights() {
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// directional light to emulate sunlight coming into scene
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.5f, -1.0f, -0.3f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.7f, 0.7f, 0.7f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 1.0f, 1.0f, 0.9f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// point light 
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 10.0f, 8.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.15f, 0.15f, 0.15f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", .5f, .5f, .5f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPyramid3Mesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();

	// load the textures for the 3D scene
	LoadSceneTextures();
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 1);
	SetShaderTexture("light stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	RenderTower();
	RenderLeftBuilding();
	RenderRightBuilding();
	RenderPool();
}


/****************************************************************
	Method for rendering the main tower
**************************************************************/
void SceneManager::RenderTower() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//BASE CYLINDER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 1.0f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("stone");
	SetTextureUVScale(6.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	//MAIN CYLINDER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 12.0f, 1.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.68, 0.85, 0.90, 1);
	SetShaderTexture("stone");
	SetTextureUVScale(5.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	//TOP CYLINDER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.7f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 13.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.68, 0.85, 0.90, 1);
	SetShaderTexture("stone");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	//EAST BIG CYLINDER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 13.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.59f, 0.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 0.8, 1);
	SetShaderTexture("light stone");
	SetTextureUVScale(5.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	//WEST BIG CYLINDER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 13.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.59f, 0.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 0.8, 1);
	SetShaderTexture("light stone");
	SetTextureUVScale(5.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	//NORTH BIG CYLINDER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 13.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -7.59f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 0.8, 1);
	SetShaderTexture("light stone");
	SetTextureUVScale(5.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	//NORTH BIG CYLINDER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 13.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -4.41f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 0.8, 1);
	SetShaderTexture("light stone");
	SetTextureUVScale(5.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	//SOUTH EAST SMALL CYLINDER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 11.0f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.02f, 0.0f, -4.98f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 0.8, 1);
	SetShaderTexture("light stone");
	SetTextureUVScale(5.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	//SOUTH EAST BOX
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 1.8f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.02f, 11.9f, -4.98f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	//SOUTH WEST CYLINDER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 11.0f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.02f, 0.0f, -4.98f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 0.8, 1);
	SetShaderTexture("light stone");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	//SOUTH WEST BOX
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 1.8f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.02f, 11.9f, -4.98);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	//NORTH WEST SMALL CYLINDER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 11.0f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.02f, 0.0f, -7.02f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 0.8, 1);
	SetShaderTexture("light stone");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	//NORTH WEST BOX
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 1.8f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.02f, 11.9f, -7.02f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	//NORTH EAST SMALL CYLINDER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 11.0f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.02f, 0.0f, -7.02f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 0.8, 1);
	SetShaderTexture("light stone");
	SetTextureUVScale(5.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	//NORTH EAST SMALL CYLINDER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 1.8f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.02f, 11.9f, -7.02f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	//TOP TAPERED
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.8f, 1.5f, 1.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 13.4f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("stone");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/
	//BOTTOM TAPERED
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();
}

/****************************************************************
	Method for rendering the left building
**************************************************************/
void SceneManager::RenderLeftBuilding(){
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//BOTTOM FLOOR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 0.4f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-15.0f, 0.2f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH WEST CORNER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 3.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-18.4f, 1.9f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH EAST CORNER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 3.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.6f, 1.9f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH WEST CORNER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 3.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-18.4f, 1.9f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH EAST CORNER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 3.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.6f, 1.9f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH TOP WALL
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.6f, 1.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-15.0f, 2.9f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH TOP WALL
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.6f, 1.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-15.0f, 2.9f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//WEST TOP WALL
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.2f, 1.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-18.8f, 2.9f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//EAST TOP WALL
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.2f, 1.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.2f, 2.9f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-15.0f, 1.4f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-14.0f, 1.4f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.0f, 1.4f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-16.0f, 1.4f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-17.0f, 1.4f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-15.0f, 1.4f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-14.0f, 1.4f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.0f, 1.4f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-16.0f, 1.4f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-17.0f, 1.4f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//WEST PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-18.8f, 1.4f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//WEST PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-18.8f, 1.4f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//WEST PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-18.8f, 1.4f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//EAST PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.2f, 1.4f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//EAST PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.2f, 1.4f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//EAST PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.2f, 1.4f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//ROOF FLOOR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.2f, 0.2f, 3.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-15.0f, 3.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-15.0f, 3.5f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-14.0f, 3.5f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.0f, 3.5f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-16.0f, 3.5f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-17.0f, 3.5f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-15.0f, 3.5f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-14.0f, 3.5f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.0f, 3.5f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-16.0f, 3.5f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-17.0f, 3.5f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//WEST ROOF BOX
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-18.0f, 3.5f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//EAST ROOF BOX
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.0f, 3.5f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.5f, 0.2f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-15.0f, 3.85f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("glass");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

}

/****************************************************************
	Method for rendering the right building
**************************************************************/
void SceneManager::RenderRightBuilding() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//BOTTOM FLOOR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 0.4f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 0.2f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH EAST CORNER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 3.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(18.4f, 1.9f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH WEST CORNER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 3.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.6f, 1.9f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH EAST CORNER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 3.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(18.4f, 1.9f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH WEST CORNER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 3.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.6f, 1.9f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH TOP WALL
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.6f, 1.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 2.9f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH TOP WALL
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.6f, 1.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 2.9f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//WEST TOP WALL
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.2f, 1.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(18.8f, 2.9f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//EAST TOP WALL
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.2f, 1.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.2f, 2.9f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 1.4f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(14.0f, 1.4f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.0f, 1.4f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(16.0f, 1.4f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(17.0f, 1.4f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 1.4f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(14.0f, 1.4f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.0f, 1.4f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(16.0f, 1.4f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(17.0f, 1.4f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//WEST PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(18.8f, 1.4f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//WEST PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(18.8f, 1.4f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//WEST PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(18.8f, 1.4f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//EAST PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.2f, 1.4f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//EAST PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.2f, 1.4f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//EAST PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.2f, 1.4f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//ROOF FLOOR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.2f, 0.2f, 3.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 3.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 3.5f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(14.0f, 3.5f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.0f, 3.5f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(16.0f, 3.5f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//SOUTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(17.0f, 3.5f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 3.5f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(14.0f, 3.5f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.0f, 3.5f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(16.0f, 3.5f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//NORTH ROOF PILLAR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(17.0f, 3.5f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//WEST ROOF BOX
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(18.0f, 3.5f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//EAST ROOF BOX
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.0f, 3.5f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.5f, 0.2f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 3.85f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("glass");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

}


/****************************************************************
	Method for rendering the pool
**************************************************************/
void SceneManager::RenderPool() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.5f, 0.0f, 6.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.1f, 6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("water");
	SetShaderMaterial("pool");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.2f, 5.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.1f, 9.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.2f, 5.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.1f, 2.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.2f, 7.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.0f, 0.1f, 6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.2f, 7.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 0.1f, 6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0.5, 0, 1);
	SetShaderTexture("stone");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

}