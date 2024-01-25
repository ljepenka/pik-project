#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

class GameObject {
public:
    glm::vec2 position;
    glm::vec2 last_position = {0.0f, 0.0f};
    glm::vec2 acceleration = {0.0f, 0.0f};

    float radius{};    // Radius
    float mass = 1.0;
    glm::vec3 color = glm::vec3(0.0, 0.0, 0.0);
    int gridIndex = 0;
    int numberOfCollision = 0;
    int numberOfTestsForCollision = 0;
    float cellSizeRatio = 0.0f;



    void update(float dt)
    {
        const glm::vec2 last_update_move = position - last_position;

        const float VELOCITY_DAMPING = 40.0f; // arbitrary, approximating air friction

        const glm::vec2 new_position = position + last_update_move + (acceleration - last_update_move * VELOCITY_DAMPING) * (dt * dt);
        last_position           = position;
        position                = new_position;
        acceleration = {0.0f, 0.0f};
    }

    void setRadius(float radius, float cellSize){
        this->radius = radius;
        this->cellSizeRatio = 2 * radius / cellSize;
    }
};
