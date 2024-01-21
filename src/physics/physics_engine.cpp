#include <valarray>
#include <iostream>
#include <glm/vec2.hpp>
#include "physics_engine.h"
#include "point.h"

void PhysicsEngine::update_objects() {

    threadPool.dispatch(gameObjects.size(), [&](uint32_t start, uint32_t end) {
        for (uint32_t i = start; i < end; i++) {
            GameObject &gameObject = gameObjects[i];
            gameObject.x += gameObject.vx;
            gameObject.y += gameObject.vy;
            resolveCollisionsWithWalls(gameObject);
            gameObject.collided = false;


            float damping = 0.99;
            // Apply gravity
            gameObject.vx *= damping;
            gameObject.vy *= damping;

            gameObject.vy += gravity;
    }
});


void PhysicsEngine::update() {

    positionBallsInGrid();
    solveCollisions();
    update_objects();
}
glm::vec2 PhysicsEngine::mapToWorldToGrid(const glm::vec2& worldCoord, const CollisionGrid& grid) {
    // Assuming world coordinates range from -1 to 1
    float normalizedX = (worldCoord.x + 1.0f) / 2.0f;
    float normalizedY = (worldCoord.y + 1.0f) / 2.0f;

    // Mapping normalized coordinates to grid coordinates
    int gridX = static_cast<int>(normalizedX * grid.width);
    int gridY = static_cast<int>(normalizedY * grid.height);

    // Make sure the grid coordinates are within bounds
    gridX = std::max(0, std::min(gridX, grid.width - 1));
    gridY = std::max(0, std::min(gridY, grid.height - 1));

    return {static_cast<float>(gridX), static_cast<float>(gridY)};
}



void PhysicsEngine::positionBallsInGrid() {
    grid.clearGrid();

    uint32_t i{0};

    for(auto &gameObject: gameObjects) {
        glm::vec2 gridCoord = mapToWorldToGrid(glm::vec2{gameObject.x, gameObject.y}, grid);
        int gridIndex = grid.addObject(gridCoord.x, gridCoord.y, i);
        gameObject.gridIndex = gridIndex;
        i++;
    }
}

void PhysicsEngine::checkAtomCellCollisions(uint32_t atom_idx, const CollisionCell& c)
{
    for (uint32_t i{0}; i < c.objects_count; ++i) {
        resolveCollisionsWithBalls(atom_idx, c.objects[i]);
    }
}

bool checkIndex(uint32_t index, uint32_t width, uint32_t height) {
    return index >= 0 && index < width * height;
}

void PhysicsEngine::processCell(const CollisionCell& c, uint32_t index)
{
    for (uint32_t i{0}; i < c.objects_count; ++i) {
        const uint32_t atom_idx = c.objects[i];
        int indexToCheck = index;

        checkAtomCellCollisions(atom_idx, grid.gridData[indexToCheck]);

        indexToCheck = index - 1;
        if (checkIndex(indexToCheck, grid.width, grid.height)) {
            checkAtomCellCollisions(atom_idx, grid.gridData[indexToCheck]);
        }


        indexToCheck = index + 1;
        if (checkIndex(indexToCheck, grid.width, grid.height)) {
            checkAtomCellCollisions(atom_idx, grid.gridData[indexToCheck]);
        }

        indexToCheck = index + grid.height - 1;
        if (checkIndex(indexToCheck, grid.width, grid.height)) {
            checkAtomCellCollisions(atom_idx, grid.gridData[indexToCheck]);
        }

        indexToCheck = index + grid.height;
        if (checkIndex(indexToCheck, grid.width, grid.height)) {
            checkAtomCellCollisions(atom_idx, grid.gridData[indexToCheck]);
        }

        indexToCheck = index + grid.height + 1;
        if (checkIndex(indexToCheck, grid.width, grid.height)) {
            checkAtomCellCollisions(atom_idx, grid.gridData[indexToCheck]);
        }

        indexToCheck = index - grid.height - 1;
        if (checkIndex(indexToCheck, grid.width, grid.height)) {
            checkAtomCellCollisions(atom_idx, grid.gridData[indexToCheck]);
        }

        indexToCheck = index - grid.height;
        if (checkIndex(indexToCheck, grid.width, grid.height)) {
            checkAtomCellCollisions(atom_idx, grid.gridData[indexToCheck]);
        }

        indexToCheck = index - grid.height + 1;
        if (checkIndex(indexToCheck, grid.width, grid.height)) {
            checkAtomCellCollisions(atom_idx, grid.gridData[indexToCheck]);
        }

    }
}

void PhysicsEngine::solveCollisionThreaded(uint32_t start, uint32_t end)
{
    for(uint32_t i = start; i < end; i++) {
        processCell(grid.gridData[i], i);
    }
}

void PhysicsEngine::solveCollisions()
{

    const uint32_t thread_count = threadPool.size; // 2

    const uint32_t thread_zone_size = ceil((grid.width * grid.height) / thread_count);  // 36 / 2 = 16
    // Find collisions in two passes to avoid data races

    // First collision pass
    for (uint32_t i{0}; i < thread_count; ++i) {
        threadPool.addTask([this, i, thread_zone_size] {
            uint32_t const start = i * thread_zone_size;
            uint32_t const end = start + thread_zone_size;
            solveCollisionThreaded(start, end);
        });
    }

    uint32_t lastCell = thread_count * thread_zone_size;
    if(lastCell < grid.gridData.size()) {
        threadPool.addTask([this, lastCell] {
            solveCollisionThreaded(lastCell, grid.gridData.size());
        });
    }

    threadPool.waitTaskCompletion();

    for (uint32_t i{0}; i < thread_count; ++i) {
        threadPool.addTask([this, i, thread_zone_size]{
            uint32_t const start = i * thread_zone_size;
            uint32_t const end = start + thread_zone_size - 1;
            solveCollisionThreaded(start, end);
        });
    }

    }


//}


void PhysicsEngine::resolveCollisionsWithWalls(GameObject& gameObject) {
        // Check for collision with walls
    float damping = 0.95;
    // Horizontal walls
    if (gameObject.x - gameObject.radius < -1.0) {
        gameObject.x = -1.0 + gameObject.radius;  // Adjust position to be just outside the left wall
        gameObject.vx = std::abs(gameObject.vx) * damping;  // Reverse velocity on collision with the left wall
    } else if (gameObject.x + gameObject.radius > 1.0) {
        gameObject.x = 1.0 - gameObject.radius;  // Adjust position to be just outside the right wall
        gameObject.vx = -std::abs(gameObject.vx) * damping;  // Reverse velocity on collision with the right wall
    }

    // Vertical walls
    if (gameObject.y - gameObject.radius < -1.0) {
        gameObject.y = -1.0 + gameObject.radius;  // Adjust position to be just outside the bottom wall
        gameObject.vy = std::abs(gameObject.vy) * damping;  // Reverse velocity on collision with the bottom wall
    } else if (gameObject.y + gameObject.radius > 1.0) {
        gameObject.y = 1.0 - gameObject.radius;  // Adjust position to be just outside the top wall
        gameObject.vy = -std::abs(gameObject.vy) * damping;  // Reverse velocity on collision with the top wall
    }
}

void PhysicsEngine::resolveCollisionsWithBalls(uint32_t gameObjectId, uint32_t otherBallId) {
    constexpr float response_coef = 1.2f;
    constexpr float eps           = 0.00001f;
    GameObject& obj_1 = gameObjects[gameObjectId];
    GameObject& obj_2 = gameObjects[otherBallId];
    const glm::vec2 o2_o1  = {obj_1.x - obj_2.x, obj_1.y - obj_2.y};
    const float dist2 = o2_o1.x * o2_o1.x + o2_o1.y * o2_o1.y;
    const float radii_sum = obj_1.radius + obj_2.radius;
    if (dist2 < radii_sum*radii_sum && dist2 > eps) {
        obj_1.collided = true;
        obj_2.collided = true;
        const float dist          = sqrt(dist2);
        // Radius are all equal to 1.0f
        const float delta  = response_coef * 0.5f * (radii_sum - dist);
        const glm::vec2 col_vec = (o2_o1 / dist) * delta;
        obj_1.x += col_vec.x;
        obj_1.y += col_vec.y;
        obj_2.x -= col_vec.x;
        obj_2.y -= col_vec.y;
    }
}

