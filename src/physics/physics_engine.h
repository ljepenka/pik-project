#include <vector>
#include "game_object.h"
#include "collision_grid.cpp"
#include "../thread_pool/thread_pool.cpp"

class PhysicsEngine {
public:

    PhysicsEngine(float d, ThreadPool& tp) : threadPool(tp), grid(30, 30){
        gravity = d;
    }

    ~PhysicsEngine() {}
    void update();
    void setGameObjects(std::vector<GameObject> gameObjects) { this->gameObjects = gameObjects; }
    void addGameObject(GameObject gameObject) { gameObjects.push_back(gameObject); }
    void resizeGrid(int32_t width, int32_t height) { grid.resize(width, height); }
    std::vector<GameObject> getGameObjects() { return gameObjects; }
    CollisionGrid getGrid() { return grid; }

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


