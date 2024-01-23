#include <vector>
#include "game_object.h"
#include "collision_grid.cpp"
#include "../thread_pool/thread_pool.cpp"
#include <glm/glm.hpp>
#include "point.h"

// #define THREADED

class PhysicsEngine {
public:
#ifdef THREADED

    PhysicsEngine(float d, tp::ThreadPool& tp, int gridSize) : threadPool(tp), grid(gridSize, gridSize){
        gravity = d;
    }
#else

    PhysicsEngine(glm::vec2 d, int gridSize) : grid(gridSize, gridSize){
        gravity = d;
    }
#endif

    ~PhysicsEngine() {}
    void update();
    void setGameObjects(std::vector<GameObject> gameObjects) { this->gameObjects = gameObjects; }
    void addGameObject(GameObject gameObject) { gameObjects.push_back(gameObject); }
    void resizeGrid(int32_t width, int32_t height) { grid.resize(width, height); }
    std::vector<GameObject>& getGameObjects() { return gameObjects; }
    CollisionGrid& getGrid() { return grid; }
    glm::ivec2 getGridSize() { return glm::ivec2(grid.width, grid.height); }
    glm::vec2 mapToWorldToGrid(const glm::vec2 &worldCoord, glm::ivec2 gridSize);
    float getCellSize() {return 2.0f / grid.height; }
    void setThreadCount(int threadCount) { this->threadCount = threadCount; }
    void update_objects(float dt);
    void update(float dt);



private:
    glm::vec2 gravity;  // Gravity force
    std::vector<GameObject> gameObjects;
    int threadCount = 1;
#ifdef THREADED
    tp::ThreadPool& threadPool;
#endif

    CollisionGrid grid;

    void positionBallsInGrid();

    void solveCollisions();

    void solveCollisionThreaded(uint32_t start, uint32_t end);

    void processCell(const CollisionCell& c, uint32_t index);

    void checkAtomCellCollisions(uint32_t atom_idx, const CollisionCell& c);

    void update_objects();

    void resolveCollisionsWithWalls(GameObject &gameObject);

    void resolveCollisionsWithBalls(uint32_t gameObject, uint32_t other);

    void positionBallsThreaded(int start, int end);


    void solveContact(uint32_t atom_1_idx, uint32_t atom_2_idx);
};


