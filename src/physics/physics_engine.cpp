#include <valarray>
#include <glm/vec2.hpp>
#include "physics_engine.h"


void PhysicsEngine::updateObjectsThreader(float dt){
    if(threadCount > 1) {
        const uint32_t thread_count = threadCount;

        const uint32_t thread_zone_size = gameObjects.size() / thread_count;
        // Find collisions in two passes to avoid data races

        // First collision pass
        std::vector<std::thread> mythreads;
        for (int i = 0; i < thread_count; i++) {
            uint32_t const start = i * thread_zone_size;
            uint32_t const end = start + thread_zone_size;
            mythreads.emplace_back(&PhysicsEngine::updateObjects, this, start, end, dt);
        }
        auto originalthread = mythreads.begin();
        //Do other stuff here.
        while (originalthread != mythreads.end()) {
            originalthread->join();
            originalthread++;
        }
        if (gameObjects.size() % thread_count != 0) {
            updateObjects(gameObjects.size() - (gameObjects.size() % thread_count), gameObjects.size(), dt);
        }
    }
    else{
        updateObjects(0, gameObjects.size(), dt);
    }

}
void PhysicsEngine::updateObjects(int start, int end, float dt) {

    for (int i = start; i < end; i++) {
        GameObject& gameObject = gameObjects[i];
        gameObject.acceleration += gravity;
        gameObject.update(dt);
        resolveCollisionsWithWalls(gameObject);
        collisionCount += gameObject.numberOfCollision;
        collisionTestCount += gameObject.numberOfTestsForCollision;
    }

}

void PhysicsEngine::update(float dt) {

    const float sub_dt = dt / static_cast<float>(sub_steps);
    for (uint32_t i(sub_steps); i--;) {
        positionBallsInGrid();
        solveCollisions();
        updateObjectsThreader(sub_dt);
    }


}

glm::ivec2 PhysicsEngine::mapWorldToGrid(const glm::vec2& worldCoord, glm::ivec2 gridSize) {

    float normalizedX = (worldCoord.x + 1.0f) / 2.0f;
    float normalizedY = (worldCoord.y + 1.0f) / 2.0f;

    // Mapping normalized coordinates to grid coordinates
    int gridX = static_cast<int>(normalizedX * gridSize.x);
    int gridY = static_cast<int>(normalizedY * gridSize.y);

    // Make sure the grid coordinates are within bounds
    gridX = std::max(0, std::min(gridX, gridSize.x - 1));
    gridY = std::max(0, std::min(gridY, gridSize.y - 1));

    return {gridX, gridY};

}


void PhysicsEngine::positionBallsThreaded(int start, int end){
    collisionTestCount = 0;
    collisionCount = 0;
    for(int i = start; i < end; i++) {
        GameObject& gameObject = gameObjects[i];
        gameObject.numberOfCollision = 0;
        gameObject.numberOfTestsForCollision = 0;
        glm::ivec2 gridCoord = mapWorldToGrid(gameObject.position, getGridSize());
        int gridIndex = grid.addObject(gridCoord.x, gridCoord.y, i, gameObject.cellSizeRatio);
        gameObject.gridIndex = gridIndex;

    }
}
void PhysicsEngine::positionBallsInGrid() {

     grid.clearGrid(threadCount);
     if(threadCount > 1) {
         const uint32_t thread_count = threadCount;

         const uint32_t thread_zone_size = gameObjects.size() / thread_count;

         std::vector<std::thread> mythreads;
         for (int i = 0; i < thread_count; i++) {
             uint32_t const start = i * thread_zone_size;
             uint32_t const end = start + thread_zone_size;
             mythreads.emplace_back(&PhysicsEngine::positionBallsThreaded, this, start, end);
         }
         auto originalthread = mythreads.begin();
         while (originalthread != mythreads.end()) {
             originalthread->join();
             originalthread++;
         }
         if (gameObjects.size() % thread_count != 0) {
             positionBallsThreaded(gameObjects.size() - (gameObjects.size() % thread_count), gameObjects.size());
         }
     }
     else {
         positionBallsThreaded(0, gameObjects.size());
     }
}

void PhysicsEngine::checkAtomCellCollisions(uint32_t atom_idx, const CollisionCell& c)
{
    for (uint32_t i{0}; i < c.objects_count; ++i) {
        solveContact(atom_idx, c.objects[i]);
    }
}

bool checkIndex(uint32_t index, uint32_t width, uint32_t height) {
    return  index < width * height;
}

