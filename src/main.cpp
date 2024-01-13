#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include "imgui.h"
#include "backends/imgui_impl_opengl2.h"
#include "backends/imgui_impl_glut.h"
#include "physics/physics_engine.h"
#include "imgui_internal.h"

GLuint window;
GLuint width = 1000, height = 1000;

int number_of_balls_to_add = 0;
int particle_counter = 0;
int ball_add_counter = 0;
float particle_velocity_x = 0.01;
float particle_velocity_y = 0.01;
int particle_time_delta = 5;
float particle_size = 0.02;

float gravity = -0.000;

void MainLoopStep();
ThreadPool threadPool(10);
PhysicsEngine physicsEngine = PhysicsEngine(gravity, threadPool);

std::vector<GameObject> balls = std::vector<GameObject>();



// Display callback function
void display() {
    auto gameObjects = physicsEngine.getGameObjects();
    // Draw balls
    for (int i = 0; i < gameObjects.size(); ++i) {
        glBegin(GL_TRIANGLE_FAN);
        for (int j = 0; j <= 360; ++j) {
            float angle = j * 3.14159265 / 180.0;
            float x = gameObjects[i].x + gameObjects[i].radius * std::cos(angle);
            float y = gameObjects[i].y + gameObjects[i].radius * std::sin(angle);
            glVertex2f(x, y);
        }
        glEnd();
    }

}

// Timer callback function for animation
void timer(int) {
    physicsEngine.update();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);  // 60 frames per second
    if (ball_add_counter % particle_time_delta == 0) {
        if (number_of_balls_to_add > 0) {
            physicsEngine.addGameObject(GameObject{0, 0.8, particle_velocity_x, particle_velocity_y, particle_size, 100*particle_size});
            number_of_balls_to_add--;
            ball_add_counter++;
            particle_counter++;
        }
    } else{
        ball_add_counter++;
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(100, 100);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutCreateWindow("2D Ball Simulation");
    glutDisplayFunc(MainLoopStep);
    glutTimerFunc(0, timer, 0);
    glClearColor(0.0, 0.0, 0.0, 1.0);  // Black background
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);  // Set the coordinate system
//    initializeBalls(10, 0, 1.0, 100, 0.1 );
    physicsEngine.setGameObjects(balls);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGLUT_Init();
    ImGui_ImplOpenGL2_Init();
    ImGui_ImplGLUT_InstallFuncs();

    glutMainLoop();
    return 0;
}

int balls_to_add = 10;
void MainLoopStep()

{

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO();

    {

        ImGui::Begin("Imgui DEMO window");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("2D gravity ball simulator.");               // Display some text (you can use a format strings too)
        ImGui::SliderInt("Number of balls to add", &balls_to_add, 0, 1000);

        ImGui::Separator();

        ImGui::SliderFloat("Particle velocity x", &particle_velocity_x, -0.1, 0.1);
        ImGui::SliderFloat("Particle velocity y", &particle_velocity_y, -0.1, 0.1);
        ImGui::SliderInt("Particle time delta", &particle_time_delta, 1, 60);
        ImGui::SliderFloat("Particle size", &particle_size, 0.01, 0.1);

        if(ImGui::Button("Add balls")){
            number_of_balls_to_add = balls_to_add;
        }



        ImGui::Text("Number of particles: %d", particle_counter);

        if(ImGui::Button("Clear balls")){
            physicsEngine.setGameObjects(std::vector<GameObject>());
            particle_counter = 0;
        }

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
    glClearColor(0.5f, 0.5f, 0.5f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound, but prefer using the GL3+ code.
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    display();

    glutSwapBuffers();

}