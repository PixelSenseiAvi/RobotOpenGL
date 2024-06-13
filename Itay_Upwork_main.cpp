#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include "imgui.h"
#include "imgui_impl_glut.h"
#include "imgui_impl_opengl2.h"

#ifdef DEBUG
#include <iostream>
#endif

// Global variables for robot parts transformations
float robotX = 0.0f, robotY = 0.0f, robotZ = 0.0f;
float shoulderAngle = 0.0f, elbowAngle = 0.0f, wristAngle = 0.0f;
float headYaw = 0.0f, headPitch = 0.0f;

// Camera parameters
float camX = 0.0f, camY = 5.0f, camZ = 15.0f; // Adjusted Z value for better view
float camPitch = 0.0f, camYaw = 0.0f;

// Lighting parameters
float lightPos[] = {1.2f, 2.5f, 2.0f, 1.0f};
float ambientStrength = 0.1f;
float pointLightIntensity = 1.0f;

// Set initial window size
int windowWidth = 1280;
int windowHeight = 720;

bool show_help_window = false; // Variable to control the display of the help window

ImFont *smallFont, *font;

void setupLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat ambientLight[] = {ambientStrength, ambientStrength, ambientStrength, 1.0f};
    GLfloat diffuseLight[] = {pointLightIntensity, pointLightIntensity, pointLightIntensity, 1.0f};
    GLfloat specularLight[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
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
    GLfloat yellow[] = {1.0f, 1.0f, 0.0f, 1.0f};
    GLfloat brightYellow[] = {1.0f, 1.0f, 0.0f, 1.0f};
    GLfloat white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat emission[] = {0.5f, 0.5f, 0.0f, 1.0f}; // Higher emissive property for brightness

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

void drawRobot()
{
    glPushMatrix();
    glTranslatef(robotX, robotY, robotZ);

    // Draw body
    glPushMatrix();
    glColor3f(1.0f, 0.0f, 0.0f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Draw head
    glPushMatrix();
    glTranslatef(0.0f, 0.75f, 0.0f);
    glRotatef(headYaw, 0.0f, 1.0f, 0.0f);
    glRotatef(headPitch, 1.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glutSolidSphere(0.5f, 20, 20);
    glPopMatrix();

    // Draw right arm
    glPushMatrix();
    glTranslatef(0.75f, 0.0f, 0.0f);
    glRotatef(shoulderAngle, 1.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(0.5f, 0.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glutSolidCube(0.5f);
    glPopMatrix();

    glTranslatef(0.5f, 0.0f, 0.0f);
    glRotatef(elbowAngle, 0.0f, 0.0f, 1.0f);
    glPushMatrix();
    glTranslatef(0.25f, 0.0f, 0.0f);
    glutSolidCube(0.5f);
    glPopMatrix();

    glTranslatef(0.25f, 0.0f, 0.0f);
    glRotatef(wristAngle, 0.0f, 1.0f, 0.0f);
    glPushMatrix();
    glTranslatef(0.1f, 0.0f, 0.0f);
    glutSolidCube(0.2f);
    glPopMatrix();

    glPopMatrix();

    glPopMatrix();
}

void display()
{
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
    gluLookAt(camX, camY, camZ, camX + sin(camYaw), camY + sin(camPitch), camZ - cos(camYaw), 0.0f, 1.0f, 0.0f);

    // Set up lighting
    setupLighting();

    // Render the floor
    glPushMatrix();
    glColor3f(0.5f, 0.5f, 0.5f);
    glTranslatef(0.0f, -1.0f, 0.0f);
    glScalef(10.0f, 0.1f, 10.0f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Render the robot
    drawRobot();

    // Render the light source as a box
    drawLightBox();

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
    ImGui::Text("Shoulder");
    ImGui::SliderFloat("##Shoulder Angle", &shoulderAngle, -90.0f, 90.0f);
    ImGui::Text("Elbow");
    ImGui::SliderFloat("##Elbow Angle", &elbowAngle, -90.0f, 90.0f);
    ImGui::Text("Wrist");
    ImGui::SliderFloat("##Wrist Angle", &wristAngle, -90.0f, 90.0f);
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, 7.0f));
    ImGui::Text("Head");
    ImGui::PushFont(smallFont);
    ImGui::Text("Yaw");
    ImGui::SameLine();
    ImGui::SliderFloat("##Head Yaw", &headYaw, -90.0f, 90.0f);
    ImGui::Text("Pitch");
    ImGui::SameLine();
    ImGui::SliderFloat("##Head Pitch", &headPitch, -45.0f, 45.0f);
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
    ImGui::SliderFloat("##Ambient Strength", &ambientStrength, 0.0f, 1.0f);
    ImGui::Dummy(ImVec2(0.0f, 7.0f));
    ImGui::Text("Point Light Intensity");
    ImGui::SliderFloat("##Point Light Intensity", &pointLightIntensity, 0.0f, 1.0f);

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
        ImGui::BulletText("A: Move Left");
        ImGui::BulletText("D: Move Right");
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
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{
    if (key == 'w')
        robotZ -= 0.1f;
    if (key == 's')
        robotZ += 0.1f;
    if (key == 'a')
        robotX -= 0.1f;
    if (key == 'd')
        robotX += 0.1f;
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

    headYaw += xoffset;
    headPitch += yoffset;

    if (headPitch > 45.0f)
        headPitch = 45.0f;
    if (headPitch < -45.0f)
        headPitch = -45.0f;

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
    ImGuiIO &io = ImGui::GetIO();
    (void)&io;
    io.FontGlobalScale = 1.5f;
    io.DisplaySize = ImVec2((float)windowWidth, (float)windowHeight); // Set initial display size

    // Load custom font
    font = io.Fonts->AddFontFromFileTTF("../Assets/Roboto-Regular.ttf", 18.0f);
    smallFont = io.Fonts->AddFontFromFileTTF("../Assets/Roboto-Regular.ttf", 12.0f);
    if (font == NULL || smallFont == NULL)
    {
#ifdef DEBUG
        std::cerr << "Failed to load font file!" << std::endl;
#endif
    }
}

void idle()
{
    glutPostRedisplay();
}

int main(int argc, char **argv)
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
