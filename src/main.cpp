#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include "imgui.h"
#include "backends/imgui_impl_opengl2.h"
#include "backends/imgui_impl_glut.h"
#include "physics/physics_engine.h"
#include "imgui_internal.h"
#include <glm/glm.hpp>

// #define THREADED



GLuint window;
GLuint width = 1000, height = 1000;

int particle_counter = 0;
float particle_velocity_x = 0.001;
float particle_velocity_y = -0.005;
int particle_time_delta = 1;
float particle_size = 0.004;
int particle_segments = 10;
int balls_to_add = 6000;

bool showGrid = false;
bool showBallColor = false;
bool global_particle_size = false;
bool renderGameObjects = true;

float gravity = -0.00f;
int grid_size = 85;
#ifdef THREADED
tp::ThreadPool threadPool(6);
#endif

glm::vec2 lastSourcePosition = glm::vec2(0.0, 0.0);
struct ParticleSource{
    glm::vec2 position;
    glm::vec2 velocity;
    float radius;
    int number_of_balls;
    int particle_time_delta;
    bool active = false;
    int particle_add_counter = 0;
};

std::vector<ParticleSource> particleSources = {
        ParticleSource{glm::vec2(-0.5, -0.3), glm::vec2(particle_velocity_x, particle_velocity_y), particle_size, balls_to_add, particle_time_delta, false},
        ParticleSource{glm::vec2(0.0, 0.0), glm::vec2(particle_velocity_x, particle_velocity_y), particle_size, balls_to_add, particle_time_delta, false},
        ParticleSource{glm::vec2(0.5, 0.3), glm::vec2(particle_velocity_x, particle_velocity_y), particle_size , balls_to_add, particle_time_delta, false},
        ParticleSource{glm::vec2(-0.6, -0.2), glm::vec2(particle_velocity_x, -particle_velocity_y), particle_size , balls_to_add, particle_time_delta, false},
        ParticleSource{glm::vec2(0.1, 0.1), glm::vec2(particle_velocity_x, -particle_velocity_y), particle_size, balls_to_add, particle_time_delta, false},
        ParticleSource{glm::vec2(0.2, 0.1), glm::vec2(particle_velocity_x, -particle_velocity_y), particle_size, balls_to_add, particle_time_delta, false},
        ParticleSource{glm::vec2(0.3, 0.1), glm::vec2(particle_velocity_x, -particle_velocity_y), particle_size, balls_to_add, particle_time_delta, false},
        ParticleSource{glm::vec2(0.4, 0.1), glm::vec2(particle_velocity_x, -particle_velocity_y), particle_size, balls_to_add, particle_time_delta, false},
        ParticleSource{glm::vec2(0.5, 0.1), glm::vec2(particle_velocity_x, -particle_velocity_y), particle_size, balls_to_add, particle_time_delta, false},
        ParticleSource{glm::vec2(0.6 ,0.1), glm::vec2(particle_velocity_x, -particle_velocity_y), particle_size, balls_to_add, particle_time_delta, false},

};

void MainLoopStep();

void displaySourcePoints();
#ifdef THREADED
PhysicsEngine physicsEngine = PhysicsEngine(gravity, threadPool, grid_size);
#else
PhysicsEngine physicsEngine = PhysicsEngine(gravity, grid_size);

#endif
std::vector<GameObject> balls = std::vector<GameObject>();


void drawGrid(glm::ivec2 gridSize){
    int gridHeight = gridSize.y;
    int gridWidth = gridSize.x;
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
    displaySourcePoints();
    auto& gameObjects = physicsEngine.getGameObjects();
    if(showGrid){
        drawGrid(physicsEngine.getGridSize());

    }
    // Draw balls
    float red = 0.0, green = 0.0, blue = 0.0;

    for (size_t i = 0; i < gameObjects.size(); ++i) {
        glm::vec3 color = glm::vec3(0.0, 0.0, 0.0);
        if (showBallColor) {
        getColor(gameObjects[i].gridIndex, red, green, blue);
            color = glm::vec3(red, green, blue);
    }
        else{
            color = gameObjects[i].color;
        }
        GameObject& gameObject = gameObjects[i];
        if(renderGameObjects) {
            drawCircle(gameObject.x, gameObject.y, gameObject.radius, particle_segments, color, gameObject.collided);
        }
        gameObject.collided = false;

    }


}

