#include <vector>
#include "game_object.h"
#include "collision_grid.cpp"
#include <glm/glm.hpp>
#include <atomic>

class PhysicsEngine {
public:


    PhysicsEngine(glm::vec2 d, int gridSize, int sub_steps) : grid(gridSize, gridSize), sub_steps(sub_steps){
        gravity = d;
    }

    ~PhysicsEngine() {}
    void update();
    void setGameObjects(std::vector<GameObject> gameObjects) { this->gameObjects = gameObjects; }
    void addGameObject(GameObject gameObject) { gameObjects.push_back(gameObject); }
    void resizeGrid(int32_t width, int32_t height);
    std::vector<GameObject>& getGameObjects() { return gameObjects; }
    CollisionGrid& getGrid() { return grid; }
    glm::ivec2 getGridSize() { return glm::ivec2(grid.width, grid.height); }
    std::vector<glm::ivec2> mapWorldToMultipleGrid(const glm::vec2 &worldCoord, glm::ivec2 gridSize, float radius);
    glm::ivec2 mapWorldToGrid(const glm::vec2 &worldCoord, glm::ivec2 gridSize);

    float getCellSize() {return 2.0f / grid.height; }
    void setThreadCount(int threadCount) { this->threadCount = threadCount; }
    void update(float dt);
    void setSubSteps(int sub_steps) { this->sub_steps = sub_steps; }


    void setGravity(glm::vec2 gravity) { this->gravity = gravity; }

    int getCollisionCount() { return collisionCount; }
    int getCollisionTestCount() { return collisionTestCount; }

private:
    glm::vec2 gravity;  // Gravity force
    std::vector<GameObject> gameObjects;
    int threadCount = 1;
    int sub_steps = 8;
    std::atomic<int>  collisionCount = 0;
    std::atomic<int>  collisionTestCount = 0;

    CollisionGrid grid;

    void positionBallsInGrid();

    void solveCollisions();

    void solveCollisionThreaded(uint32_t start, uint32_t end);

    void processCell(const CollisionCell& c, uint32_t index);

    void checkAtomCellCollisions(uint32_t atom_idx, const CollisionCell& c);

    void resolveCollisionsWithWalls(GameObject &gameObject);

    void positionBallsThreaded(int start, int end);


    void solveContact(uint32_t atom_1_idx, uint32_t atom_2_idx);

    void updateObjectsThreader(float dt);

    void updateObjects(int start, int end, float dt);
};


