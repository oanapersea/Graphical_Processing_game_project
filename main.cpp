#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::mat4 light_direction_matrix;

// light parameters
glm::vec3 light_dir;
glm::vec3 light_color;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;

GLboolean cameraMovement = true;

GLint loc_light_direction_matrix;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.3f;

GLboolean pressedKeys[1024];

// models
gps::Model3D scene;
gps::Model3D rain;
gps::Model3D medit;
GLfloat angleMedit = 0;

// shaders
gps::Shader myBasicShader;
gps::Shader skybox_shader;
gps::Shader light_shader;
gps::Shader depthMap_shader;


// for moving objects
glm::mat4 medit_model;
glm::mat3 medit_matrix_normal;
glm::mat4 rain_model;
glm::mat3 rain_normal_matrix;

const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

bool night=true;
bool nightBut=false;

bool fog=false;

bool light=false;
bool lightBut=false;

float rainStartVal = 0.0f;
float rainAddVal = 0.35f;

float mouse_sensitivity = 0.07f;
float rotationSpeed = 0.8f;

float cameraAngle = 270;
float yaw = -90, pitch;

// fog---------------------
float fogDensity = 0;
GLint log_fogDensity;

// skybox-----------------------
gps::SkyBox skyboxDay, skyboxNight;


// shadows-----------------------
GLuint shadowMapFBO;
GLuint depthMapTexture;

// point lights-----------------
glm::vec3 point_light_1;

GLuint loc_point_light_1;

GLuint loc_point_light_color;
glm::vec3 point_light_color;

GLenum glCheckError_(const char *file, int line)
{
      GLenum errorCode;
      while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_STACK_OVERFLOW:
                error = "STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                error = "STACK_UNDERFLOW";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
          std::cout << error << " | " << file << " (" << line << ")" << std::endl;
  }
  return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
    //TODO
}


void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void cameraAnimation()
{
    myCamera.move(gps::MOVE_FORWARD, cameraSpeed/2);
    //update view matrix
    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    yaw -= 0.2f;
    myCamera.rotate(pitch, yaw);
    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
}

void mouseCallback(GLFWwindow *window, double xpos, double ypos)
{
      int windowWidth, windowHeight;
      glfwGetWindowSize(window, &windowWidth, &windowHeight);

      float xOffset = xpos - windowWidth / 2.0f;
      float yOffset = windowHeight / 2.0f - ypos;

      xOffset *= mouse_sensitivity;
      yOffset *= mouse_sensitivity;

      yaw += xOffset;
      pitch += yOffset;

      if (pitch > 89.0f) pitch = 89.0f;
      if (pitch < -89.0f) pitch = -89.0f;

      myCamera.rotate(pitch, yaw);

      view = myCamera.getViewMatrix();
      myBasicShader.useShaderProgram();
      glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

      normalMatrix = glm::mat3(glm::inverseTranspose(view * model));

      glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);
  
}

