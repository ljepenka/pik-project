class GameObject {
public:
    float x{}, y{};      // Position
    float vx{}, vy{};    // Velocity
    float radius{};    // Radius
    float mass = 1.0;
    int gridIndex = 0;
    bool collided = false;
};