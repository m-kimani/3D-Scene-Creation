///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
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

	modelView = translation * rotationX * rotationY * rotationZ * scale;

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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/***********************************************************
* LoadSceneTextures()
*
* This method is used for preparing the 3D scene by loading
* the shapes, textures in memory to support the 3D scene
* rendering
***********************************************************/
void SceneManager::LoadSceneTextures() {
	// variable for this method
	bool bReturn = false;
	// initialize the loaded textures count to 0
	m_loadedTextures = 0;
	// load the texture images into memory and create the OpenGL textures
	bReturn = CreateGLTexture("./Utilities/textures/redfoil.jpg", "redfoil");
	bReturn = CreateGLTexture("./Utilities/textures/rock.jpg", "rock");
	bReturn = CreateGLTexture("./Utilities/textures/black-texture.jpg", "leather");
	bReturn = CreateGLTexture("./Utilities/textures/blue-cookie.jpg", "cookie-box");
	bReturn = CreateGLTexture("./Utilities/textures/black-plastic.jpg", "plastic");
	bReturn = CreateGLTexture("./Utilities/textures/basketball.jpg", "basketball");
	
	// a�er  the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	OBJECT_MATERIAL foil;
	foil.ambientColor = glm::vec3(0.35f, 0.08f, 0.08f);
	foil.ambientStrength = 0.18f;
	foil.diffuseColor = glm::vec3(0.90f, 0.22f, 0.22f);
	foil.specularColor = glm::vec3(0.55f, 0.55f, 0.55f);
	foil.shininess = 28.0f;
	foil.tag = "foil";
	m_objectMaterials.push_back(foil);

	OBJECT_MATERIAL rock;
	rock.ambientColor = glm::vec3(0.08f, 0.08f, 0.08f);
	rock.ambientStrength = 0.20f;
	rock.diffuseColor = glm::vec3(0.42f, 0.42f, 0.42f);
	rock.specularColor = glm::vec3(0.10f, 0.10f, 0.10f);
	rock.shininess = 10.0f;
	rock.tag = "rock";
	m_objectMaterials.push_back(rock);

	OBJECT_MATERIAL leather;
	leather.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	leather.ambientStrength = 0.12f;	
	leather.diffuseColor = glm::vec3(0.22f, 0.22f, 0.22f);
	leather.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	leather.shininess = 4.0f;
	leather.tag = "leather";
	m_objectMaterials.push_back(leather);

	OBJECT_MATERIAL plastic;
	plastic.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	plastic.ambientStrength = 0.12f;
	plastic.diffuseColor = glm::vec3(0.22f, 0.22f, 0.22f);
	plastic.specularColor = glm::vec3(0.55f, 0.55f, 0.55f);
	plastic.shininess = 16.0f;
	plastic.tag = "plastic";
	m_objectMaterials.push_back(plastic);

	OBJECT_MATERIAL basketball;
	basketball.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	basketball.ambientStrength = 0.12f;
	basketball.diffuseColor = glm::vec3(0.22f, 0.22f, 0.22f);
	basketball.specularColor = glm::vec3(0.55f, 0.55f, 0.55f);
	basketball.shininess = 16.0f;
	basketball.tag = "basketball";
	m_objectMaterials.push_back(basketball);

	OBJECT_MATERIAL cookieBox;
	cookieBox.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	cookieBox.ambientStrength = 0.12f;
	cookieBox.diffuseColor = glm::vec3(0.22f, 0.22f, 0.22f);
	cookieBox.specularColor = glm::vec3(0.55f, 0.55f, 0.55f);
	cookieBox.shininess = 20.0f;
	cookieBox.tag = "cookie-box";
	m_objectMaterials.push_back(cookieBox);


}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/
	m_pShaderManager->setVec3Value("lightSources[0].position", -6.0f, 9.0f, 8.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.12f, 0.10f, 0.08f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.85f, 0.78f, 0.68f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.55f, 0.50f, 0.45f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.35f);

	m_pShaderManager->setVec3Value("lightSources[1].position", 7.0f, 5.0f, 2.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.03f, 0.04f, 0.05f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.22f, 0.26f, 0.30f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.10f, 0.12f, 0.14f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 24.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.12f);

	m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, 6.0f, -10.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.00f, 0.00f, 0.00f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.18f, 0.18f, 0.20f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.20f, 0.20f, 0.22f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 40.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.18f);

	m_pShaderManager->setVec3Value("lightSources[3].position", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.00f, 0.00f, 0.00f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.00f, 0.00f, 0.00f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.00f, 0.00f, 0.00f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.0f);

	m_pShaderManager->setBoolValue("bUseLighting", true);


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
	// load the textures for the 3D scene
	LoadSceneTextures();

	// define the materials for the 3D scene
	DefineObjectMaterials();

	// set up the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->DrawHalfTorusMesh();
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

	// Setting the texture UV scale for the plane mesh
	SetTextureUVScale(10.0f, 10.0f);
	// Set the texture for the plane mesh
	SetShaderTexture("rock");
	// Set the material for the plane mesh
	SetShaderMaterial("rock");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/*** Set needed transformations before drawing the cone mesh.  ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.2f, 3.0f, 2.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.9f, 10.3f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Setting the texture UV scale for the cone mesh
	SetTextureUVScale(1.0f, 1.0f);
	// Set the texture for the cone mesh
	// Set the material for the cone mesh
	SetShaderTexture("redfoil");
	SetShaderMaterial("foil");
	
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();


	/*** Set needed transformations before drawing the sphere mesh.  ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 1.5f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.9f, 11.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Setting the texture UV scale for the first sphere mesh
	SetTextureUVScale(1.0f, 1.0f);
	// Set the texture for the first sphere mesh
	SetShaderTexture("redfoil");
	// Set the material for the first sphere mesh
	SetShaderMaterial("foil");
	

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();



	/*** Set needed transformations before drawing the sphere mesh.  ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 1.5f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.9f, 11.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Setting the same texture UV scale for the second sphere mesh
	SetTextureUVScale(1.0f, 1.0f);
	// Set the same texture for the second sphere mesh
	SetShaderTexture("redfoil");
	// Set the same material for the second sphere mesh
	SetShaderMaterial("foil");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the box mesh.  ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.46f, 0.7f, 2.66f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 4.24f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Setting the texture UV scale for the box mesh
	SetTextureUVScale(1.0f, 1.0f);
	// Set the texture for the box mesh
	SetShaderTexture("leather"); // Change to cardboard box texture
	// Set the material for the box mesh
	SetShaderMaterial("leather"); // Change to cardboard material

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*** Set needed transformations before drawing the box mesh.  ***/
	
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.2f, 3.64f, 0.42f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.3f, 6.04f, -1.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Setting the texture UV scale for the box mesh
	SetTextureUVScale(1.0f, 1.0f);
	// Set the texture for the box mesh
	SetShaderTexture("leather"); // Change to cardboard box texture
	// Set the material for the box mesh
	SetShaderMaterial("leather"); // Change to cardboard material

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	/*** Set needed transformations for the cylinder mesh.  ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.308f, 3.01f, 0.308f);	

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 1.1f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Setting the texture UV scale for the cylinder mesh
	SetTextureUVScale(1.0f, 1.0f);

	// Set the texture for the cylinder mesh
	SetShaderTexture("plastic");
	// Set the material for the cylinder mesh
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// /*** Set needed transformations before drawing the torus mesh.  ***/

	// // set the XYZ scale for the mesh
	// scaleXYZ = glm::vec3(0.672f, 1.008f, 0.224f);

	// // set the XYZ rotation for the mesh
	// XrotationDegrees = 0.0f;
	// YrotationDegrees = 0.0f;
	// ZrotationDegrees = 180.0f;

	// // set the XYZ position for the mesh
	// positionXYZ = glm::vec3(-0.4f, 5.395f, 1.35f);

	// // set the transformations into memory to be used on the drawn meshes
	// SetTransformations(
	// 	scaleXYZ,
	// 	XrotationDegrees,
	// 	YrotationDegrees,
	// 	ZrotationDegrees,
	// 	positionXYZ);

	// // Setting the texture UV scale for the torus mesh
	// SetTextureUVScale(1.0f, 1.0f);
	// // Set the texture for the torus mesh
	// SetShaderTexture("rock");
	// // Set the material for the torus mesh
	// SetShaderMaterial("rock");

	// // draw the mesh with transformation values
	// m_basicMeshes->DrawHalfTorusMesh();

	// /*** Set needed transformations before drawing the torus mesh.  ***/

	// // set the XYZ scale for the mesh
	// scaleXYZ = glm::vec3(0.672f, 1.008f, 0.224f);

	// // set the XYZ rotation for the mesh
	// XrotationDegrees = 0.0f;
	// YrotationDegrees = 180.0f;
	// ZrotationDegrees = 0.0f;

	// // set the XYZ position for the mesh
	// positionXYZ = glm::vec3(2.8f, 5.395f, 1.35f);

	// // set the transformations into memory to be used on the drawn meshes
	// SetTransformations(
	// 	scaleXYZ,
	// 	XrotationDegrees,
	// 	YrotationDegrees,
	// 	ZrotationDegrees,
	// 	positionXYZ);

	// // Setting the texture UV scale for the torus mesh
	// SetTextureUVScale(1.0f, 1.0f);
	// // Set the texture for the torus mesh
	// SetShaderTexture("rock");
	// // Set the material for the torus mesh
	// SetShaderMaterial("rock");

	// // draw the mesh with transformation values
	// m_basicMeshes->DrawHalfTorusMesh();

	/*** Set needed transformations before drawing the torus mesh.  ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.47f, 0.14f, 0.14f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 1.05f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Setting the texture UV scale for the torus mesh
	SetTextureUVScale(1.0f, 1.0f);
	// Set the texture for the torus mesh
	SetShaderTexture("plastic");
	// Set the material for the torus mesh
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfTorusMesh();

	/*** Set needed transformations before drawing the torus mesh.  ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.47f, 0.14f, 0.14f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 1.05f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Setting the texture UV scale for the torus mesh
	SetTextureUVScale(1.0f, 1.0f);
	// Set the texture for the torus mesh
	SetShaderTexture("plastic");
	// Set the material for the torus mesh
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfTorusMesh();

	/*** Set needed transformations before drawing the sphere mesh.  ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.078f, 1.033f, 1.078f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.15f, 5.5f, 0.25f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Setting the texture UV scale for the sphere mesh
	SetTextureUVScale(1.0f, 1.0f);
	// Set the texture for the sphere mesh
	SetShaderTexture("basketball");
	// Set the material for the sphere mesh
	SetShaderMaterial("basketball");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the box mesh.  ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.49f, 1.44f, 0.315f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -20.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.85f, 5.45f, 0.65f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Setting the texture UV scale for the box mesh
	SetTextureUVScale(1.0f, 1.0f);
	// Set the texture for the box mesh
	SetShaderTexture("cookie-box");
	// Set the material for the box mesh
	SetShaderMaterial("cookie-box");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*** Set needed transformations before drawing the sphere mesh.  ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.322f, 0.322f, 0.322f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.95f, 0.95f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Setting the texture UV scale for the sphere mesh
	SetTextureUVScale(1.0f, 1.0f);
	// Set the texture for the sphere mesh
	SetShaderTexture("plastic");
	// Set the material for the sphere mesh
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the sphere mesh.  ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.322f, 0.322f, 0.322f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.35f, 0.95f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Setting the texture UV scale for the sphere mesh
	SetTextureUVScale(1.0f, 1.0f);
	// Set the texture for the sphere mesh
	SetShaderTexture("plastic");
	// Set the material for the sphere mesh
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the sphere mesh.  ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.322f, 0.322f, 0.322f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 0.95f, 2.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Setting the texture UV scale for the sphere mesh
	SetTextureUVScale(1.0f, 1.0f);
	// Set the texture for the sphere mesh
	SetShaderTexture("plastic");
	// Set the material for the sphere mesh
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the sphere mesh.  ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.322f, 0.322f, 0.322f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.8f, 0.95f, -2.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Setting the texture UV scale for the sphere mesh
	SetTextureUVScale(1.0f, 1.0f);
	// Set the texture for the sphere mesh
	SetShaderTexture("plastic");
	// Set the material for the sphere mesh
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	/****************************************************************/
}