void processMovement()
{
  if (pressedKeys[GLFW_KEY_W]) {
    myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
    //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
  }

  if (pressedKeys[GLFW_KEY_S]) {
    myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
  }

  if (pressedKeys[GLFW_KEY_A]) {
    myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
  }

  if (pressedKeys[GLFW_KEY_D]) {
    myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
  }

    if (pressedKeys[GLFW_KEY_Q]) {
        cameraAngle -= rotationSpeed;
        if (cameraAngle < 0.0f)
          cameraAngle += 360.0f;
        myCamera.rotate(0, cameraAngle);
        //doar pana aici difera

        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        cameraAngle += rotationSpeed;
        if (cameraAngle > 360.0f)
          cameraAngle -= 360.0f;
        myCamera.rotate(0, cameraAngle);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }
  
  if (pressedKeys[GLFW_KEY_F])
  {
    fog = true;
  }
  else
  {
    if (fog)
    {
      fogDensity += 0.005;
      fogDensity = fogDensity > 0.04 ? 0 : fogDensity;
      myBasicShader.useShaderProgram();

      // fogDensity density location
      log_fogDensity = glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity");
      glUniform1f(log_fogDensity, (GLfloat)fogDensity);
    }
    fog = false;
  }
  
  if (pressedKeys[GLFW_KEY_L])
  {
    lightBut = true;
  }
  else if (lightBut)
  {

    myBasicShader.useShaderProgram();

    light = !light;
    point_light_color = light ? glm::vec3(0.0f, 1.0f, 1.0f) : glm::vec3(0.0f, 0.0f, 0.0f);

    loc_point_light_color = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor");
    glUniform3fv(loc_point_light_color, 1, glm::value_ptr(point_light_color));
    lightBut = false;
  }

  if (pressedKeys[GLFW_KEY_N])
  {
    nightBut= true;
  }
  else if (nightBut)
  {
    myBasicShader.useShaderProgram();

    night = !night;
    light_color = night ? glm::vec3(1.0f, 1.0f, 1.0f) : glm::vec3(1.03f, 0.07f, 0.3f);

    lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(light_color));
    nightBut = false;
  }
  
  if (pressedKeys[GLFW_KEY_T]) {
      cameraMovement = false;
  }
  
  if(pressedKeys[GLFW_KEY_1]){
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
  }
  
  if(pressedKeys[GLFW_KEY_2]){
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
  
  if(pressedKeys[GLFW_KEY_3]){
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }

}

void initOpenGLWindow()
{
  myWindow.Create(1200, 768, "OpenGL Project Core");
}

void setWindowCallbacks()
{
    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

void initOpenGLState()
{
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_BACK); // cull back face
    glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}


void initModels()
{
  scene.LoadModel("/Users/perseaoana/Downloads/OpenGL_Project_Core_final/models/teren23.obj");
  rain.LoadModel("/Users/perseaoana/Downloads/OpenGL_Project_Core_final/models/rain.obj");
  medit.LoadModel("/Users/perseaoana/Downloads/OpenGL_Project_Core_final/models/untitled.obj");
}

///////shadere

void initShaders()
{
  myBasicShader.loadShader(
      "/Users/perseaoana/Downloads/OpenGL_Project_Core_final/shaders/basic.vert",
      "/Users/perseaoana/Downloads/OpenGL_Project_Core_final/shaders/basic.frag");
  skybox_shader.loadShader(
      "/Users/perseaoana/Downloads/OpenGL_Project_Core_final/shaders/skybox_shader.vert",
      "/Users/perseaoana/Downloads/OpenGL_Project_Core_final/shaders/skybox_shader.frag");
  light_shader.loadShader(
      "/Users/perseaoana/Downloads/OpenGL_Project_Core_final/shaders/light_shader.vert",
      "/Users/perseaoana/Downloads/OpenGL_Project_Core_final/shaders/light_shader.frag");
  depthMap_shader.loadShader(
      "/Users/perseaoana/Downloads/OpenGL_Project_Core_final/shaders/depthMap_shader.vert",
      "/Users/perseaoana/Downloads/OpenGL_Project_Core_final/shaders/depthMap_shader.frag");
}


void initFBO()
{
  // generate FBO ID
  glGenFramebuffers(1, &shadowMapFBO);

  // create depth texture for FBO
  glGenTextures(1, &depthMapTexture);
  glBindTexture(GL_TEXTURE_2D, depthMapTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
               SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);


  // attach texture to FBO
  glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
  
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initUniforms()
{
  myBasicShader.useShaderProgram();

  loc_light_direction_matrix = glGetUniformLocation(myBasicShader.shaderProgram, "lightDirMatrix"); // #######

  // get view matrix for current camera
  view = myCamera.getViewMatrix();
  viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
  // send view matrix to shader
  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));


  // create projection matrix
  projection = glm::perspective(glm::radians(45.0f),
                                (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                                0.1f, 300.0f);
  projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
  // send projection matrix to shader
  glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

  // set light direction
  light_dir = glm::vec3(0.0f, 150.0f, 150.0f);
  lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
  // send light dir to shader
  glUniform3fv(lightDirLoc, 1, glm::value_ptr(light_dir));

  // set light color
  light_color = glm::vec3(1.0f, 1.0f, 1.0f); // white light
  lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
  // send light color to shader
  glUniform3fv(lightColorLoc, 1, glm::value_ptr(light_color));

   //point light color
  point_light_color = glm::vec3(0.0f, 0.0f, 0.0f); // point lights start as off
  loc_point_light_color = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor");
  // send light color to shader
  glUniform3fv(loc_point_light_color, 1, glm::value_ptr(point_light_color));

}

