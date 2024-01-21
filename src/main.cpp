#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include "imgui.h"
#include "backends/imgui_impl_opengl2.h"
#include "backends/imgui_impl_glut.h"
#include "physics/physics_engine.h"
#include "imgui_internal.h"
#include <glm/glm.hpp>

GLuint window;
GLuint width = 1000, height = 1000;

int number_of_balls_to_add = 0;
int particle_counter = 0;
int ball_add_counter = 0;
float particle_velocity_x = 0.01;
float particle_velocity_y = 0.01;
int particle_time_delta = 5;
float particle_size = 0.02;
int particle_segments = 10;
bool showGrid = false;
bool showBallColor = false;

float gravity = -0.00f;
int grid_size = 30;

void MainLoopStep();
ThreadPool threadPool(1);
PhysicsEngine physicsEngine = PhysicsEngine(gravity, threadPool);

std::vector<GameObject> balls = std::vector<GameObject>();


void drawGrid(int gridHeight, int gridWidth) {
    glBegin(GL_LINES);
    glColor3f(0.0f, 0.0f, 0.0f);

    // Draw horizontal lines
    for (int i = 0; i <= gridHeight; ++i) {
        glVertex2f(-1.0, 2.0 * i / gridHeight - 1.0);
        glVertex2f(1.0, 2.0 * i / gridHeight - 1.0);
    }

    // Draw vertical lines
    for (int i = 0; i <= gridWidth; ++i) {
        glVertex2f(2.0 * i / gridWidth - 1.0, -1.0);
        glVertex2f(2.0 * i / gridWidth - 1.0, 1.0);
    }

    glEnd();
}

void getColor(int index, float& red, float& green, float& blue) {
    // Example: Coloring cells based on a pattern (you can modify this logic)
        const float frequency = 0.5;
        red = sin(frequency * index + 0) * 0.5 + 0.5;
        green = sin(frequency * index + 2) * 0.5 + 0.5;
        blue = sin(frequency * index + 4) * 0.5 + 0.5;

}



void drawCircle(float cx, float cy, float radius, int num_segments, glm::vec3 color, bool collided = false) {


    glBegin(GL_TRIANGLE_FAN );
    for (int i = 0; i < num_segments; i++) {
        float theta = 2.0f * 3.1415926f * float(i) / float(num_segments);
        float x = radius * cosf(theta);
        float y = radius * sinf(theta);
        glColor3f(color.x, color.y, color.z);
        glVertex2f(x + cx, y + cy);
    }
    glEnd();
    glm::vec3 borderColor = glm::vec3(0.0, 0.0, 0.0);
    if (collided) {
        glBegin(GL_LINE_LOOP);

        for (int i = 0; i < num_segments; i++) {
            float theta = 2.0f * 3.1415926f * float(i) / float(num_segments);
            float x = radius * cosf(theta);
            float y = radius * sinf(theta);
            glColor3f(0.f, 0.f, 0.f);
            glVertex2f(x + cx, y + cy);
        }
        glEnd();
    }
}

// Display callback function
void display() {
    auto& gameObjects = physicsEngine.getGameObjects();
    if(showGrid){
        drawGrid(physicsEngine.getGrid().height, physicsEngine.getGrid().width);

    }
    // Draw balls
    float red = 0.0, green = 0.0, blue = 0.0;

    for (int i = 0; i < gameObjects.size(); ++i) {
        glm::vec3 color = glm::vec3(0.0, 0.0, 0.0);
        if (showBallColor) {
        getColor(gameObjects[i].gridIndex, red, green, blue);
            color = glm::vec3(red, green, blue);
    }
        else{
            color = gameObjects[i].color;
        }
        GameObject& gameObject = gameObjects[i];
        drawCircle(gameObject.x, gameObject.y, gameObject.radius, particle_segments, color, gameObject.collided);
        gameObject.collided = false;

    }


}

// Timer callback function for animation
void timer(int) {
    physicsEngine.update();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);  // 60 frames per second
    if (ball_add_counter % particle_time_delta == 0) {
        if (number_of_balls_to_add > 0) {
            float red = 0.0, green = 0.0, blue = 0.0;
            getColor(particle_counter, red, green, blue);
            glm::vec3 color = glm::vec3(red, green, blue);
            physicsEngine.addGameObject(GameObject{0, 0.8, particle_velocity_x, particle_velocity_y, particle_size, 100*particle_size, color});
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

int balls_to_add = 200;
void MainLoopStep()

{


    // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();



    ImGuiIO& io = ImGui::GetIO();

    {

        ImGui::Begin("Ball Collision Detection DEMO");                          // Create a window called "Hello, world!" and append into it.
        ImGui::Text("Thread pool size: %d", threadPool.size);               // Display some text (you can use a format strings too

        ImGui::SliderInt("Number of objects to add", &balls_to_add, 0, 10000);

        ImGui::Separator();

        ImGui::SliderFloat("Particle velocity x", &particle_velocity_x, -0.1, 0.1);
        ImGui::SliderFloat("Particle velocity y", &particle_velocity_y, -0.1, 0.1);
        ImGui::SliderInt("Particle time delta", &particle_time_delta, 1, 60);
        if(ImGui::SliderFloat("Particle size", &particle_size, 0.001, 0.04)){
            for(auto &gameObject: physicsEngine.getGameObjects()){
                gameObject.radius = particle_size;
           }
        }
        ImGui::SliderInt("Particle segments", &particle_segments, 3, 365);
        if (ImGui::SliderInt("Grid size (nxn)", &grid_size, 1, 100)){
            physicsEngine.resizeGrid(grid_size, grid_size);
        }


        if(ImGui::Button("Add balls")){
            number_of_balls_to_add = balls_to_add;
        }
        ImGui::Checkbox("Show grid", &showGrid);
        ImGui::Checkbox("Show ball color", &showBallColor);



        ImGui::Text("Number of particles: %d", particle_counter);

        if(ImGui::Button("Clear balls")){
            physicsEngine.setGameObjects(std::vector<GameObject>());
            particle_counter = 0;
        }

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        glm::vec2 mouseWorldPos = glm::vec2{io.MousePos.x / width * 2.0f - 1.0f, io.MousePos.y / height * 2.0f - 1.0f};
        glm::vec2 mousePosGrid = physicsEngine.mapToWorldToGrid(mouseWorldPos, physicsEngine.getGrid());
        ImGui::Text("Mouse position x= %.3f y= %.3f, Grid coordinates x=%d, y=%d (index = %d)", mouseWorldPos.x, mouseWorldPos.y, (int)mousePosGrid.x, (int)mousePosGrid.y, (int)(mousePosGrid.x * physicsEngine.getGrid().height + mousePosGrid.y));

        ImGui::End();
    }



    // Rendering
    ImGui::Render();
    glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
    glClearColor(0.5f, 0.5f, 0.5f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound, but prefer using the GL3+ code.
    display();

    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());


    glutSwapBuffers();

}