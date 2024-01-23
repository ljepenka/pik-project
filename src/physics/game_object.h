#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

class GameObject {
public:
//    float x{}, y{};      // Position
    glm::vec2 position;
    glm::vec2 last_position = {0.0f, 0.0f};
//    float vx{}, vy{};    // Velocity
   // glm::vec2 velocity;
    glm::vec2 acceleration = {0.0f, 0.0f};

    float radius{};    // Radius
    float mass = 1.0;
    glm::vec3 color = glm::vec3(0.0, 0.0, 0.0);
    int gridIndex = 0;
    bool collided = false;


    void update(float dt)
    {
        const glm::vec2 last_update_move = position - last_position;

        const float VELOCITY_DAMPING = 40.0f; // arbitrary, approximating air friction

        const glm::vec2 new_position = position + last_update_move + (acceleration - last_update_move * VELOCITY_DAMPING) * (dt * dt);
        last_position           = position;
        position                = new_position;
        acceleration = {0.0f, 0.0f};
    }
};

struct Rectangle {
    glm::vec2 leftPos, rightPos;
};
