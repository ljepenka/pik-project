#include <glm/vec2.hpp>

class GameObject {
public:
//    float x{}, y{};      // Position
    glm::vec2 position;
    glm::vec2 acceleration;
    glm::vec2 last_position;
    float vx{}, vy{};    // Velocity
    float radius{};    // Radius
    float mass = 1.0;
    int gridIndex = 0;

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