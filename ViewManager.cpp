///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////


#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// declaration of the global variables and defines
namespace
{
    // Variables for window width and height
    const int WINDOW_WIDTH = 1000;
    const int WINDOW_HEIGHT = 800;
    const char* g_ViewName = "view";
    const char* g_ProjectionName = "projection";

    // camera object used for viewing and interacting with
    // the 3D scene
    Camera* g_pCamera = nullptr;

    // these variables are used for mouse movement processing
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // time between current frame and last frame
    float gDeltaTime = 0.0f;
    float gLastFrame = 0.0f;

    // the following variable is false when orthographic projection
    // is off and true when it is on
    bool bOrthographicProjection = false;
}

bool ViewManager::keys[1024] = { false };
float ViewManager::lastX = WINDOW_WIDTH / 2.0f;
float ViewManager::lastY = WINDOW_HEIGHT / 2.0f;
bool ViewManager::firstMouse = true;

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(ShaderManager* pShaderManager)
    : m_pShaderManager(pShaderManager), m_camera(glm::vec3(0.0f, 5.0f, 12.0f)), orthographicView(false)
{
    // initialize the member variables
    m_pWindow = NULL;
    g_pCamera = new Camera(glm::vec3(0.0f, 5.0f, 12.0f));
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
    // free up allocated memory
    m_pShaderManager = NULL;
    m_pWindow = NULL;
    if (NULL != g_pCamera)
    {
        delete g_pCamera;
        g_pCamera = NULL;
    }
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
    GLFWwindow* window = nullptr;

    // try to create the displayed OpenGL window
    window = glfwCreateWindow(
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        windowTitle,
        NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return NULL;
    }
    glfwMakeContextCurrent(window);

    // tell GLFW to capture all mouse events
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // this callback is used to receive mouse moving events
    glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

    // this callback is used to receive mouse scroll events
    glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);

    // this callback is used to receive keyboard events
    glfwSetKeyCallback(window, &ViewManager::Key_Callback);

    // enable blending for supporting tranparent rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_pWindow = window;

    return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
    if (firstMouse)
    {
        lastX = xMousePos;
        lastY = yMousePos;
        firstMouse = false;
    }

    float xoffset = xMousePos - lastX;
    float yoffset = lastY - yMousePos; // reversed since y-coordinates go from bottom to top

    lastX = xMousePos;
    lastY = yMousePos;

    g_pCamera->ProcessMouseMovement(xoffset, yoffset);
}

/***********************************************************
 *  Mouse_Scroll_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse scroll wheel is moved within the active GLFW
 *  display window.
 ***********************************************************/
void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xoffset, double yoffset)
{
    g_pCamera->ProcessMouseScroll(yoffset);
}

/***********************************************************
 *  Key_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  a key is pressed within the active GLFW display window.
 ***********************************************************/
void ViewManager::Key_Callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
        keys[key] = true;
    else if (action == GLFW_RELEASE)
        keys[key] = false;

    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_P)
        {
            bOrthographicProjection = false;
        }
        else if (key == GLFW_KEY_O)
        {
            bOrthographicProjection = true;
        }
    }
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents(float deltaTime)
{
    // close the window if the escape key has been pressed
    if (keys[GLFW_KEY_ESCAPE])
    {
        glfwSetWindowShouldClose(m_pWindow, true);
    }

    // movement keys
    if (keys[GLFW_KEY_W])
        g_pCamera->ProcessKeyboard(FORWARD, deltaTime);
    if (keys[GLFW_KEY_S])
        g_pCamera->ProcessKeyboard(BACKWARD, deltaTime);
    if (keys[GLFW_KEY_A])
        g_pCamera->ProcessKeyboard(LEFT, deltaTime);
    if (keys[GLFW_KEY_D])
        g_pCamera->ProcessKeyboard(RIGHT, deltaTime);
    if (keys[GLFW_KEY_Q])
        g_pCamera->ProcessKeyboard(UP, deltaTime);
    if (keys[GLFW_KEY_E])
        g_pCamera->ProcessKeyboard(DOWN, deltaTime);
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
    glm::mat4 view;
    glm::mat4 projection;

    // per-frame timing
    float currentFrame = glfwGetTime();
    gDeltaTime = currentFrame - gLastFrame;
    gLastFrame = currentFrame;

    // process any keyboard events that may be waiting in the 
    // event queue
    ProcessKeyboardEvents(gDeltaTime);

    // get the current view matrix from the camera
    view = g_pCamera->GetViewMatrix();

    if (bOrthographicProjection)
    {
        // define the current orthographic projection matrix
        float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
        float orthoHeight = 10.0f; // adjust this value as needed
        projection = glm::ortho(-aspect * orthoHeight, aspect * orthoHeight, -orthoHeight, orthoHeight, 0.1f, 100.0f);
    }
    else
    {
        // define the current perspective projection matrix
        projection = glm::perspective(glm::radians(g_pCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }

    // if the shader manager object is valid
    if (NULL != m_pShaderManager)
    {
        // set the view matrix into the shader for proper rendering
        m_pShaderManager->setMat4Value(g_ViewName, view);
        // set the view matrix into the shader for proper rendering
        m_pShaderManager->setMat4Value(g_ProjectionName, projection);
        // set the view position of the camera into the shader for proper rendering
        m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
    }
}