glm::mat4 lightSpTrMatCalc()
{
  const GLfloat near_plane = 35.0f, far_plane = 700.0f;
  glm::mat4 lightProjection = glm::ortho(-150.0f, 150.0f, -150.0f, 150.0f, near_plane, far_plane);

  glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)0), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(light_dir, 1.0f));

  glm::mat4 lightView = glm::lookAt(lightDirTr, myCamera.getCameraTarget(), glm::vec3(0.0f, 1.0f, 0.0f));

  return lightProjection * lightView;
  
  
}


void initSkyBox()
{
  std::vector<const GLchar *> faces;
  faces.push_back("/Users/perseaoana/Downloads/OpenGL_Project_Core_final/textures/skybox/day/seeingred_rt.tga");
  faces.push_back("/Users/perseaoana/Downloads/OpenGL_Project_Core_final/textures/skybox/day/seeingred_lf.tga");
  faces.push_back("/Users/perseaoana/Downloads/OpenGL_Project_Core_final/textures/skybox/day/seeingred_up.tga");
  faces.push_back("/Users/perseaoana/Downloads/OpenGL_Project_Core_final/textures/skybox/day/seeingred_dn.tga");
  faces.push_back("/Users/perseaoana/Downloads/OpenGL_Project_Core_final/textures/skybox/day/seeingred_bk.tga");
  faces.push_back("/Users/perseaoana/Downloads/OpenGL_Project_Core_final/textures/skybox/day/seeingred_ft.tga");

  skyboxDay.Load(faces);

  std::vector<const GLchar *> faces2;
  faces2.push_back("/Users/perseaoana/Downloads/OpenGL_Project_Core_final/textures/skybox/night/nightsky_rt.tga");
  faces2.push_back("/Users/perseaoana/Downloads/OpenGL_Project_Core_final/textures/skybox/night/nightsky_lf.tga");
  faces2.push_back("/Users/perseaoana/Downloads/OpenGL_Project_Core_final/textures/skybox/night/nightsky_up.tga");
  faces2.push_back("/Users/perseaoana/Downloads/OpenGL_Project_Core_final/textures/skybox/night/nightsky_dn.tga");
  faces2.push_back("/Users/perseaoana/Downloads/OpenGL_Project_Core_final/textures/skybox/night/nightsky_bk.tga");
  faces2.push_back("/Users/perseaoana/Downloads/OpenGL_Project_Core_final/textures/skybox/night/nightsky_ft.tga");

  skyboxNight.Load(faces2);
}


void renderTeren(gps::Shader shader)
{
  // select active shader program
  shader.useShaderProgram();

  // send scene model matrix data to shader
  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

  // send scene normal matrix data to shader
  glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

  // draw teapot
  scene.Draw(shader);
}



void renderRain(gps::Shader shader)
{

  shader.useShaderProgram();

  // send scene model matrix data to shader
  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rain_model));

  // send scene normal matrix data to shader
  glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(rain_normal_matrix));

  rain.Draw(shader);
}

void renderMedit(gps::Shader shader)
{
  shader.useShaderProgram();

  // send scene model matrix data to shader
  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(medit_model));

  // send scene normal matrix data to shader
  glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(medit_matrix_normal));

  medit.Draw(shader);
}