void displaySourcePoints() {
    glColor3f(1.0, 0.0, 0.0);
    for (auto& source: particleSources) {
        glPointSize(width * source.radius);

        glBegin(GL_POINTS);
        glVertex2f(source.position.x, source.position.y);
        glEnd();    }


}

// Timer callback function for animation
void timer(int) {
    // imgui position in world
    physicsEngine.update();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);  // 60 frames per second
        for (auto& source: particleSources) {
            if(source.active){
                if (source.number_of_balls > 0 && source.particle_add_counter % source.particle_time_delta == 0) {
                    float red = 0.0, green = 0.0, blue = 0.0;
                    getColor(particle_counter, red, green, blue);
                    glm::vec3 color = glm::vec3(red, green, blue);
                    physicsEngine.addGameObject(GameObject{source.position.x, source.position.y, source.velocity.x, source.velocity.y, source.radius, 100*source.radius, color});
                    source.number_of_balls--;
                    particle_counter++;
                }
                source.particle_add_counter++;
            }
        }

}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(100, 100);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutCreateWindow("2D Particle System DEMO");
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

    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGLUT_Shutdown();
    ImGui::DestroyContext();
    return 0;
}

void MainLoopStep()

{


    // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();


    ImGuiIO& io = ImGui::GetIO();

    {
        ImGui::Begin("Interactive GUI panel");
#ifdef THREADED
        ImGui::Text("Thread pool size: %d", threadPool.size);
#endif

        ImGui::SliderInt("Number of particles per source", &balls_to_add, 0, 10000);

        ImGui::Separator();

        ImGui::SliderFloat("Particle velocity x", &particle_velocity_x, -0.1, 0.1);
        ImGui::SliderFloat("Particle velocity y", &particle_velocity_y, -0.1, 0.1);
        if(ImGui::SliderFloat("Particle size", &particle_size, 0.001, 0.02)){
            if(global_particle_size) {
                for (auto &gameObject: physicsEngine.getGameObjects()) {
                    gameObject.radius = particle_size;
                }
            }
        }
        ImGui::SliderInt("Particle segments", &particle_segments, 3, 365);
        if (ImGui::SliderInt("Grid size (nxn)", &grid_size, 1, 500)){
            physicsEngine.resizeGrid(grid_size, grid_size);
        }


        if(ImGui::Button("Start Particle Sources")){
            for(auto &source: particleSources){
                source.active = true;
            }
        }
        if(ImGui::Button("Stop Particle Sources")){
            for(auto &source: particleSources){
                source.active = false;
            }
        }
        if(ImGui::Button("Clear Particle Sources")){
            particleSources.clear();
        }
        if(ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && !ImGui::IsWindowHovered()){
            lastSourcePosition = glm::vec2{io.MousePos.x / width * 2.0f - 1.0f, -(io.MousePos.y / height * 2.0f - 1.0f)};
            particleSources.push_back(ParticleSource{lastSourcePosition, glm::vec2(particle_velocity_x, particle_velocity_y), particle_size, balls_to_add, particle_time_delta, false});
        }
        ImGui::Checkbox("Show grid", &showGrid);
        ImGui::Checkbox("Render game objects", &renderGameObjects);
        ImGui::Checkbox("Show particle grid color", &showBallColor);
        ImGui::Checkbox("Change global particle size", &global_particle_size);


        ImGui::Text("Number of particles: %d", particle_counter);
        ImGui::Text("Number of cells: %d", grid_size*grid_size);

        if(ImGui::Button("Clear particles")){
            physicsEngine.setGameObjects(std::vector<GameObject>());
            particle_counter = 0;
        }

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        glm::vec2 mouseWorldPos = glm::vec2{io.MousePos.x / width * 2.0f - 1.0f, io.MousePos.y / height * 2.0f - 1.0f};
        glm::vec2 mousePosGrid = physicsEngine.mapToWorldToGrid(mouseWorldPos, physicsEngine.getGridSize());
        ImGui::Text("Mouse position x= %.3f y= %.3f, Grid coordinates x=%d, y=%d (index = %d)", mouseWorldPos.x, mouseWorldPos.y, (int)mousePosGrid.x, (int)mousePosGrid.y, (int)(mousePosGrid.x * physicsEngine.getGrid().height + mousePosGrid.y));


        ImGui::End();
    }



    // Rendering
    ImGui::Render();
    glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
    glClearColor(0.5f, 0.5f, 0.5f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    display();

    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());


    glutSwapBuffers();

}