#include <iostream>             // cout, cerr
#include <cstdlib>              // EXIT_FAILURE
#include <GL/glew.h>            // GLEW library
#include <GLFW/glfw3.h>         // GLFW library

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnOpengl/camera.h> // Camera class

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Tutorial 4.4"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbo;         // Handle for the vertex buffer object
        GLuint nVertices;    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh gPlaneMesh, gCubeMesh, gCylinderMesh, gSphereMesh, gMesh;
    // Shader program
    GLuint gProgramId;
    GLuint gCubeProgramId;
    GLuint gLampProgramId;

    // camera
    Camera gCamera(glm::vec3(2.0f, 1.0f, 4.0f), glm::vec3(0.0f, 1.0f, 0.0f), -120.0f, -15.0f); // position, up vector, yaw angle, pitch angle
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;
    bool perspectiveCamera = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    GLclampf gBackgroundColor_R = 0.55f;
    GLclampf gBackgroundColor_G = 0.3f;
    GLclampf gBackgroundColor_B = 0.4f;
    GLclampf gBackgroundColor_A = 0.5f;

    // Subject position and scale
    glm::vec3 gCubePosition(0.0f, 0.0f, 0.0f);
    glm::vec3 gCubeScale(0.2f);

    // Cube and light color
    glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
    glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);

    // Light position and scale
    glm::vec3 gLightPosition(1.5f, 0.5f, 2.0f);
    glm::vec3 gLightScale(0.2f);

    // Lamp animation
    bool gIsLampOrbiting = true;
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh, vector<glm::vec3>& positions, vector<glm::vec3>& normals);
void UCreatePlaneMesh(GLMesh& mesh);
void UCreateCubeMesh(GLMesh& mesh);
void UCreateCylinderMesh(GLMesh& mesh);
void UCreateSphereMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0
    layout(location = 1) in vec3 normal;   // Normal data from Vertex Attrib Pointer 1

    out vec3 vertexNormal; // Variable to transfer normal data to the fragment shader
    out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader

    // Global variables for the transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

        vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

        vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    }
);

/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,
    in vec3 vertexNormal; // Variable to hold incoming normal data from vertex shader
    in vec3 vertexFragmentPos; // For incoming fragment position

    out vec4 fragmentColor; // Final output color

    uniform vec3 objectColor; // Current object color
    uniform vec3 lightColor;
    uniform vec3 lightPos;
    uniform vec3 viewPosition;


    void main()
    {
        /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

        //Calculate Ambient lighting*/
        float ambientStrength = 0.1f; // Set ambient or global lighting strength
        vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

        //Calculate Diffuse lighting*/
        vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
        vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
        vec3 diffuse = impact * lightColor; // Generate diffuse light color

        //Calculate Specular lighting*/
        float specularIntensity = 0.8f; // Set specular light strength
        float highlightSize = 16.0f; // Set specular highlight size
        vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
        //Calculate specular component
        float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
        vec3 specular = specularIntensity * specularComponent * lightColor;

        // Calculate phong result
        vec3 phong = (ambient + diffuse + specular) * objectColor;

        fragmentColor = vec4(phong, 1.0f); // Send lighting results to GPU
    }
);

/* Cube Vertex Shader Source Code*/
const GLchar* cubeVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
    layout(location = 1) in vec3 normal; // VAP position 1 for normals

    out vec3 vertexNormal; // For outgoing normals to fragment shader
    out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader

    //Uniform / Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

        vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

        vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    }
);


