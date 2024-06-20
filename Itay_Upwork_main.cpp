#include <GL/glew.h>
#include <GL/freeglut.h>
#include "imgui.h"
#include "imgui_impl_glut.h"
#include "imgui_impl_opengl2.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <vector>
#include <string>

#ifdef DEBUG
#include <iostream>
#endif

// Robot parts transformations
float robotX = 0.0f, robotY = 0.0f, robotZ = 0.0f;
float robotRotation = 0.0f;
float shoulderPitch = 0.0f, shoulderYaw = 0.0f, shoulderRoll = 0.0f;
float elbowPitch = 0.0f, elbowYaw = 0.0f, elbowRoll = 0.0f;
float wristPitch = 0.0f, wristYaw = 0.0f, wristRoll = 0.0f;
float headYaw = 0.0f, headPitch = 0.0f;
float leftHipAngle = 0.0f, leftKneeAngle = 0.0f;
float rightHipAngle = 0.0f, rightKneeAngle = 0.0f;

// Camera parameters
float camX = 0.0f, camY = 5.0f, camZ = 20.0f;
float camPitch = 0.0f, camYaw = 0.0f;

// Secondary camera (inside robot head)
float headCamYaw = 0.0f, headCamPitch = 0.0f;
float headCamX = 0.0f, headCamY = 1.5f, headCamZ = 0.0f;
float secCamX = 0.0f, secCamY = 1.5f, secCamZ = 0.0f;
bool useHeadCam = false;
bool headVisible = true;  // New variable to control head visibility

// Lighting parameters
float lightPos[] = { 1.2f, 7.5f, 2.0f, 1.0f };
float ambientStrength = 0.25f;
float pointLightIntensity = 1.0f;
float lightAngle = 0.0f;

// Window size
int windowWidth = 1280;
int windowHeight = 720;
bool show_help_window = false;

ImFont* smallFont, * font;
GLuint cubemapTexture;

// Animation parameters
bool isMoving = false;
float walkCycle = 0.0f;

GLuint floorTexture;

// Material properties
GLfloat floorSpecular[] = { 0.9f, 0.9f, 0.9f, 1.0f };
GLfloat floorShininess = 100.0f;
GLfloat floorDiffuse[] = { 0.5f, 0.5f, 0.5f, 1.0f };

GLfloat cubeDiffuse[] = { 0.6f, 0.2f, 0.3f , 1.0f };
GLfloat cubeSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat cubeShininess = 64.0f;

GLfloat plasticDiffuse[] = { 0.9f, 0.8f, 0.1f, 1.0f };
GLfloat plasticSpecular[] = { 0.3f, 0.3f, 0.3f, 1.0f };
GLfloat plasticShininess = 5.0f;

GLfloat teapotDiffuse[] = { 0.804f, 0.498f, 0.196f, 1.0f };
GLfloat teapotSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat teapotShininess = 200.0f;

GLfloat robotDiffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
GLfloat robotSpecular[] = { 0.9f, 0.9f, 0.9f, 1.0f };
GLfloat robotShininess = 128.0f;

enum Direction
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};
Direction currentDirection = FORWARD;

void setupLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat ambientLight[] = { ambientStrength, ambientStrength, ambientStrength, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);

    GLfloat diffuseLight[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat specularLight[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
}

bool loadTexture(const char* filepath, GLuint& textureID)
{
    int width, height, channels;
    unsigned char* data = stbi_load(filepath, &width, &height, &channels, 0);
    if (data)
    {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, channels == 3 ? GL_RGB : GL_RGBA, width, height, 0, channels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        return true;
    }
    else
    {
#ifdef DEBUG
        std::cerr << "Failed to load texture: " << filepath << std::endl;
#endif
        return false;
    }
}

bool loadCubemapTexture(const std::vector<std::string>& faces, GLuint& textureID)
{
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, channels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &channels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
#ifdef DEBUG
            std::cerr << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
#endif
            stbi_image_free(data);
            return false;
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return true;
}

void loadTextures()
{
    loadTexture("Assets/tiles_0006_color_1k.jpg", floorTexture);

    std::vector<std::string> faces
    {
        "Assets/field-skyboxes/right.bmp",
        "Assets/field-skyboxes/left.bmp",
        "Assets/field-skyboxes/top.bmp",
        "Assets/field-skyboxes/bottom.bmp",
        "Assets/field-skyboxes/front.bmp",
        "Assets/field-skyboxes/back.bmp"
    };

    loadCubemapTexture(faces, cubemapTexture);
}

void drawLightBox()
{
    glPushMatrix();
    glTranslatef(lightPos[0], lightPos[1], lightPos[2]);

    GLfloat prevMaterial[4];
    glGetMaterialfv(GL_FRONT, GL_AMBIENT, prevMaterial);

    GLfloat yellow[] = { 1.0f, 1.0f, 0.0f, 1.0f };
    GLfloat emission[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glMaterialfv(GL_FRONT, GL_AMBIENT, yellow);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, yellow);
    glMaterialfv(GL_FRONT, GL_SPECULAR, yellow);
    glMaterialfv(GL_FRONT, GL_EMISSION, emission);
    glMaterialf(GL_FRONT, GL_SHININESS, 50.0f);

    glutSolidCube(0.2f);

    glMaterialfv(GL_FRONT, GL_AMBIENT, prevMaterial);
    glMaterialfv(GL_FRONT, GL_EMISSION, prevMaterial);

    glPopMatrix();
}

void drawRobotHead()
{
    if (!headVisible)
        return;

    glPushMatrix();
    glTranslatef(0.0f, 1.75f, 0.0f);
    glRotatef(headYaw, 0.0f, 1.0f, 0.0f);
    glRotatef(headPitch, 1.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glutSolidSphere(0.5f, 20, 20);

    glPushMatrix();
    glColor3f(1.0f, 1.0f, 1.0f);
    glTranslatef(0.2f, 0.1f, -0.45f);
    glutSolidSphere(0.1f, 20, 20);
    glTranslatef(-0.4f, 0.0f, 0.0f);
    glutSolidSphere(0.1f, 20, 20);
    glPopMatrix();

    glPopMatrix();
}

void drawLimb(float length, float radius)
{
    GLUquadric* quadric = gluNewQuadric();
    gluCylinder(quadric, radius, radius, length, 20, 20);
    gluDeleteQuadric(quadric);
}

void drawJoint(float radius)
{
    glutSolidSphere(radius, 20, 20);
}

void drawRightArm()
{
    glPushMatrix();
    glTranslatef(0.65f, 1.0f, 0.0f);

    glm::quat shoulderQuaternion = glm::angleAxis(glm::radians(shoulderYaw), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::angleAxis(glm::radians(shoulderPitch), glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::angleAxis(glm::radians(shoulderRoll), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 shoulderRotation = glm::toMat4(shoulderQuaternion);

    glMultMatrixf(glm::value_ptr(shoulderRotation));
    glColor3f(0.0f, 1.0f, 0.0f);
    drawJoint(0.25f);

    glPushMatrix();
    glTranslatef(0.0f, -0.25f, 0.0f);
    glRotatef(90, 1.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    drawLimb(0.25f, 0.1f);
    glPopMatrix();

    glTranslatef(0.0f, -0.5f, 0.0f);

    glm::quat elbowQuaternion = glm::angleAxis(glm::radians(elbowYaw), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::angleAxis(glm::radians(elbowPitch), glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::angleAxis(glm::radians(elbowRoll), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 elbowRotation = glm::toMat4(elbowQuaternion);

    glMultMatrixf(glm::value_ptr(elbowRotation));
    glColor3f(0.0f, 1.0f, 0.0f);
    drawJoint(0.2f);

    glPushMatrix();
    glTranslatef(0.0f, -0.25f, 0.0f);
    glRotatef(90, 1.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    drawLimb(0.25f, 0.1f);
    glPopMatrix();

    glTranslatef(0.0f, -0.5f, 0.0f);

    glm::quat wristQuaternion = glm::angleAxis(glm::radians(wristYaw), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::angleAxis(glm::radians(wristPitch), glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::angleAxis(glm::radians(wristRoll), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 wristRotation = glm::toMat4(wristQuaternion);

    glMultMatrixf(glm::value_ptr(wristRotation));
    glColor3f(0.0f, 1.0f, 0.0f);
    drawJoint(0.15f);

    glPushMatrix();
    glTranslatef(0.0f, -0.1f, 0.0f);
    glRotatef(90, 1.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    drawLimb(0.2f, 0.05f);
    glPopMatrix();

    glPopMatrix();
}

void drawLeg(float hipAngle, float kneeAngle, float translateX, float translateY, float translateZ)
{
    glPushMatrix();
    glTranslatef(translateX, translateY, translateZ);
    glRotatef(hipAngle, 1.0f, 0.0f, 0.0f);

    glColor3f(0.0f, 1.0f, 0.0f);
    drawJoint(0.2f);

    glPushMatrix();
    glTranslatef(0.0f, -0.1f, 0.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    drawLimb(0.35f, 0.2f);
    glPopMatrix();

    glTranslatef(0.0f, -0.55f, 0.0f);
    glRotatef(kneeAngle, 1.0f, 0.0f, 0.0f);

    glColor3f(0.0f, 1.0f, 0.0f);
    drawJoint(0.18f);

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    drawLimb(0.35f, 0.16f);
    glPopMatrix();

    glPopMatrix();
}

void drawNeck()
{
    glTranslatef(0.0f, 1.125f, 0.0f);
    drawJoint(0.25f);
}

void drawRobot()
{
    glMaterialfv(GL_FRONT, GL_DIFFUSE, robotDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, robotSpecular);
    glMaterialf(GL_FRONT, GL_SHININESS, robotShininess);

    glPushMatrix();
    glTranslatef(robotX, robotY, robotZ);
    glRotatef(robotRotation, 0.0f, 1.0f, 0.0f);

    drawLeg(leftHipAngle, leftKneeAngle, -0.25f, 0.0f, 0.0f); // Left leg
    drawLeg(rightHipAngle, rightKneeAngle, 0.25f, 0.0f, 0.0f); // Right leg

    glPushMatrix();
    glTranslatef(0.0f, 0.9f, 0.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    drawLimb(0.75f, 0.15f);
    glPopMatrix();

    drawJoint(0.18f);

    drawRobotHead();
    drawRightArm();

    drawNeck();

    glPopMatrix();
}

void drawFloor()
{
    glMaterialfv(GL_FRONT, GL_SPECULAR, floorSpecular);
    glMaterialf(GL_FRONT, GL_SHININESS, 128.0f - floorShininess);  // Adjust shininess correctly
    glMaterialfv(GL_FRONT, GL_DIFFUSE, floorDiffuse);

    glPushMatrix();
    glTranslatef(0.0f, -0.9f, 0.0f);

    // Bind floor texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floorTexture);

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    for (int i = -10; i < 10; ++i)
    {
        for (int j = -10; j < 10; ++j)
        {
            glTexCoord2f(0.0f, 0.0f); glVertex3f(i, 0.0f, j);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(i + 1, 0.0f, j);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(i + 1, 0.0f, j + 1);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(i, 0.0f, j + 1);
        }
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
}

void drawPlasticSphere()
{
    glMaterialfv(GL_FRONT, GL_DIFFUSE, plasticDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, plasticSpecular);
    glMaterialf(GL_FRONT, GL_SHININESS, plasticShininess);

    glPushMatrix();
    glTranslatef(-7.0f, 0.0f, 0.0f);
    glutSolidSphere(0.5f, 20, 20);
    glPopMatrix();
}

void drawTexturedCube()
{
    glMaterialfv(GL_FRONT, GL_DIFFUSE, cubeDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, cubeSpecular);
    glMaterialf(GL_FRONT, GL_SHININESS, cubeShininess);

    glPushMatrix();
    glTranslatef(2.0f, 0.0f, -10.0f);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawMetalTeapot()
{
    glMaterialfv(GL_FRONT, GL_SPECULAR, teapotSpecular);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, teapotDiffuse);
    glMaterialf(GL_FRONT, GL_SHININESS, teapotShininess);

    glPushMatrix();
    glTranslatef(-4.0f, 0.0f, -1.0f);
    glutSolidTeapot(1.0);
    glPopMatrix();
}

void drawSkybox()
{
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    glBegin(GL_QUADS);
    glTexCoord3f(-1.0f, 1.0f, -1.0f); glVertex3f(-50.0f, 50.0f, -50.0f);
    glTexCoord3f(-1.0f, -1.0f, -1.0f); glVertex3f(-50.0f, -50.0f, -50.0f);
    glTexCoord3f(1.0f, -1.0f, -1.0f); glVertex3f(50.0f, -50.0f, -50.0f);
    glTexCoord3f(1.0f, 1.0f, -1.0f); glVertex3f(50.0f, 50.0f, -50.0f);

    glTexCoord3f(1.0f, 1.0f, 1.0f); glVertex3f(-50.0f, 50.0f, 50.0f);
    glTexCoord3f(1.0f, -1.0f, 1.0f); glVertex3f(-50.0f, -50.0f, 50.0f);
    glTexCoord3f(-1.0f, -1.0f, 1.0f); glVertex3f(50.0f, -50.0f, 50.0f);
    glTexCoord3f(-1.0f, 1.0f, 1.0f); glVertex3f(50.0f, 50.0f, 50.0f);

    glTexCoord3f(-1.0f, 1.0f, 1.0f); glVertex3f(-50.0f, 50.0f, -50.0f);
    glTexCoord3f(1.0f, 1.0f, 1.0f); glVertex3f(50.0f, 50.0f, -50.0f);
    glTexCoord3f(1.0f, 1.0f, -1.0f); glVertex3f(50.0f, 50.0f, 50.0f);
    glTexCoord3f(-1.0f, 1.0f, -1.0f); glVertex3f(-50.0f, 50.0f, 50.0f);

    glTexCoord3f(-1.0f, -1.0f, -1.0f); glVertex3f(-50.0f, -50.0f, -50.0f);
    glTexCoord3f(1.0f, -1.0f, -1.0f); glVertex3f(50.0f, -50.0f, -50.0f);
    glTexCoord3f(1.0f, -1.0f, 1.0f); glVertex3f(50.0f, -50.0f, 50.0f);
    glTexCoord3f(-1.0f, -1.0f, 1.0f); glVertex3f(-50.0f, -50.0f, 50.0f);

    glTexCoord3f(1.0f, -1.0f, -1.0f); glVertex3f(50.0f, -50.0f, -50.0f);
    glTexCoord3f(1.0f, -1.0f, 1.0f); glVertex3f(50.0f, -50.0f, 50.0f);
    glTexCoord3f(1.0f, 1.0f, 1.0f); glVertex3f(50.0f, 50.0f, 50.0f);
    glTexCoord3f(1.0f, 1.0f, -1.0f); glVertex3f(50.0f, 50.0f, -50.0f);

    glTexCoord3f(-1.0f, -1.0f, 1.0f); glVertex3f(-50.0f, -50.0f, 50.0f);
    glTexCoord3f(-1.0f, -1.0f, -1.0f); glVertex3f(-50.0f, -50.0f, -50.0f);
    glTexCoord3f(-1.0f, 1.0f, -1.0f); glVertex3f(-50.0f, 50.0f, -50.0f);
    glTexCoord3f(-1.0f, 1.0f, 1.0f); glVertex3f(-50.0f, 50.0f, 50.0f);

    glEnd();

    glDisable(GL_TEXTURE_CUBE_MAP);
    glDepthFunc(GL_LESS);
}

void renderScene()
{
    setupLighting();
    drawFloor();
    drawRobot();
    drawLightBox();
    drawPlasticSphere();
    drawTexturedCube();
    drawMetalTeapot();
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)windowWidth / (float)windowHeight, 0.1f, 1000.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (useHeadCam)
    {
        // Calculate the robot's rotation matrix
        glm::mat4 robotRotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(robotRotation), glm::vec3(0.0f, 1.0f, 0.0f));

        // Transform the secondary camera's position using the robot's rotation matrix
        glm::vec4 transformedSecCamPos = robotRotationMatrix * glm::vec4(secCamX, secCamY, secCamZ, 1.0f);
        glm::vec4 transformedLookDir = robotRotationMatrix * glm::vec4(
            sin(glm::radians(headCamYaw)) * cos(glm::radians(headCamPitch)),
            sin(glm::radians(headCamPitch)),
            -cos(glm::radians(headCamYaw)) * cos(glm::radians(headCamPitch)),
            0.0f
        );

        float eyeX = robotX + transformedSecCamPos.x;
        float eyeY = robotY + transformedSecCamPos.y;
        float eyeZ = robotZ + transformedSecCamPos.z;
        float lookX = eyeX + transformedLookDir.x;
        float lookY = eyeY + transformedLookDir.y;
        float lookZ = eyeZ + transformedLookDir.z;

        gluLookAt(eyeX, eyeY, eyeZ, lookX, lookY, lookZ, 0.0f, 1.0f, 0.0f);
    }
    else
    {
        gluLookAt(camX, camY, camZ, camX + sin(camYaw), camY + sin(camPitch), camZ - cos(camYaw), 0.0f, 1.0f, 0.0f);
    }

    glPushMatrix();
    glTranslatef(camX, camY, camZ);
    drawSkybox();
    glPopMatrix();

    renderScene();

    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(windowWidth - 300, 0));
    ImGui::SetNextWindowSize(ImVec2(300, windowHeight));
    ImGui::Begin("Robot Control", NULL, ImGuiWindowFlags_NoMove);

    ImGui::Text("Robot Position");
    ImGui::PushFont(smallFont);
    ImGui::Text("X");
    ImGui::SameLine();
    ImGui::SliderFloat("##Robot Position X", &robotX, -10.0f, 10.0f);
    ImGui::Text("Y");
    ImGui::SameLine();
    ImGui::SliderFloat("##Robot Position Y", &robotY, -10.0f, 10.0f);
    ImGui::Text("Z");
    ImGui::SameLine();
    ImGui::SliderFloat("##Robot Position Z", &robotZ, -10.0f, 10.0f);
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, 7.0f));
    ImGui::Text("Right-hand Angles");
    ImGui::PushFont(smallFont);
    ImGui::Text("Shoulder");
    ImGui::Text("Pitch"); ImGui::SameLine();
    ImGui::SliderFloat("##Shoulder Pitch", &shoulderPitch, -180.0f, 180.0f);
    ImGui::Text("Yaw"); ImGui::SameLine();
    ImGui::SliderFloat("##Shoulder Yaw", &shoulderYaw, -180.0f, 180.0f);
    ImGui::Text("Roll"); ImGui::SameLine();
    ImGui::SliderFloat("##Shoulder Roll", &shoulderRoll, -180.0f, 180.0f);
    ImGui::Text("Elbow");
    ImGui::Text("Pitch"); ImGui::SameLine();
    ImGui::SliderFloat("##Elbow Pitch", &elbowPitch, -180.0f, 180.0f);
    ImGui::Text("Yaw"); ImGui::SameLine();
    ImGui::SliderFloat("##Elbow Yaw", &elbowYaw, -180.0f, 180.0f);
    ImGui::Text("Roll"); ImGui::SameLine();
    ImGui::SliderFloat("##Elbow Roll", &elbowRoll, -180.0f, 180.0f);
    ImGui::Text("Wrist");
    ImGui::Text("Pitch"); ImGui::SameLine();
    ImGui::SliderFloat("##Wrist Pitch", &wristPitch, -180.0f, 180.0f);
    ImGui::Text("Yaw"); ImGui::SameLine();
    ImGui::SliderFloat("##Wrist Yaw", &wristYaw, -180.0f, 180.0f);
    ImGui::Text("Roll"); ImGui::SameLine();
    ImGui::SliderFloat("##Wrist Roll", &wristRoll, -180.0f, 180.0f);
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, 7.0f));
    ImGui::Text("Head");
    ImGui::PushFont(smallFont);
    ImGui::Text("Yaw");
    ImGui::SameLine();
    ImGui::SliderFloat("##Head Yaw", &headYaw, -60.0f, 60.0f);
    ImGui::Text("Pitch");
    ImGui::SameLine();
    ImGui::SliderFloat("##Head Pitch", &headPitch, -35.0f, 15.0f);
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, 7.0f));
    ImGui::Text("Leg Angles");
    ImGui::PushFont(smallFont);
    ImGui::Text("Hip");
    ImGui::Text("Left Hip Angle");
    ImGui::SameLine();
    ImGui::SliderFloat("##Left Hip Angle", &leftHipAngle, -90.0f, 90.0f);
    ImGui::Text("Right Hip Angle");
    ImGui::SameLine();
    ImGui::SliderFloat("##Right Hip Angle", &rightHipAngle, -90.0f, 90.0f);
    ImGui::Text("Knee");
    ImGui::Text("Left Knee Angle");
    ImGui::SameLine();
    ImGui::SliderFloat("##Left Knee Angle", &leftKneeAngle, -90.0f, 90.0f);
    ImGui::Text("Right Knee Angle");
    ImGui::SameLine();
    ImGui::SliderFloat("##Right Knee Angle", &rightKneeAngle, -90.0f, 90.0f);
    ImGui::PopFont();

    ImGui::Separator();
    ImGui::Text("Camera Position");
    ImGui::PushFont(smallFont);
    ImGui::Text("X");
    ImGui::SameLine();
    ImGui::SliderFloat("##Camera Position X", &camX, -20.0f, 20.0f);
    ImGui::Text("Y");
    ImGui::SameLine();
    ImGui::SliderFloat("##Camera Position Y", &camY, -20.0f, 20.0f);
    ImGui::Text("Z");
    ImGui::SameLine();
    ImGui::SliderFloat("##Camera Position Z", &camZ, 0.0f, 50.0f);
    ImGui::PopFont();

    ImGui::Separator();
    ImGui::Text("Light Position");
    ImGui::PushFont(smallFont);
    ImGui::Text("X");
    ImGui::SameLine();
    ImGui::SliderFloat("##Light Position X", &lightPos[0], -10.0f, 10.0f);
    ImGui::Text("Y");
    ImGui::SameLine();
    ImGui::SliderFloat("##Light Position Y", &lightPos[1], -10.0f, 10.0f);
    ImGui::Text("Z");
    ImGui::SameLine();
    ImGui::SliderFloat("##Light Position Z", &lightPos[2], -10.0f, 10.0f);
    ImGui::Text("W");
    ImGui::SameLine();
    ImGui::SliderFloat("##Light Position W", &lightPos[3], -10.0f, 10.0f);

    ImGui::Text("Light Angle");
    ImGui::SameLine();
    ImGui::SliderFloat("##Light Angle", &lightAngle, 0.0f, 360.0f);
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    ImGui::Text("Ambient Strength");
    ImGui::PushFont(smallFont);
    ImGui::SliderFloat("##Ambient Strength", &ambientStrength, 0.0f, 1.0f);
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    ImGui::Text("Point Light Intensity");
    ImGui::PushFont(smallFont);
    ImGui::SliderFloat("##Point Light Intensity", &pointLightIntensity, 0.0f, 1.0f);
    ImGui::PopFont();

    ImGui::Separator();

    ImGui::Text("Floor Material");
    ImGui::PushFont(smallFont);
    ImGui::ColorEdit3("Floor Specular", floorSpecular);
    ImGui::SliderFloat("Floor Shininess", &floorShininess, 1.0f, 128.0f);  // Corrected the range
    ImGui::PopFont();

    ImGui::Separator();

    ImGui::Text("Plastic Sphere Material");
    ImGui::PushFont(smallFont);
    ImGui::ColorEdit3("Specular", plasticSpecular);
    ImGui::SliderFloat("Shininess", &plasticShininess, 1.0f, 128.0f);
    ImGui::PopFont();

    ImGui::Text("Teapot Material");
    ImGui::PushFont(smallFont);
    ImGui::ColorEdit3("Teapot Specular", teapotSpecular);
    ImGui::SliderFloat("Teapot Shininess", &teapotShininess, 1.0f, 200.0f);
    ImGui::PopFont();

    ImGui::Text("Robot Material");
    ImGui::PushFont(smallFont);
    ImGui::ColorEdit3("Robot Specular", robotSpecular);
    ImGui::SliderFloat("Robot Shininess", &robotShininess, 1.0f, 128.0f);
    ImGui::PopFont();

    ImGui::Text("Cube Material");
    ImGui::PushFont(smallFont);
    ImGui::ColorEdit3("Cube Specular", cubeSpecular);
    ImGui::SliderFloat("Cube Shininess", &cubeShininess, 1.0f, 128.0f);
    ImGui::PopFont();

    ImGui::Separator();

    if (ImGui::Checkbox("Use Head Camera", &useHeadCam))
    {
        if (useHeadCam)
        {
            robotX = robotY = robotZ = 0.0f;
            headVisible = false;  // Hide head when using head camera
        }
        else
        {
            headVisible = true;   // Show head when using main camera
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Help"))
    {
        show_help_window = true;
    }
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(7.0f, 0.0f));
    ImGui::SameLine();
    if (ImGui::Button("Quit"))
    {
        exit(0);
    }
    ImGui::End();

    if (show_help_window)
    {
        ImGui::Begin("Help", &show_help_window);
        ImGui::Text("Controls:");
        ImGui::BulletText("W: Move Forward");
        ImGui::BulletText("S: Move Backward");
        ImGui::BulletText("A: Turn Left");
        ImGui::BulletText("D: Turn Right");
        ImGui::BulletText("Use mouse to control the robot's head");
        ImGui::BulletText("Use the sliders to adjust robot parts and camera");
        ImGui::End();
    }

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    glutSwapBuffers();
}

void reshape(int width, int height)
{
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, windowWidth, windowHeight);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{
    isMoving = true;

    if (key == 'w')
    {
        if (currentDirection != FORWARD)
        {
            switch (currentDirection)
            {
            case BACKWARD:
                robotRotation += 180.0f;
                break;
            case LEFT:
                robotRotation -= 90.0f;
                break;
            case RIGHT:
                robotRotation += 90.0f;
                break;
            }
            currentDirection = FORWARD;
        }
        robotZ -= 0.1f;
        walkCycle += 0.1f;
    }
    else if (key == 's')
    {
        if (currentDirection != BACKWARD)
        {
            switch (currentDirection)
            {
            case FORWARD:
                robotRotation -= 180.0f;
                break;
            case LEFT:
                robotRotation += 90.0f;
                break;
            case RIGHT:
                robotRotation -= 90.0f;
                break;
            }
            currentDirection = BACKWARD;
        }
        robotZ += 0.1f;
        walkCycle -= 0.1f;
    }
    else if (key == 'a')
    {
        if (currentDirection != LEFT)
        {
            switch (currentDirection)
            {
            case FORWARD:
                robotRotation += 90.0f;
                break;
            case BACKWARD:
                robotRotation -= 90.0f;
                break;
            case RIGHT:
                robotRotation -= 180.0f;
                break;
            }
            currentDirection = LEFT;
        }
        robotX -= 0.1f;
        walkCycle += 0.1f;
    }
    else if (key == 'd')
    {
        if (currentDirection != RIGHT)
        {
            switch (currentDirection)
            {
            case FORWARD:
                robotRotation -= 90.0f;
                break;
            case BACKWARD:
                robotRotation += 90.0f;
                break;
            case LEFT:
                robotRotation -= 180.0f;
                break;
            }
            currentDirection = RIGHT;
        }
        robotX += 0.1f;
        walkCycle += 0.1f;
    }

    glutPostRedisplay();
}

void mouseMotion(int x, int y)
{
    static bool firstMouse = true;
    static int lastX, lastY;
    if (firstMouse)
    {
        lastX = x;
        lastY = y;
        firstMouse = false;
    }

    int xoffset = x - lastX;
    int yoffset = lastY - y;
    lastX = x;
    lastY = y;

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    if (useHeadCam)
    {
        headCamYaw += xoffset;
        headCamPitch += yoffset;

        if (headCamYaw > 60.0f)
            headCamYaw = 60.0f;
        if (headCamYaw < -60.0f)
            headCamYaw = -60.0f;
        if (headCamPitch > 15.0f)
            headCamPitch = 15.0f;
        if (headCamPitch < -35.0f)
            headCamPitch = -35.0f;
    }
    else
    {
        headYaw += xoffset;
        headPitch += yoffset;

        if (headYaw > 60.0f)
            headYaw = 60.0f;
        if (headYaw < -60.0f)
            headYaw = -60.0f;
        if (headPitch > 15.0f)
            headPitch = 15.0f;
        if (headPitch < -35.0f)
            headPitch = -35.0f;
    }

    glutPostRedisplay();
}

void updateLightPosition()
{
    lightPos[0] = 7.5f * cos(glm::radians(lightAngle));
    lightPos[2] = 7.5f * sin(glm::radians(lightAngle));
    glutPostRedisplay();
}

void updateAnimation()
{
    if (isMoving)
    {
        leftHipAngle = 30.0f * sin(walkCycle);
        leftKneeAngle = 30.0f * sin(walkCycle + glm::pi<float>() / 2);
        rightHipAngle = 30.0f * sin(walkCycle + glm::pi<float>());
        rightKneeAngle = 30.0f * sin(walkCycle + 3 * glm::pi<float>() / 2);
    }
    else
    {
        leftHipAngle = 0.0f;
        leftKneeAngle = 0.0f;
        rightHipAngle = 0.0f;
        rightKneeAngle = 0.0f;
    }

    glutPostRedisplay();
}

void init()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGLUT_Init();
    ImGui_ImplGLUT_InstallFuncs();
    ImGui_ImplOpenGL2_Init();

    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO(); (void)&io;
    io.FontGlobalScale = 1.5f;
    io.DisplaySize = ImVec2((float)windowWidth, (float)windowHeight);

    font = io.Fonts->AddFontFromFileTTF("Assets/Roboto-Regular.ttf", 18.0f);
    smallFont = io.Fonts->AddFontFromFileTTF("Assets/Roboto-Regular.ttf", 12.0f);
    if (font == NULL || smallFont == NULL)
    {
#ifdef DEBUG
        std::cerr << "Failed to load font file!" << std::endl;
#endif
    }

    loadTextures();
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
}

void idle()
{
    updateLightPosition();
    updateAnimation();
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Robot");

    glewInit();
    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutPassiveMotionFunc(mouseMotion);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);

    glutMainLoop();

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGLUT_Shutdown();
    ImGui::DestroyContext();

    return 0;
}