void PhysicsEngine::processCell(const CollisionCell& c, uint32_t index)
{
    for (uint32_t i{0}; i < c.objects_count; ++i) {
        const uint32_t atom_idx = c.objects[i];
        int indexToCheck = index;

//        checkAtomCellCollisions(atom_idx, grid.gridData[indexToCheck]);
        int cellMultiplier = ceil(c.maxRadiusRatio);
        for(int j = -cellMultiplier; j < 2*cellMultiplier; j++){
            for(int k = -cellMultiplier * grid.height; k < cellMultiplier * grid.height; k+=grid.height){
                indexToCheck = index + j + k;
                if (checkIndex(indexToCheck, grid.width, grid.height)) {
                    checkAtomCellCollisions(atom_idx, grid.gridData[indexToCheck]);
                }
            }
        }

    }
}

void PhysicsEngine::solveCollisionThreaded(uint32_t start, uint32_t end)
{
    for(uint32_t i = start; i < end; i++) {
        processCell(grid.gridData[i], i);
    }
}

// Checks if two atoms are colliding and if so create a new contact
void PhysicsEngine::solveContact(uint32_t atom_1_idx, uint32_t atom_2_idx)
{
    if(atom_1_idx == atom_2_idx) {
        return;
    }
    constexpr float response_coef = 1.0f;
    constexpr float eps           = 0.00001f;
    GameObject& obj_1 = gameObjects[atom_1_idx];
    GameObject& obj_2 = gameObjects[atom_2_idx];
    obj_1.numberOfTestsForCollision++;
    obj_2.numberOfTestsForCollision++;
    const glm::vec2 o2_o1  = obj_1.position - obj_2.position;
    const float dist2 = o2_o1.x * o2_o1.x + o2_o1.y * o2_o1.y;
    if (dist2 < (obj_1.radius + obj_2.radius) * (obj_1.radius + obj_2.radius) && dist2 > eps ) {
        const float dist          = sqrt(dist2);
        // Radius are all equal to 1.0f
        const float delta  = response_coef * 0.5f * (obj_2.radius + obj_1.radius - dist);
        const glm::vec2 col_vec = (o2_o1 / dist) * delta;
        obj_1.position += col_vec;
        obj_2.position -= col_vec;
        obj_1.numberOfCollision++;
        obj_2.numberOfCollision++;
    }
}


void PhysicsEngine::solveCollisions() {
    if(threadCount > 1 ) {
        const uint32_t thread_count = threadCount;

        const uint32_t thread_zone_size = ceil((grid.width * grid.height) / thread_count);

        std::vector<std::thread> mythreads;
        for (int i = 0; i < thread_count; i++) {
            uint32_t const start = i * thread_zone_size;
            uint32_t const end = start + thread_zone_size;
            mythreads.emplace_back(&PhysicsEngine::solveCollisionThreaded, this, start, end);
        }
        auto originalthread = mythreads.begin();
        //Do other stuff here.
        while (originalthread != mythreads.end()) {
            originalthread->join();
            originalthread++;
        }
        if (grid.width * grid.height % thread_count != 0) {
            solveCollisionThreaded(grid.width * grid.height - (grid.width * grid.height % thread_count),
                                   grid.width * grid.height);
        }
    }
    else {
        solveCollisionThreaded(0, grid.width * grid.height);
    }

}



void PhysicsEngine::resolveCollisionsWithWalls(GameObject& gameObject) {
        // Check for collision with walls
    float damping = 0.95;
    // calulcate margin to be cell size
    // Horizontal walls
    if (gameObject.position.x - gameObject.radius < -1.0) {
        gameObject.position.x = -1.0 + gameObject.radius;  // Adjust position to be just outside the left wall
        //gameObject.velocity.x = std::abs(gameObject.velocity.x) * damping;  // Reverse velocity on collision with the left wall
    } else if (gameObject.position.x + gameObject.radius > 1.0) {
        gameObject.position.x = 1.0 - gameObject.radius;  // Adjust position to be just outside the right wall
        //gameObject.velocity.x = -std::abs(gameObject.velocity.x) * damping;  // Reverse velocity on collision with the right wall
    }

    // Vertical walls
    if (gameObject.position.y - gameObject.radius < -1.0) {
        gameObject.position.y = -1.0 + gameObject.radius;  // Adjust position to be just outside the bottom wall
        //gameObject.velocity.y = std::abs(gameObject.velocity.y) * damping;  // Reverse velocity on collision with the bottom wall
    } else if (gameObject.position.y + gameObject.radius > 1.0) {
        gameObject.position.y = 1.0 - gameObject.radius;  // Adjust position to be just outside the top wall
        //gameObject.velocity.y = -std::abs(gameObject.velocity.y) * damping;  // Reverse velocity on collision with the top wall
    }
}


void PhysicsEngine::resizeGrid(int32_t width, int32_t height) {
    grid.resize(width, height);
    for(auto& gameObject: gameObjects) {
       gameObject.setRadius(gameObject.radius, getCellSize());
    }
}


