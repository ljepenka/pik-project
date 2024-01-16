#include <vector>
#include "game_object.h"

class PhysicsEngine {
public:

    PhysicsEngine(float d) {
        gravity = d;
    }

    ~PhysicsEngine() {}
    void update();
    void setGameObjects(std::vector<GameObject> gameObjects) { this->gameObjects = gameObjects; }
    void addGameObject(GameObject gameObject) { gameObjects.push_back(gameObject); }
    std::vector<GameObject> getGameObjects() { return gameObjects; }


private:
    float gravity;  // Gravity force
    std::vector<GameObject> gameObjects;

    void resolveCollisionsWithWalls(GameObject &gameObject);

    void resolveCollisionsWithBalls(GameObject &gameObject, GameObject &otherBall);

    void solveCollisions();
};


