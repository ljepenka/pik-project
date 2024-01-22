#include <vector>
#include "game_object.h"
#include "collision_grid.cpp"
#include "../thread_pool/thread_pool.cpp"
#include <glm/glm.hpp>
#include "point.h"

class PhysicsEngine {
public:

    PhysicsEngine(float d, ThreadPool& tp, int gridSize) : threadPool(tp), grid(gridSize, gridSize){
        gravity = d;
    }

    ~PhysicsEngine() {}
    void update();
    void setGameObjects(std::vector<GameObject> gameObjects) { this->gameObjects = gameObjects; }
    void addGameObject(GameObject gameObject) { gameObjects.push_back(gameObject); }
    void resizeGrid(int32_t width, int32_t height) { grid.resize(width, height); }
    std::vector<GameObject>& getGameObjects() { return gameObjects; }
    CollisionGrid getGrid() { return grid; }
    glm::ivec2 getGridSize() { return glm::ivec2(grid.width, grid.height); }
    glm::vec2 mapToWorldToGrid(const glm::vec2 &worldCoord, glm::ivec2 gridSize);

private:
    float gravity;  // Gravity force
    std::vector<GameObject> gameObjects;
    CollisionGrid grid;
    ThreadPool& threadPool;

    void positionBallsInGrid();

    void solveCollisions();

    void solveCollisionThreaded(uint32_t start, uint32_t end);

    void processCell(const CollisionCell& c, uint32_t index);

    void checkAtomCellCollisions(uint32_t atom_idx, const CollisionCell& c);

    void update_objects();

    void resolveCollisionsWithWalls(GameObject &gameObject);

    void resolveCollisionsWithBalls(uint32_t gameObject, uint32_t other);

};


