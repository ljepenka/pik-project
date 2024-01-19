#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include "imgui.h"
#include "backends/imgui_impl_opengl2.h"
#include "backends/imgui_impl_glut.h"
#include "physics2/physics_engine.cpp"
#include "imgui_internal.h"
#include "thread_pool/thread_pool2.cpp"

GLuint window;
GLuint width = 1000, height = 1000;

int number_of_balls_to_add = 0;
int particle_counter = 0;
int ball_add_counter = 0;
float particle_velocity_x = 0.5f;
float particle_velocity_y = 0.5f;
int particle_time_delta = 5;
float particle_size = 0.02;

float gravity = -0.000;

void MainLoopStep();
tp::ThreadPool threadPool(1);
//PhysicsEngine physicsEngine = PhysicsEngine(gravity, threadPool);
PhysicSolver physicsEngine = PhysicSolver(glm::ivec2{10, 10}, threadPool);

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


void drawGridColor(int gridHeight, int gridWidth) {
    glBegin(GL_QUADS);

    float cellWidth = 2.0 / gridWidth;
    float cellHeight = 2.0 / gridHeight;

    for (int row = 0; row < gridHeight; ++row) {
        for (int col = 0; col < gridWidth; ++col) {
            int index = row * gridWidth + col;

            float xMin = -1.0 + col * cellWidth;
            float xMax = xMin + cellWidth;
            float yMin = 1.0 - (row + 1) * cellHeight;
            float yMax = yMin + cellHeight;

            float red, green, blue;
            getColor(index, red, green, blue);

            glColor3f(red, green, blue);
            glVertex2f(xMin, yMin);
            glVertex2f(xMax, yMin);
            glVertex2f(xMax, yMax);
            glVertex2f(xMin, yMax);

//            if (std::find(coloredCells.begin(), coloredCells.end(), index) == coloredCells.end()) {
//                coloredCells.push_back(index);
//            }
        }
    }

    glEnd();
}



// Display callback function
void display() {
    auto gameObjects = physicsEngine.getGameObjects();
    drawGrid(physicsEngine.getGrid().height, physicsEngine.getGrid().width);
    // Draw balls
    for (int i = 0; i < gameObjects.size(); ++i) {
        glBegin(GL_TRIANGLE_FAN);
        for (int j = 0; j <= 360; ++j) {
            float angle = j * 3.14159265 / 180.0;
            float x = gameObjects[i].position.x + gameObjects[i].radius * std::cos(angle);
            float y = gameObjects[i].position.y + gameObjects[i].radius * std::sin(angle);
            float red, green, blue;
            int ind = gameObjects[i].gridIndex;
            getColor(ind, red, green, blue);
            glColor3f(red, green, blue);
            glVertex2f(x, y);
//            std::cout << "x: " << x << " y: " << y << std::endl;
        }
        glEnd();
    }

}

// Timer callback function for animation
//void timer(int) {
//    ImGuiIO& io = ImGui::GetIO(); (void)io;
//    physicsEngine.update(io.DeltaTime);
//    glutPostRedisplay();
//    glutTimerFunc(16, timer, 0);  // 60 frames per second
//    if (ball_add_counter % particle_time_delta == 0) {
//        if (number_of_balls_to_add > 0) {
//            physicsEngine.addObject(GameObject{{0.0, 0.8}, {particle_velocity_x, particle_velocity_y}, {0.0, 0.8}, particle_size, 100*particle_size});
//            number_of_balls_to_add--;
//            ball_add_counter++;
//            particle_counter++;
//        }
//    } else{
//        ball_add_counter++;
//    }
//}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(100, 100);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutCreateWindow("2D Ball Simulation");
    glutDisplayFunc(MainLoopStep);
//    glutTimerFunc(0, timer, 0);
    glClearColor(0.0, 0.0, 0.0, 1.0);  // Black background
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);  // Set the coordinate system
//    initializeBalls(10, 0, 1.0, 100, 0.1 );
//    physicsEngine.setGameObjects(balls);
    physicsEngine.clearGrid();

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

int balls_to_add = 1;
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

        ImGui::SliderFloat("Particle velocity x", &particle_velocity_x, -1.f, 1.f);
        ImGui::SliderFloat("Particle velocity y", &particle_velocity_y, -1.f, 1.f);
        ImGui::SliderInt("Particle time delta", &particle_time_delta, 1, 60);
        ImGui::SliderFloat("Particle size", &particle_size, 0.01, 0.1);

        if(ImGui::Button("Add balls")){
            number_of_balls_to_add = balls_to_add;
        }



        ImGui::Text("Number of particles: %d", particle_counter);

        if(ImGui::Button("Clear balls")){
            physicsEngine.clearGrid();
            particle_counter = 0;
        }

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("Mouse position x= %.3f y= %.3f, Grid coordinates x=0, x=0", io.MousePos.x, io.MousePos.y);

        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
    glClearColor(0.5f, 0.5f, 0.5f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound, but prefer using the GL3+ code.
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    physicsEngine.update(io.DeltaTime);
    glutPostRedisplay();
//    glutTimerFunc(16, timer, 0);  // 60 frames per second
    if (ball_add_counter % particle_time_delta == 0) {
        if (number_of_balls_to_add > 0) {
            physicsEngine.addObject(GameObject{{0.0, 0.6}, {particle_velocity_x, particle_velocity_y}, particle_size, 100*particle_size});
            number_of_balls_to_add--;
            ball_add_counter++;
            particle_counter++;
        }
    } else{
        ball_add_counter++;
    }

    display();

    glutSwapBuffers();

}