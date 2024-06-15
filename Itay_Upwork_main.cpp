#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include "imgui.h"
#include "imgui_impl_glut.h"
#include "imgui_impl_opengl2.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef DEBUG
#include <iostream>
#endif

// Global variables for robot parts transformations
float robotX = 0.0f, robotY = 0.0f, robotZ = 0.0f;
float robotRotation = 0.0f; // Rotation of the robot
float shoulderPitch = 0.0f, shoulderYaw = 0.0f, shoulderRoll = 0.0f;
float elbowPitch = 0.0f, elbowYaw = 0.0f, elbowRoll = 0.0f;
float wristPitch = 0.0f, wristYaw = 0.0f, wristRoll = 0.0f;
float headYaw = 0.0f, headPitch = 0.0f;
float leftHipAngle = 0.0f, leftKneeAngle = 0.0f;
float rightHipAngle = 0.0f, rightKneeAngle = 0.0f;

// Camera parameters
float camX = 0.0f, camY = 5.0f, camZ = 15.0f; // Adjusted Z value for better view
float camPitch = 0.0f, camYaw = 0.0f;

// Variables to save the main camera state
float savedCamX, savedCamY, savedCamZ;
float savedCamPitch, savedCamYaw;

// Secondary camera parameters (inside robot head)
float headCamX = 0.0f, headCamY = 0.75f, headCamZ = 0.0f;
float headCamYaw = 0.0f, headCamPitch = 0.0f;

bool useHeadCam = false; // Boolean flag to switch cameras

// Lighting parameters
float lightPos[] = { 1.2f, 2.5f, 2.0f, 1.0f };
float ambientStrength = 0.1f;
float pointLightIntensity = 1.0f;

// Set initial window size
int windowWidth = 1280;
int windowHeight = 720;

bool show_help_window = false; // Variable to control the display of the help window

ImFont* smallFont, * font;

// Textures
GLuint floorTexture, metalTexture, cubeTexture;

// Animation parameters
bool isMoving = false;
float walkCycle = 0.0f;

// Material properties
GLfloat floorSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat floorShininess = 100.0f;

GLfloat metalSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat metalShininess = 128.0f;

GLfloat cubeSpecular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
GLfloat cubeShininess = 64.0f;

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
    GLfloat diffuseLight[] = { pointLightIntensity, pointLightIntensity, pointLightIntensity, 1.0f };
    GLfloat specularLight[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
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

        // Set texture parameters
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

void loadTextures()
{
    if (!loadTexture("Assets/tiles_0006_color_1k.jpg", floorTexture))
    {
#ifdef DEBUG
        std::cerr << "Failed to load floor texture!" << std::endl;
#endif
    }

    if (!loadTexture("Assets/metal.jpg", metalTexture))
    {
#ifdef DEBUG
        std::cerr << "Failed to load metal texture!" << std::endl;
#endif
    }

    if (!loadTexture("Assets/canvas.jpg", cubeTexture))
    {
#ifdef DEBUG
        std::cerr << "Failed to load cube texture!" << std::endl;
#endif
    }
}

void drawLightBox()
{
    glPushMatrix();
    glTranslatef(lightPos[0], lightPos[1], lightPos[2]);

    // Save the current material properties
    GLfloat prevAmbient[4], prevDiffuse[4], prevSpecular[4], prevShininess[1], prevEmission[4];
    glGetMaterialfv(GL_FRONT, GL_AMBIENT, prevAmbient);
    glGetMaterialfv(GL_FRONT, GL_DIFFUSE, prevDiffuse);
    glGetMaterialfv(GL_FRONT, GL_SPECULAR, prevSpecular);
    glGetMaterialfv(GL_FRONT, GL_SHININESS, prevShininess);
    glGetMaterialfv(GL_FRONT, GL_EMISSION, prevEmission);

    // Set material properties for the light box
    GLfloat yellow[] = { 1.0f, 1.0f, 0.0f, 1.0f };
    GLfloat brightYellow[] = { 1.0f, 1.0f, 0.0f, 1.0f };
    GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat emission[] = { 0.5f, 0.5f, 0.0f, 1.0f }; // Higher emissive property for brightness

    glMaterialfv(GL_FRONT, GL_AMBIENT, yellow);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, brightYellow);
    glMaterialfv(GL_FRONT, GL_SPECULAR, white);
    glMaterialf(GL_FRONT, GL_SHININESS, 50.0f);    // Increase shininess for a brighter appearance
    glMaterialfv(GL_FRONT, GL_EMISSION, emission); // Set emissive property

    // Draw the light box
    glutSolidCube(0.2f);

    // Restore the previous material properties
    glMaterialfv(GL_FRONT, GL_AMBIENT, prevAmbient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, prevDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, prevSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, prevShininess);
    glMaterialfv(GL_FRONT, GL_EMISSION, prevEmission);

    glPopMatrix();
}