/* Cube Fragment Shader Source Code*/
const GLchar* cubeFragmentShaderSource = GLSL(440,

    in vec3 vertexNormal; // For incoming normals
    in vec3 vertexFragmentPos; // For incoming fragment position

    out vec4 fragmentColor; // For outgoing cube color to the GPU

    // Uniform / Global variables for object color, light color, light position, and camera/view position
    uniform vec3 objectColor;
    uniform vec3 lightColor;
    uniform vec3 lightPos;
    uniform vec3 viewPosition;

    void main()
    {
        /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

        //Calculate Ambient lighting*/
        float ambientStrength = 0.1f; // Set ambient or global lighting strength
        vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

        //Calculate Diffuse lighting*/
        vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
        vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
        vec3 diffuse = impact * lightColor; // Generate diffuse light color

        //Calculate Specular lighting*/
        float specularIntensity = 0.8f; // Set specular light strength
        float highlightSize = 16.0f; // Set specular highlight size
        vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
        //Calculate specular component
        float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
        vec3 specular = specularIntensity * specularComponent * lightColor;

        // Calculate phong result
        vec3 phong = (ambient + diffuse + specular) * objectColor;

        fragmentColor = vec4(phong, 1.0f); // Send lighting results to GPU
    }
);

/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

    //Uniform / Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
    }
);


