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
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
    m_pShaderManager = pShaderManager;
    m_basicMeshes = new ShapeMeshes();

    for (int i = 0; i < 16; i++)
    {
        m_textureIDs[i].tag = "/0";
        m_textureIDs[i].ID = -1;
    }
    m_loadedTextures = 0;
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

    DestroyGLTextures();
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

    CreateGLTexture("Source/Brick.jpg", "brickTexture");
    CreateGLTexture("Source/Wood.jpg", "woodTexture");
    CreateGLTexture("Source/Granite.jpg", "graniteTexture");
    CreateGLTexture("Source/ceramicMaterial.jpg", "mugTexture");
    CreateGLTexture("Source/monsterTexture.jpg", "monsterTexture");
    CreateGLTexture("Source/blackboxTexture.jpg", "blackboxTexture");
    CreateGLTexture("Source/monsterTop.jpg", "monsterTopTexture");
    CreateGLTexture("Source/orangeTexture.jpg", "orangeTexture");


    // after the texture image data is loaded into memory, the
    // loaded textures need to be bound to texture slots - there
    // are a total of 16 available slots for scene textures
    BindGLTextures();
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
    LoadSceneTextures();

    SetupSceneLights();

    m_basicMeshes->LoadBoxMesh();
    m_basicMeshes->LoadPlaneMesh();
    m_basicMeshes->LoadCylinderMesh();
    m_basicMeshes->LoadConeMesh();
    m_basicMeshes->LoadPrismMesh();
    m_basicMeshes->LoadPyramid4Mesh();
    m_basicMeshes->LoadSphereMesh();
    m_basicMeshes->LoadTaperedCylinderMesh();
    m_basicMeshes->LoadTorusMesh();
}
/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
    glm::vec3 scaleXYZ;
    glm::vec3 positionXYZ;

    // Draw the plane mesh (granite countertop)
    scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
    positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f); 
    SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
    SetShaderTexture("graniteTexture");
    SetShaderMaterial("granite");
    m_basicMeshes->DrawPlaneMesh();

    // Draw the black box
    scaleXYZ = glm::vec3(2.0f, 0.5f, 3.0f);
    positionXYZ = glm::vec3(-8.0f, 0.5f, 2.5f); 
    SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
    SetShaderTexture("blackboxTexture");
    SetShaderMaterial("wood");
    m_basicMeshes->DrawBoxMesh();

    // Draw the cylinder for the crayon body
    scaleXYZ = glm::vec3(0.7f, 3.0f, 0.7f);
    positionXYZ = glm::vec3(-3.5f, 0.25f, -0.5f); 
    SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
    SetShaderTexture("orangeTexture");
    SetShaderMaterial("wood");
    m_basicMeshes->DrawCylinderMesh();

    // Draw the cone for the crayon tip
    scaleXYZ = glm::vec3(0.7f, 1.0f, 0.7f);
    positionXYZ = glm::vec3(-3.5f, 3.25f, -0.5f); 
    SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
    SetShaderTexture("orangeTexture");
    SetShaderMaterial("metal");
    m_basicMeshes->DrawConeMesh();

    // Draw the Monster can body
    scaleXYZ = glm::vec3(0.7f, 3.0f, 0.7f);
    positionXYZ = glm::vec3(2.0f, 0.0f, 0.0f);  
    SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
    SetShaderTexture("monsterTexture");
    SetShaderMaterial("wood");
    m_basicMeshes->DrawCylinderMesh();

    // Draw the top of the Monster can with the top texture
    scaleXYZ = glm::vec3(0.7f, 0.01f, 0.7f);
    positionXYZ = glm::vec3(2.0f, 3.0f, 0.0f);  
    SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
    SetShaderTexture("monsterTopTexture");
    m_basicMeshes->DrawCylinderMesh();


    // Draw the mug body
    scaleXYZ = glm::vec3(1.0f, 2.0f, 1.0f);
    positionXYZ = glm::vec3(6.5f, 0.25f, 2.0f); 
    SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
    SetShaderTexture("mugTexture");
    SetShaderMaterial("ceramicMaterial");
    m_basicMeshes->DrawCylinderMesh();

    // Draw the mug handle
    scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f); 
    positionXYZ = glm::vec3(7.5f, 1.25f, 2.0f); 
    SetTransformations(scaleXYZ, 0.0f, 0.0f, 90.0f, positionXYZ);
    SetShaderTexture("mugTexture");
    m_basicMeshes->DrawTorusMesh();

    scaleXYZ = glm::vec3(1.01f, 0.01f, 1.01f); 
    positionXYZ = glm::vec3(6.5f, 2.25f, 2.0f); 
    SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
    SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); 
    m_basicMeshes->DrawCylinderMesh(); 
}
/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
    OBJECT_MATERIAL goldMaterial;
    goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
    goldMaterial.ambientStrength = 0.3f;
    goldMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
    goldMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
    goldMaterial.shininess = 22.0;
    goldMaterial.tag = "metal";
    m_objectMaterials.push_back(goldMaterial);

    OBJECT_MATERIAL woodMaterial;
    woodMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
    woodMaterial.ambientStrength = 0.2f;
    woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
    woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
    woodMaterial.shininess = 0.3;
    woodMaterial.tag = "wood";
    m_objectMaterials.push_back(woodMaterial);

    OBJECT_MATERIAL graniteMaterial;
    graniteMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
    graniteMaterial.ambientStrength = 0.3f;
    graniteMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
    graniteMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
    graniteMaterial.shininess = 20.0;
    graniteMaterial.tag = "granite";
    m_objectMaterials.push_back(graniteMaterial);

    // Ceramic material for the Mug
    OBJECT_MATERIAL ceramicMaterial;
    ceramicMaterial.ambientColor = glm::vec3(0.25f, 0.25f, 0.25f);
    ceramicMaterial.ambientStrength = 0.4f;
    ceramicMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
    ceramicMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
    ceramicMaterial.shininess = 50.0f;
    ceramicMaterial.tag = "ceramicMaterial";
    m_objectMaterials.push_back(ceramicMaterial);

    // Matte material for the Black Box
    OBJECT_MATERIAL matteMaterial;
    matteMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
    matteMaterial.ambientStrength = 0.2f;
    matteMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
    matteMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
    matteMaterial.shininess = 1.0f;
    matteMaterial.tag = "matteMaterial";
    m_objectMaterials.push_back(matteMaterial);

    // Metallic material for the Monster Can
    OBJECT_MATERIAL metalCanMaterial;
    metalCanMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
    metalCanMaterial.ambientStrength = 0.3f;
    metalCanMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
    metalCanMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    metalCanMaterial.shininess = 80.0f;
    metalCanMaterial.tag = "metalCanMaterial";
    m_objectMaterials.push_back(metalCanMaterial);
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


    m_pShaderManager->setBoolValue(g_UseLightingName, true);

    // Light Source 1 - Bright Yellow
    m_pShaderManager->setVec3Value("lightSources[0].position", -5.0f, 5.0f, 5.0f);
    m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.3f, 0.3f, 0.1f); // Yellow ambient
    m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.8f, 0.8f, 0.4f); // Bright yellow diffuse
    m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.6f, 0.6f, 0.3f); // Yellow specular
    m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 40.0f);
    m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.7f);

    // Light Source 2 - Bright Yellow
    m_pShaderManager->setVec3Value("lightSources[1].position", 5.0f, 5.0f, 5.0f);
    m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.3f, 0.3f, 0.1f); // Yellow ambient
    m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.8f, 0.8f, 0.4f); // Bright yellow diffuse
    m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.6f, 0.6f, 0.3f); // Yellow specular
    m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 40.0f);
    m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.7f);

    // Light Source 3 - Brighter Blue-Yellow Mix
    m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, 10.0f, 0.0f);
    m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.2f, 0.2f, 0.1f); // Yellowish ambient
    m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.6f, 0.6f, 0.4f); // Mix of yellow and blue
    m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.4f, 0.4f, 0.3f); // Yellow specular with some blue
    m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 20.0f);
    m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.5f);

    // Light Source 4 - Bright Yellow
    m_pShaderManager->setVec3Value("lightSources[3].position", 0.0f, 5.0f, -5.0f);
    m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.3f, 0.3f, 0.1f); // Yellow ambient
    m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.8f, 0.8f, 0.4f); // Bright yellow diffuse
    m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.6f, 0.6f, 0.3f); // Yellow specular
    m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 40.0f);
    m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.7f);
}