void renderScene()
{

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  depthMap_shader.useShaderProgram();

  // compute shadows for directional light
  glUniformMatrix4fv(glGetUniformLocation(depthMap_shader.shaderProgram, "lightSpaceTrMatrix"),
                     1, GL_FALSE, glm::value_ptr(lightSpTrMatCalc()));

  glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
  glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);

  glClear(GL_DEPTH_BUFFER_BIT);


  depthMap_shader.useShaderProgram();
  glm::mat4 model_shadow = glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)0), glm::vec3(0.0, 1.0, 0.0));
  glUniformMatrix4fv(glGetUniformLocation(depthMap_shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model_shadow));
  renderTeren(depthMap_shader);


  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // 2nd step: render the scene

  myBasicShader.useShaderProgram();

  // send lightSpace matrix to shader
  glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpTrMatCalc()));

  // send view matrix to shader
  view = myCamera.getViewMatrix();

  glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

  // compute light direction transfor_matrixion matrix
  light_direction_matrix = glm::mat3(glm::inverseTranspose(view));
  // send light_dir matrix data to shader

  loc_light_direction_matrix = glGetUniformLocation(myBasicShader.shaderProgram, "lightDirMatrix");
  glUniformMatrix3fv(loc_light_direction_matrix, 1, GL_FALSE, glm::value_ptr(light_direction_matrix));

  glViewport(0, 0, (int)myWindow.getWindowDimensions().width, (int)myWindow.getWindowDimensions().height);
  myBasicShader.useShaderProgram();

  // bind the depth map
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, depthMapTexture);
  glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

  // render the scene

  model = glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)0), glm::vec3(0.0f, 1.0f, 0.0f));
  modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");
  renderTeren(myBasicShader);

  rain_model = glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  rain_model = glm::translate(rain_model, glm::vec3(-80.0f, -9.0f, -130.0f));
  rain_model = glm::translate(rain_model, glm::vec3(rainStartVal * -0.15f, 0.0f, rainStartVal * 1.0f));
  rain_model = glm::scale(rain_model, glm::vec3(40.0f));

  rain_normal_matrix = glm::mat3(glm::inverseTranspose(view * rain_model));

//   compute next position in the animation

  rainStartVal += rainAddVal;
  if (rainStartVal > 200.0)
    rainAddVal = -rainAddVal;
  if (rainStartVal < 0.0)
    rainAddVal = -rainAddVal;

  renderRain(myBasicShader);

  medit_model = glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  medit_model = glm::translate(medit_model, glm::vec3(5.0f, 10.0f, 60.0f));
  medit_model = glm::scale(medit_model, glm::vec3(10.0f));
  medit_model = glm::rotate(medit_model, glm::radians(angleMedit), glm::vec3(0.0f, 1.0f, 0.0f));


  medit_matrix_normal = glm::mat3(glm::inverseTranspose(view * medit_model));


  angleMedit += 0.3f;
  if (angleMedit > 360.0)
    angleMedit -= 360;

  renderMedit(myBasicShader);

  // render skybox
  if (night)
    skyboxDay.Draw(skybox_shader, view, projection, fogDensity);
  else
    skyboxNight.Draw(skybox_shader, view, projection, fogDensity);
}

void cleanup()
{
  myWindow.Delete();
  // cleanup code for your own data
}

int main(int argc, const char *argv[])
{

  try
  {
    initOpenGLWindow();
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  initOpenGLState();
  initFBO();
  initModels();
  initSkyBox();
  initShaders();
  initUniforms();
  setWindowCallbacks();

  glCheckError();
  // application loop
  while (!glfwWindowShouldClose(myWindow.getWindow()))
  {
    if(cameraMovement)
           cameraAnimation();
    processMovement();
    renderScene();

    glfwPollEvents();
    glfwSwapBuffers(myWindow.getWindow());

  }

  cleanup();

  return EXIT_SUCCESS;
}

