#include <GL/glut.h>
#include <cmath>



void MainLoopStep();

// Ball structure to store ball properties
struct Ball {
    float x, y;      // Position
    float vx, vy;    // Velocity
    float radius;    // Radius
};

const int numBalls = 2;
Ball balls[numBalls];

const float gravity = -0.001;  // Gravity force

// Initialize ball properties
void initializeBalls() {
    balls[0] = {0.0, 0.0, 0.02, 0.01, 0.1};
    balls[1] = {1.0, 0.5, -0.01, 0.02, 0.15};
}

// Update ball positions based on velocity and handle collisions
void updateBalls() {
    for (int i = 0; i < numBalls; ++i) {
        // Update position based on velocity
        balls[i].x += balls[i].vx;
        balls[i].y += balls[i].vy;

        // Check for collision with walls
        if (balls[i].x - balls[i].radius < -1.0 || balls[i].x + balls[i].radius > 1.0) {
            balls[i].vx *= -1.0;  // Reverse velocity on collision with horizontal walls
        }
        if (balls[i].y - balls[i].radius < -1.0 || balls[i].y + balls[i].radius > 1.0) {
            balls[i].vy *= -1.0;  // Reverse velocity on collision with vertical walls
        }

        // Apply gravity
        balls[i].vy += gravity;
    }

    // Check for collision between balls
    for (int i = 0; i < numBalls; ++i) {
        for (int j = i + 1; j < numBalls; ++j) {
            float dx = balls[j].x - balls[i].x;
            float dy = balls[j].y - balls[i].y;
            float distance = std::sqrt(dx * dx + dy * dy);

            // If the distance between the centers of the two balls is less than the sum of their radii, they collide
            if (distance < balls[i].radius + balls[j].radius) {
                // Swap velocities to simulate elastic collision
                std::swap(balls[i].vx, balls[j].vx);
                std::swap(balls[i].vy, balls[j].vy);
            }
        }
    }
}

// Display callback function
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0, 1.0, 1.0);  // White color

    // Draw balls
    for (int i = 0; i < numBalls; ++i) {
        glBegin(GL_TRIANGLE_FAN);
        for (int j = 0; j <= 360; ++j) {
            float angle = j * 3.14159265 / 180.0;
            float x = balls[i].x + balls[i].radius * std::cos(angle);
            float y = balls[i].y + balls[i].radius * std::sin(angle);
            glVertex2f(x, y);
        }
        glEnd();
    }

    glutSwapBuffers();
}

// Timer callback function for animation
void timer(int) {
    updateBalls();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);  // 60 frames per second
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutCreateWindow("2D Ball Simulation");
    glutDisplayFunc(MainLoopStep);
    glutTimerFunc(0, timer, 0);
    glClearColor(0.0, 0.0, 0.0, 1.0);  // Black background
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);  // Set the coordinate system
    initializeBalls();


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    // FIXME: Consider reworking this example to install our own GLUT funcs + forward calls ImGui_ImplGLUT_XXX ones, instead of using ImGui_ImplGLUT_InstallFuncs().
    ImGui_ImplGLUT_Init();
    ImGui_ImplOpenGL2_Init();

    // Install GLUT handlers (glutReshapeFunc(), glutMotionFunc(), glutPassiveMotionFunc(), glutMouseFunc(), glutKeyboardFunc() etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    ImGui_ImplGLUT_InstallFuncs();



    glutMainLoop();
    return 0;
}


void MainLoopStep()

{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO();




    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static int particleCount = 10;
        static int counter = 0;

        ImGui::Begin("Particle system!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("Number of particles to add from current source.");               // Display some text (you can use a format strings too)

        ImGui::SliderInt("particle to add", &particleCount, 0, 100);            // Edit 1 float using a slider from 0.0f to 1.0f
        if(ImGui::ColorEdit3("Start color", (float*)&startColor)){
            particleSystem.startColor = {startColor.x, startColor.y, startColor.z};
        }// Edit 3 floats representing a color
        if(ImGui::ColorEdit3("End color", (float*)&endColor)){
            particleSystem.endColor = {endColor.x, endColor.y, endColor.z};

        }// Edit 3 floats representing a color

        if(ImGui::SliderFloat("Particles size, ", &particleSize, 0.05f, 2.5f)){
            particleSystem.particleSize = particleSize;
        }

        if (ImGui::Button("Add particles"))   {
            counter += particleCount;
            particleSystem.create_particles(particleCount);

        }                         // Buttons return true when clicked (most widgets return true when edited/activated)
        if(ImGui::IsMouseClicked(1)){
            particleSystem.center = {io.MousePos.x/ (float)width * 2.0f - 1.0f, -io.MousePos.y/ (float)height * 2.0f + 1.0f, 0};
        }
        if(ImGui::IsKeyPressed(ImGuiKey_W, false) ){
            cameraPosZ -= cameraSpeed;

        }
        if(ImGui::IsKeyPressed(ImGuiKey_A, false) ){

            cameraPosX -= cameraSpeed;


        }
        if(ImGui::IsKeyPressed(ImGuiKey_S, false) ){
            cameraPosZ += cameraSpeed;
        }
        if(ImGui::IsKeyPressed(ImGuiKey_D, false) ){

            cameraPosX += cameraSpeed;

        }


        glOrtho(-1, 1, -1, 1, -5 , 5);    //	okomita projekcija


        glMatrixMode(GL_MODELVIEW);        //	matrica projekcije
        glm::mat4 viewMatrix = glm::lookAt(glm::vec3(cameraPosX, cameraPosY, cameraPosZ), glm::vec3(cameraPosX, cameraPosY, cameraPosZ-5.f), glm::vec3(0, 1, 0));

//        glLoadIdentity();  //	jedinicna matrica
        glLoadMatrixf(&viewMatrix[0][0]);
        ImGui::SameLine();
        ImGui::Text("Particle Count = %d", counter);

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
    myDisplay();


    glutSwapBuffers();
    glutPostRedisplay();

}