void drawRobotHead(bool visible)
{
    if (!visible)
        return;

    // Draw head
    glPushMatrix();
    glTranslatef(0.0f, 1.75f, 0.0f); // Move the head up
    glRotatef(headYaw, 0.0f, 1.0f, 0.0f);
    glRotatef(headPitch, 1.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glutSolidSphere(0.5f, 20, 20);

    // Draw eyes
    glPushMatrix();
    glColor3f(1.0f, 1.0f, 1.0f);
    glTranslatef(0.2f, 0.1f, 0.45f);
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

void drawMiddlePart()
{
    glPushMatrix();
    glTranslatef(0.0f, 0.9f, 0.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    drawLimb(0.75f, 0.15f);
    glPopMatrix();
}

void drawRightArm()
{
    glPushMatrix();
    glTranslatef(0.65f, 1.0f, 0.0f);

    // Apply shoulder rotations using quaternions
    glm::quat shoulderQuaternion = glm::angleAxis(glm::radians(shoulderYaw), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::angleAxis(glm::radians(shoulderPitch), glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::angleAxis(glm::radians(shoulderRoll), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 shoulderRotation = glm::toMat4(shoulderQuaternion);

    glMultMatrixf(glm::value_ptr(shoulderRotation));

    // Shoulder joint
    glColor3f(0.0f, 1.0f, 0.0f);
    drawJoint(0.25f);

    glPushMatrix();
    glTranslatef(0.0f, -0.25f, 0.0f);  // Adjust for half the cylinder length
    glRotatef(90, 1.0f, 0.0f, 0.0f);  // Align cylinder along y-axis
    glColor3f(0.0f, 0.0f, 1.0f);
    drawLimb(0.25f, 0.1f);
    glPopMatrix();

    glTranslatef(0.0f, -0.5f, 0.0f);  // Move to the end of the first limb

    // Apply elbow rotations using quaternions
    glm::quat elbowQuaternion = glm::angleAxis(glm::radians(elbowYaw), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::angleAxis(glm::radians(elbowPitch), glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::angleAxis(glm::radians(elbowRoll), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 elbowRotation = glm::toMat4(elbowQuaternion);

    glMultMatrixf(glm::value_ptr(elbowRotation));

    // Elbow joint
    glColor3f(0.0f, 1.0f, 0.0f);
    drawJoint(0.2f);

    glPushMatrix();
    glTranslatef(0.0f, -0.25f, 0.0f);  // Adjust for half the cylinder length
    glRotatef(90, 1.0f, 0.0f, 0.0f);  // Align cylinder along y-axis
    glColor3f(0.0f, 0.0f, 1.0f);
    drawLimb(0.25f, 0.1f);
    glPopMatrix();

    glTranslatef(0.0f, -0.5f, 0.0f);  // Move to the end of the second limb

    // Apply wrist rotations using quaternions
    glm::quat wristQuaternion = glm::angleAxis(glm::radians(wristYaw), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::angleAxis(glm::radians(wristPitch), glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::angleAxis(glm::radians(wristRoll), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 wristRotation = glm::toMat4(wristQuaternion);

    glMultMatrixf(glm::value_ptr(wristRotation));

    // Wrist joint
    glColor3f(0.0f, 1.0f, 0.0f);
    drawJoint(0.15f);

    glPushMatrix();
    glTranslatef(0.0f, -0.1f, 0.0f);  // Adjust for half the cylinder length
    glRotatef(90, 1.0f, 0.0f, 0.0f);  // Align cylinder along y-axis
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

    // Hip joint
    glColor3f(0.0f, 1.0f, 0.0f);
    drawJoint(0.2f);

    glPushMatrix();
    glTranslatef(0.0f, -0.1f, 0.0f);  // Adjust for half the cylinder length
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    drawLimb(0.35f, 0.2f);
    glPopMatrix();

    glTranslatef(0.0f, -0.55f, 0.0f);  // Move to the end of the first limb
    glRotatef(kneeAngle, 1.0f, 0.0f, 0.0f);

    // Knee joint
    glColor3f(0.0f, 1.0f, 0.0f);
    drawJoint(0.18f);

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.0f);  // Adjust for half the cylinder length
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
    glPushMatrix();
    glTranslatef(robotX, robotY, robotZ);
    glRotatef(robotRotation, 0.0f, 1.0f, 0.0f);

    // Draw legs
    drawLeg(leftHipAngle, leftKneeAngle, -0.25f, 0.0f, 0.0f); // Left leg
    drawLeg(rightHipAngle, rightKneeAngle, 0.25f, 0.0f, 0.0f); // Right leg

    // Draw middle part
    drawMiddlePart();

    drawJoint(0.18f);

    // Draw head
    drawRobotHead(!useHeadCam);

    // Draw arms
    drawRightArm();
    //drawLeftArm();

    drawNeck();

    glPopMatrix();
}

void drawFloor()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, floorTexture);

    // Set the material properties for a shiny floor
    glMaterialfv(GL_FRONT, GL_SPECULAR, floorSpecular);
    glMaterialf(GL_FRONT, GL_SHININESS, floorShininess);

    glPushMatrix();
    glTranslatef(0.0f, -0.9f, 0.0f);
    glScalef(10.0f, 0.1f, 10.0f);

    // Render the tiled floor
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f); // Normal pointing up
    for (int i = -5; i < 5; ++i)
    {
        for (int j = -5; j < 5; ++j)
        {
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(i, 0.0f, j);
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(i + 1, 0.0f, j);
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(i + 1, 0.0f, j + 1);
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(i, 0.0f, j + 1);
        }
    }
    glEnd();

    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
}

void drawMetalSphere()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, metalTexture);

    // Set the material properties for a metallic sphere
    glMaterialfv(GL_FRONT, GL_SPECULAR, metalSpecular);
    glMaterialf(GL_FRONT, GL_SHININESS, metalShininess);

    glPushMatrix();
    glTranslatef(-2.0f, 1.0f, 0.0f);
    glutSolidSphere(0.5f, 20, 20);
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
}

void drawTexturedCube()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, cubeTexture);

    // Set the material properties for a cube
    glMaterialfv(GL_FRONT, GL_SPECULAR, cubeSpecular);
    glMaterialf(GL_FRONT, GL_SHININESS, cubeShininess);

    glPushMatrix();
    glTranslatef(2.0f, 1.0f, 0.0f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
}

void drawTexturedCone()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, cubeTexture);

    // Set the material properties for a cone
    glMaterialfv(GL_FRONT, GL_SPECULAR, cubeSpecular);
    glMaterialf(GL_FRONT, GL_SHININESS, cubeShininess);

    glPushMatrix();
    glTranslatef(0.0f, 1.0f, -5.0f);
    glutSolidCone(0.5f, 1.0f, 20, 20);
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
}

void renderScene()
{
    // Set up lighting
    setupLighting();

    // Render the floor
    drawFloor();

    // Render the robot
    drawRobot();

    // Render the light source as a box
    drawLightBox();

    // Render additional objects
    drawMetalSphere();
    drawTexturedCube();
    drawTexturedCone();
}

void display()
{
    // Clear the default framebuffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // Start the ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();

    // Set up the camera
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (useHeadCam)
    {
        // Use the secondary camera inside the robot's head
        float eyeX = robotX;
        float eyeY = robotY + headCamY;
        float eyeZ = robotZ + headCamZ;
        float lookX = eyeX + sin(glm::radians(headYaw)) * cos(glm::radians(headPitch));
        float lookY = eyeY + sin(glm::radians(headPitch));
        float lookZ = eyeZ - cos(glm::radians(headYaw)) * cos(glm::radians(headPitch));

        gluLookAt(eyeX, eyeY, eyeZ, lookX, lookY, lookZ, 0.0f, 1.0f, 0.0f);
    }
    else
    {
        // Use the main camera
        gluLookAt(camX, camY, camZ, camX + sin(camYaw), camY + sin(camPitch), camZ - cos(camYaw), 0.0f, 1.0f, 0.0f);
    }

    // Render the scene
    renderScene();

    ImGui::NewFrame();
    // Render ImGui UI
    ImGui::SetNextWindowPos(ImVec2(windowWidth - 300, 0)); // Dock to the right
    ImGui::SetNextWindowSize(ImVec2(300, windowHeight));   // Adjust width as needed
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
    ImGui::Text("Shoulder Pitch");
    ImGui::SliderFloat("##Shoulder Pitch", &shoulderPitch, -180.0f, 180.0f);
    ImGui::Text("Shoulder Yaw");
    ImGui::SliderFloat("##Shoulder Yaw", &shoulderYaw, -180.0f, 180.0f);
    ImGui::Text("Shoulder Roll");
    ImGui::SliderFloat("##Shoulder Roll", &shoulderRoll, -180.0f, 180.0f);
    ImGui::Text("Elbow Pitch");
    ImGui::SliderFloat("##Elbow Pitch", &elbowPitch, -180.0f, 180.0f);
    ImGui::Text("Elbow Yaw");
    ImGui::SliderFloat("##Elbow Yaw", &elbowYaw, -180.0f, 180.0f);
    ImGui::Text("Elbow Roll");
    ImGui::SliderFloat("##Elbow Roll", &elbowRoll, -180.0f, 180.0f);
    ImGui::Text("Wrist Pitch");
    ImGui::SliderFloat("##Wrist Pitch", &wristPitch, -180.0f, 180.0f);
    ImGui::Text("Wrist Yaw");
    ImGui::SliderFloat("##Wrist Yaw", &wristYaw, -180.0f, 180.0f);
    ImGui::Text("Wrist Roll");
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
    ImGui::SliderFloat("##Left Hip Angle", &leftHipAngle, -90.0f, 90.0f);
    ImGui::SliderFloat("##Right Hip Angle", &rightHipAngle, -90.0f, 90.0f);
    ImGui::Text("Knee");
    ImGui::SliderFloat("##Left Knee Angle", &leftKneeAngle, -90.0f, 90.0f);
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
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, 7.0f));
    ImGui::Text("Ambient Strength");
    ImGui::PushFont(smallFont);
    ImGui::SliderFloat("##Ambient Strength", &ambientStrength, 0.0f, 1.0f);
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0.0f, 7.0f));
    ImGui::Text("Point Light Intensity");
    ImGui::PushFont(smallFont);
    ImGui::SliderFloat("##Point Light Intensity", &pointLightIntensity, 0.0f, 1.0f);
    ImGui::PopFont();

    ImGui::Separator();

    ImGui::Text("Floor Material");
    ImGui::PushFont(smallFont);
    ImGui::ColorEdit3("Floor Specular", (float*)floorSpecular);
    ImGui::SliderFloat("Floor Shininess", &floorShininess, 1.0f, 128.0f);
    ImGui::PopFont();

    ImGui::Separator();

    ImGui::Text("Metal Sphere Material");
    ImGui::PushFont(smallFont);
    ImGui::ColorEdit3("Metal Specular", (float*)metalSpecular);
    ImGui::SliderFloat("Metal Shininess", &metalShininess, 1.0f, 128.0f);
    ImGui::PopFont();

    ImGui::Separator();

    ImGui::Text("Cube Material");
    ImGui::PushFont(smallFont);
    ImGui::ColorEdit3("Cube Specular", (float*)cubeSpecular);
    ImGui::SliderFloat("Cube Shininess", &cubeShininess, 1.0f, 128.0f);
    ImGui::PopFont();

    ImGui::Separator();

    if (ImGui::Checkbox("Use Head Camera", &useHeadCam))
    {
        // Save or restore the camera state when switching
        if (useHeadCam)
        {
            // Save the current camera state
            savedCamX = camX;
            savedCamY = camY;
            savedCamZ = camZ;
            savedCamPitch = camPitch;
            savedCamYaw = camYaw;

            // Reset robot movement
            robotX = robotY = robotZ = 0.0f;
        }
        else
        {
            // Restore the saved camera state
            camX = savedCamX;
            camY = savedCamY;
            camZ = savedCamZ;
            camPitch = savedCamPitch;
            camYaw = savedCamYaw;

            // Ensure the head is looking at the same direction as the secondary camera
            headYaw = headCamYaw;
            headPitch = headCamPitch;
        }
    }

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
                robotRotation += 90.0f;
                break;
            case RIGHT:
                robotRotation -= 90.0f;
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
                robotRotation += 180.0f;
                break;
            case LEFT:
                robotRotation -= 90.0f;
                break;
            case RIGHT:
                robotRotation += 90.0f;
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
                robotRotation -= 90.0f;
                break;
            case BACKWARD:
                robotRotation += 90.0f;
                break;
            case RIGHT:
                robotRotation += 180.0f;
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
                robotRotation += 90.0f;
                break;
            case BACKWARD:
                robotRotation -= 90.0f;
                break;
            case LEFT:
                robotRotation += 180.0f;
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

        // Enforce yaw and pitch limits for the head camera
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

        // Enforce yaw and pitch limits for the head
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

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGLUT_Init();
    ImGui_ImplGLUT_InstallFuncs();
    ImGui_ImplOpenGL2_Init();

    // Set up ImGui style
    ImGui::StyleColorsDark();

    // Increase DPI scaling for crisper UI
    ImGuiIO& io = ImGui::GetIO();
    (void)&io;
    io.FontGlobalScale = 1.5f;
    io.DisplaySize = ImVec2((float)windowWidth, (float)windowHeight); // Set initial display size

    // Load custom font
    font = io.Fonts->AddFontFromFileTTF("Assets/Roboto-Regular.ttf", 18.0f);
    smallFont = io.Fonts->AddFontFromFileTTF("Assets/Roboto-Regular.ttf", 12.0f);
    if (font == NULL || smallFont == NULL)
    {
#ifdef DEBUG
        std::cerr << "Failed to load font file!" << std::endl;
#endif
    }

    loadTextures();
}

void idle()
{
    updateAnimation();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("OpenGL with ImGui");

    glewInit();
    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutPassiveMotionFunc(mouseMotion);
    glutReshapeFunc(reshape); // Add reshape callback
    glutIdleFunc(idle);

    glutMainLoop();

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGLUT_Shutdown();
    ImGui::DestroyContext();

    return 0;
}