/* Lamp Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

    void main()
    {
        fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
    }
);



int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
    {
        // Let the user read the error message before exiting
        cin.get();
        return EXIT_FAILURE;
    }

    // Create the meshes for primitive shapes
    UCreatePlaneMesh(gPlaneMesh);
    UCreateCubeMesh(gCubeMesh);
    UCreateCylinderMesh(gCylinderMesh);
    UCreateSphereMesh(gSphereMesh);
    UCreateCubeMesh(gMesh); // Calls the function to create the Vertex Buffer Object

    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
    {
        // Let the user read the error message before exiting
        cin.get();
        return EXIT_FAILURE;
    }

    if (!UCreateShaderProgram(cubeVertexShaderSource, cubeFragmentShaderSource, gCubeProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = (float)glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gPlaneMesh);
    UDestroyMesh(gCubeMesh);
    UDestroyMesh(gCylinderMesh);
    UDestroyMesh(gSphereMesh);
    UDestroyMesh(gMesh);


    // Release shader program
    UDestroyShaderProgram(gProgramId);
    // Release shader programs
    UDestroyShaderProgram(gCubeProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetKeyCallback(*window, UKeyCallback);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    // Pause and resume lamp orbiting
    static bool isLKeyDown = false;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Camera Control
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);

    // Light Control
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && !gIsLampOrbiting)
        gIsLampOrbiting = true;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && gIsLampOrbiting)
        gIsLampOrbiting = false;
    if (!gIsLampOrbiting) {
        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) {
            gLightPosition.y += gDeltaTime;
        }
        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) {
            gLightPosition.y -= gDeltaTime;
        }
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            gLightPosition.z -= gDeltaTime;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            gLightPosition.z += gDeltaTime;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            gLightPosition.x += gDeltaTime;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            gLightPosition.x -= gDeltaTime;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        gBackgroundColor_R += gDeltaTime;
        if (gBackgroundColor_R > 1.0f) {
            gBackgroundColor_R = 1.0f;
        }
        cout << "BACKGROUND COLOR (RGBA) " << " R: " << gBackgroundColor_R << " G: " << gBackgroundColor_G << " B: " << gBackgroundColor_B << " A: " << gBackgroundColor_A << endl;

    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        gBackgroundColor_G += gDeltaTime;
        if (gBackgroundColor_G > 1.0f) {
            gBackgroundColor_G = 1.0f;
        }
        cout << "BACKGROUND COLOR (RGBA) " << " R: " << gBackgroundColor_R << " G: " << gBackgroundColor_G << " B: " << gBackgroundColor_B << " A: " << gBackgroundColor_A << endl;

    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        gBackgroundColor_B += gDeltaTime;
        if (gBackgroundColor_B > 1.0f) {
            gBackgroundColor_B = 1.0f;
        }
        cout << "BACKGROUND COLOR (RGBA) " << " R: " << gBackgroundColor_R << " G: " << gBackgroundColor_G << " B: " << gBackgroundColor_B << " A: " << gBackgroundColor_A << endl;

    }
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
        gBackgroundColor_A += gDeltaTime;
        if (gBackgroundColor_A > 1.0f) {
            gBackgroundColor_A = 1.0f;
        }
        cout << "BACKGROUND COLOR (RGBA) " << " R: " << gBackgroundColor_R << " G: " << gBackgroundColor_G << " B: " << gBackgroundColor_B << " A: " << gBackgroundColor_A << endl;

    }
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) {
        gBackgroundColor_R -= gDeltaTime;
        if (gBackgroundColor_R < 0.0f) {
            gBackgroundColor_R = 0.0f;
        }
        cout << "BACKGROUND COLOR (RGBA) " << " R: " << gBackgroundColor_R << " G: " << gBackgroundColor_G << " B: " << gBackgroundColor_B << " A: " << gBackgroundColor_A << endl;

    }
    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) {
        gBackgroundColor_G -= gDeltaTime;
        if (gBackgroundColor_G < 0.0f) {
            gBackgroundColor_G = 0.0f;
        }
        cout << "BACKGROUND COLOR (RGBA) " << " R: " << gBackgroundColor_R << " G: " << gBackgroundColor_G << " B: " << gBackgroundColor_B << " A: " << gBackgroundColor_A << endl;

    }
    if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) {
        gBackgroundColor_B -= gDeltaTime;
        if (gBackgroundColor_B < 0.0f) {
            gBackgroundColor_B = 0.0f;
        }
        cout << "BACKGROUND COLOR (RGBA) " << " R: " << gBackgroundColor_R << " G: " << gBackgroundColor_G << " B: " << gBackgroundColor_B << " A: " << gBackgroundColor_A << endl;

    }
    if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS) {
        gBackgroundColor_A -= gDeltaTime;
        if (gBackgroundColor_A < 0.0f) {
            gBackgroundColor_A = 0.0f;
        }
        cout << "BACKGROUND COLOR (RGBA) " << " R: " << gBackgroundColor_R << " G: " << gBackgroundColor_G << " B: " << gBackgroundColor_B << " A: " << gBackgroundColor_A << endl;

    }    

}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the key is pressed/released, this callback is called
// -------------------------------------------------------
void UKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Switch perspective/orthographic camera
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
        perspectiveCamera = !perspectiveCamera;
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = (float)xpos;
        gLastY = (float)ypos;
        gFirstMouse = false;
    }

    float xoffset = float(xpos - gLastX);
    float yoffset = float(gLastY - ypos); // reversed since y-coordinates go from bottom to top

    gLastX = (float)xpos;
    gLastY = (float)ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll((float)yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}

// Draws the specified mesh
void UDrawMesh(GLMesh& mesh)
{
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh.vao);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, mesh.nVertices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Draws a plane at the location using the size value
void UDrawPlane(glm::vec3 center, float size)
{
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 scale = glm::scale(glm::vec3(size, size, size));
    glm::mat4 translation = glm::translate(center);
    glm::mat4 model = translation * scale;

    // Draw the plane mesh using the model matrix
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    UDrawMesh(gPlaneMesh);
}

// Draws a cube at the location using the sizes vector
void UDrawCube(glm::vec3 center, glm::vec3 sizes)
{
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 scale = glm::scale(sizes);
    glm::mat4 translation = glm::translate(center);
    glm::mat4 model = translation * scale;

    // Draw the cube mesh using the model matrix
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    UDrawMesh(gCubeMesh);
}

// Draws a cylinder between start and end using the radius value
void UDrawCylinder(glm::vec3 start, glm::vec3 end, float radius)
{
    // Bounds of cylinder endpoints
    glm::vec3 bounds = glm::abs(end - start);

    // No rotation for cylinders going along the Y axis
    glm::mat4 rotation(1.0f);

    if (bounds.x > bounds.y && bounds.x > bounds.z)
    {
        // Rotate 90 degrees right for cylinders going along the X axis
        rotation = glm::rotate(glm::radians(90.0f), glm::vec3(0.0, 0.0f, 1.0f));
    }
    else if (bounds.z > bounds.y)
    {
        // Rotate 90 degrees backwards for cylinders going along the Z axis
        rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0, 0.0f, 0.0f));
    }

    // Cylinder length and center position
    float length = glm::distance(start, end);
    glm::vec3 center = (start + end) * 0.5f;

    // Model matrix: transformations are applied right-to-left order
    glm::mat4 scale = glm::scale(glm::vec3(radius, length, radius));
    glm::mat4 translation = glm::translate(center);
    glm::mat4 model = translation * rotation * scale;

    // Draw the cylinder mesh using the model matrix
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    UDrawMesh(gCylinderMesh);
}

// Draws a sphere at the location using the radius value
void UDrawSphere(glm::vec3 center, float radius)
{
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 scale = glm::scale(glm::vec3(radius, radius, radius));
    glm::mat4 translation = glm::translate(center);
    glm::mat4 model = translation * scale;

    // Draw the sphere mesh using the model matrix
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    UDrawMesh(gSphereMesh);
}

// Draws a rounded cube at the location using the sizes vector and angle value
void UDrawRoundedCube(glm::vec3 center, glm::vec3 sizes, float angle)
{
    // Draw a stretched cube at the center
    UDrawCube(center, sizes);

    // Cube half sizes
    float hx = sizes.x * 0.5f;
    float hy = sizes.y * 0.5f * sinf(angle);
    float hz = sizes.z * 0.5f * cosf(angle);

    // Rounding radius for edges and corners
    float radius = glm::min(sizes.y, sizes.z) * 0.5f;

    // Draw 4 cylinders for rounded edges
    UDrawCylinder(center + glm::vec3(-hx, -hy, -hz), center + glm::vec3(+hx, -hy, -hz), radius);
    UDrawCylinder(center + glm::vec3(-hx, +hy, +hz), center + glm::vec3(+hx, +hy, +hz), radius);
    UDrawCylinder(center + glm::vec3(-hx, -hy, -hz), center + glm::vec3(-hx, +hy, +hz), radius);
    UDrawCylinder(center + glm::vec3(+hx, -hy, -hz), center + glm::vec3(+hx, +hy, +hz), radius);

    // Draw 4 spheres for rounded corners
    UDrawSphere(center + glm::vec3(-hx, -hy, -hz), radius);
    UDrawSphere(center + glm::vec3(-hx, +hy, +hz), radius);
    UDrawSphere(center + glm::vec3(+hx, -hy, -hz), radius);
    UDrawSphere(center + glm::vec3(+hx, +hy, +hz), radius);
}

// Draws the chair on the floor
void UDrawChair()
{
    // Draw a plane for the floor
    GLint colorLoc = glGetUniformLocation(gProgramId, "objectColor");
    glUniform3f(colorLoc, 0.35f, 0.32f, 0.30f);
    UDrawPlane(glm::vec3(0.0f, -1.0f, 0.0f), 3.0f);

    // Draw stretched rounded cubes for the seat and back
    glUniform3f(colorLoc, 0.2f, 0.4f, 1.0f);
    UDrawRoundedCube(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.2f, 1.0f), 0.0f);
    UDrawRoundedCube(glm::vec3(0.0f, 1.0f, -0.5f), glm::vec3(1.2f, 0.5f, 0.2f), glm::radians(90.0f));

    // Draw 4 cylinders for the legs
    glUniform3f(colorLoc, 0.2f, 0.2f, 0.2f);
    UDrawCylinder(glm::vec3(-0.5f, -1, +0.5f), glm::vec3(-0.5f, 0, +0.5f), 0.04f);
    UDrawCylinder(glm::vec3(+0.5f, -1, +0.5f), glm::vec3(+0.5f, 0, +0.5f), 0.04f);
    UDrawCylinder(glm::vec3(-0.5f, -1, -0.5f), glm::vec3(-0.5f, +1, -0.5f), 0.04f);
    UDrawCylinder(glm::vec3(+0.5f, -1, -0.5f), glm::vec3(+0.5f, +1, -0.5f), 0.04f);
}

// Function called to render a frame
void URender()
{
    // Lamp orbits around the origin
    const float angularVelocity = glm::radians(45.0f);
    if (gIsLampOrbiting)
    {
        float angle = angularVelocity * gDeltaTime;
        glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);
        glm::vec4 newPosition = glm::rotate(angle, rotationAxis) * glm::vec4(gLightPosition, 1.0f);
        gLightPosition.x = newPosition.x;
        gLightPosition.y = newPosition.y;
        gLightPosition.z = newPosition.z;
    }

    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(gBackgroundColor_R, gBackgroundColor_G, gBackgroundColor_B, gBackgroundColor_A);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Activate the cube VAO (used by cube and lamp)
    // gPlaneMesh, gCubeMesh, gCylinderMesh, gSphereMesh

    glBindVertexArray(gPlaneMesh.vao);
    glBindVertexArray(gCubeMesh.vao);
    glBindVertexArray(gCylinderMesh.vao);
    glBindVertexArray(gSphereMesh.vao);

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = glm::translate(gCubePosition) * glm::scale(gCubeScale);

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    // Draw the chair on the floor
    UDrawChair();

    // LAMP: draw lamp
    glBindVertexArray(gMesh.vao);
    glUseProgram(gLampProgramId);

    //Transform the smaller cube used as a visual que for the light source
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices);

    // Deactivate the Vertex Array Object and shader program
    glBindVertexArray(0);
    glUseProgram(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}

// Creates a mesh from positions and normals
void UCreateMesh(GLMesh& mesh, vector<glm::vec3>& positions, vector<glm::vec3>& normals)
{
    // Combine positions and normals into a vertex data array
    vector<float> verts;
    for (int i = 0; i < positions.size(); i++)
    {
        verts.push_back(positions[i].x);
        verts.push_back(positions[i].y);
        verts.push_back(positions[i].z);
        verts.push_back(normals[i].x);
        verts.push_back(normals[i].y);
        verts.push_back(normals[i].z);
    }

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    mesh.nVertices = (GLuint)positions.size();

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create VBO
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts[0]) * verts.size(), verts.data(), GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
}

// Creates a plane mesh
void UCreatePlaneMesh(GLMesh& mesh)
{
    // Positions of plane vertices
    vector<glm::vec3> positions;
    positions.push_back(glm::vec3(-0.5f, 0.0f, -0.5f));
    positions.push_back(glm::vec3(0.5f, 0.0f, -0.5f));
    positions.push_back(glm::vec3(0.5f, 0.0f, 0.5f));
    positions.push_back(glm::vec3(0.5f, 0.0f, 0.5f));
    positions.push_back(glm::vec3(-0.5f, 0.0f, 0.5f));
    positions.push_back(glm::vec3(-0.5f, 0.0f, -0.5f));

    // Normals of plane vertices
    vector<glm::vec3> normals;
    normals.push_back(glm::vec3(0.0f, +1.0f, 0.0f));
    normals.push_back(glm::vec3(0.0f, +1.0f, 0.0f));
    normals.push_back(glm::vec3(0.0f, +1.0f, 0.0f));
    normals.push_back(glm::vec3(0.0f, +1.0f, 0.0f));
    normals.push_back(glm::vec3(0.0f, +1.0f, 0.0f));
    normals.push_back(glm::vec3(0.0f, +1.0f, 0.0f));

    // Create a mesh from positions and normals
    UCreateMesh(mesh, positions, normals);
}

// Creates a cube mesh
void UCreateCubeMesh(GLMesh& mesh)
{
    // Positions of cube vertices
    vector<glm::vec3> positions;

    positions.push_back(glm::vec3(-0.5f, -0.5f, -0.5f));
    positions.push_back(glm::vec3(0.5f, -0.5f, -0.5f));
    positions.push_back(glm::vec3(0.5f, 0.5f, -0.5f));
    positions.push_back(glm::vec3(0.5f, 0.5f, -0.5f));
    positions.push_back(glm::vec3(-0.5f, 0.5f, -0.5f));
    positions.push_back(glm::vec3(-0.5f, -0.5f, -0.5f));

    positions.push_back(glm::vec3(-0.5f, -0.5f, 0.5f));
    positions.push_back(glm::vec3(0.5f, -0.5f, 0.5f));
    positions.push_back(glm::vec3(0.5f, 0.5f, 0.5f));
    positions.push_back(glm::vec3(0.5f, 0.5f, 0.5f));
    positions.push_back(glm::vec3(-0.5f, 0.5f, 0.5f));
    positions.push_back(glm::vec3(-0.5f, -0.5f, 0.5f));

    positions.push_back(glm::vec3(-0.5f, 0.5f, 0.5f));
    positions.push_back(glm::vec3(-0.5f, 0.5f, -0.5f));
    positions.push_back(glm::vec3(-0.5f, -0.5f, -0.5f));
    positions.push_back(glm::vec3(-0.5f, -0.5f, -0.5f));
    positions.push_back(glm::vec3(-0.5f, -0.5f, 0.5f));
    positions.push_back(glm::vec3(-0.5f, 0.5f, 0.5f));

    positions.push_back(glm::vec3(0.5f, 0.5f, 0.5f));
    positions.push_back(glm::vec3(0.5f, 0.5f, -0.5f));
    positions.push_back(glm::vec3(0.5f, -0.5f, -0.5f));
    positions.push_back(glm::vec3(0.5f, -0.5f, -0.5f));
    positions.push_back(glm::vec3(0.5f, -0.5f, 0.5f));
    positions.push_back(glm::vec3(0.5f, 0.5f, 0.5f));

    positions.push_back(glm::vec3(-0.5f, -0.5f, -0.5f));
    positions.push_back(glm::vec3(0.5f, -0.5f, -0.5f));
    positions.push_back(glm::vec3(0.5f, -0.5f, 0.5f));
    positions.push_back(glm::vec3(0.5f, -0.5f, 0.5f));
    positions.push_back(glm::vec3(-0.5f, -0.5f, 0.5f));
    positions.push_back(glm::vec3(-0.5f, -0.5f, -0.5f));

    positions.push_back(glm::vec3(-0.5f, 0.5f, -0.5f));
    positions.push_back(glm::vec3(0.5f, 0.5f, -0.5f));
    positions.push_back(glm::vec3(0.5f, 0.5f, 0.5f));
    positions.push_back(glm::vec3(0.5f, 0.5f, 0.5f));
    positions.push_back(glm::vec3(-0.5f, 0.5f, 0.5f));
    positions.push_back(glm::vec3(-0.5f, 0.5f, -0.5f));

    // Normals of cube vertices
    vector<glm::vec3> normals;

    normals.push_back(glm::vec3(0.0f, 0.0f, -1.0f));
    normals.push_back(glm::vec3(0.0f, 0.0f, -1.0f));
    normals.push_back(glm::vec3(0.0f, 0.0f, -1.0f));
    normals.push_back(glm::vec3(0.0f, 0.0f, -1.0f));
    normals.push_back(glm::vec3(0.0f, 0.0f, -1.0f));
    normals.push_back(glm::vec3(0.0f, 0.0f, -1.0f));

    normals.push_back(glm::vec3(0.0f, 0.0f, +1.0f));
    normals.push_back(glm::vec3(0.0f, 0.0f, +1.0f));
    normals.push_back(glm::vec3(0.0f, 0.0f, +1.0f));
    normals.push_back(glm::vec3(0.0f, 0.0f, +1.0f));
    normals.push_back(glm::vec3(0.0f, 0.0f, +1.0f));
    normals.push_back(glm::vec3(0.0f, 0.0f, +1.0f));

    normals.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    normals.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    normals.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    normals.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    normals.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    normals.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));

    normals.push_back(glm::vec3(+1.0f, 0.0f, 0.0f));
    normals.push_back(glm::vec3(+1.0f, 0.0f, 0.0f));
    normals.push_back(glm::vec3(+1.0f, 0.0f, 0.0f));
    normals.push_back(glm::vec3(+1.0f, 0.0f, 0.0f));
    normals.push_back(glm::vec3(+1.0f, 0.0f, 0.0f));
    normals.push_back(glm::vec3(+1.0f, 0.0f, 0.0f));

    normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));

    normals.push_back(glm::vec3(0.0f, +1.0f, 0.0f));
    normals.push_back(glm::vec3(0.0f, +1.0f, 0.0f));
    normals.push_back(glm::vec3(0.0f, +1.0f, 0.0f));
    normals.push_back(glm::vec3(0.0f, +1.0f, 0.0f));
    normals.push_back(glm::vec3(0.0f, +1.0f, 0.0f));
    normals.push_back(glm::vec3(0.0f, +1.0f, 0.0f));

    // Create a mesh from positions and normals
    UCreateMesh(mesh, positions, normals);
}

// Position of vertex(i) on a circle with n subdivisions
glm::vec3 UCircularCoordinates(int i, int n)
{
    // Angle for vertex(i) with n subdivisions
    float angle = glm::radians(360.0f * i / n);

    // Rotate the initial vertex (1, 0, 0) around the Y axis
    glm::vec4 vertex(1.0f, 0.0f, 0.0f, 1.0f);
    vertex = glm::rotate(angle, glm::vec3(0.0, 1.0f, 0.0f)) * vertex;
    return glm::vec3(vertex);
}

// Creates a cylinder mesh
void UCreateCylinderMesh(GLMesh& mesh)
{
    // Positions and normals of cylinder vertices
    vector<glm::vec3> positions;
    vector<glm::vec3> normals;

    // Subdivide the cylinder into n tiles
    const int n = 16;
    for (int i = 0; i < n; i++)
    {
        // Vertices of the current tile
        glm::vec3 a = UCircularCoordinates(i + 0, n);
        glm::vec3 b = UCircularCoordinates(i + 1, n);

        // 1st triangle of the tile
        positions.push_back(a - glm::vec3(0.0f, 0.5f, 0.0f));
        positions.push_back(b - glm::vec3(0.0f, 0.5f, 0.0f));
        positions.push_back(a + glm::vec3(0.0f, 0.5f, 0.0f));
        normals.push_back(a);
        normals.push_back(b);
        normals.push_back(a);

        // 2nd triangle of the tile
        positions.push_back(b - glm::vec3(0.0f, 0.5f, 0.0f));
        positions.push_back(b + glm::vec3(0.0f, 0.5f, 0.0f));
        positions.push_back(a + glm::vec3(0.0f, 0.5f, 0.0f));
        normals.push_back(b);
        normals.push_back(b);
        normals.push_back(a);
    }

    // Create a mesh from positions and normals
    UCreateMesh(mesh, positions, normals);
}

// Position of vertex(i, j) on a circle with n*n subdivisions
glm::vec3 USphericalCoordinates(int i, int j, int n)
{
    // Longitude and latitude angles for vertex(i, j) with n*n subdivisions
    float longitude = glm::radians(360.0f * j / n);
    float latitude = glm::radians(180.0f * i / n) - glm::radians(90.0f);

    // Rotate the initial vertex (1, 0, 0) around the Z and Y axes
    glm::vec4 vertex(1.0f, 0.0f, 0.0f, 1.0f);
    vertex = glm::rotate(latitude, glm::vec3(0.0, 0.0f, 1.0f)) * vertex;
    vertex = glm::rotate(longitude, glm::vec3(0.0, 1.0f, 0.0f)) * vertex;
    return glm::vec3(vertex);
}

// Creates a sphere mesh
void UCreateSphereMesh(GLMesh& mesh)
{
    // Positions and normals of sphere vertices
    vector<glm::vec3> positions;
    vector<glm::vec3> normals;

    // Subdivide the sphere into n*n tiles
    const int n = 8;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
        {
            // Vertices of the current tile
            glm::vec3 a = USphericalCoordinates(i + 0, j + 0, n);
            glm::vec3 b = USphericalCoordinates(i + 0, j + 1, n);
            glm::vec3 c = USphericalCoordinates(i + 1, j + 1, n);
            glm::vec3 d = USphericalCoordinates(i + 1, j + 0, n);

            // 1st triangle of the tile
            positions.push_back(a);
            positions.push_back(b);
            positions.push_back(c);
            normals.push_back(a);
            normals.push_back(b);
            normals.push_back(c);

            // 2nd triangle of the tile
            positions.push_back(a);
            positions.push_back(c);
            positions.push_back(d);
            normals.push_back(a);
            normals.push_back(c);
            normals.push_back(d);
        }

    // Create a mesh from positions and normals
    UCreateMesh(mesh, positions, normals);
}

void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}
