#include <valarray>
#include <iostream>
#include <glm/vec2.hpp>
#include "physics_engine.h"

void PhysicsEngine::update_objects() {

    threadPool.dispatch(gameObjects.size(), [&](uint32_t start, uint32_t end) {
        for (uint32_t i = start; i < end; i++) {
            GameObject &gameObject = gameObjects[i];
            gameObject.x += gameObject.vx;
            gameObject.y += gameObject.vy;
            resolveCollisionsWithWalls(gameObject);


            gameObject.vy += gravity;
        }
    });
}

void PhysicsEngine::update() {

    positionBallsInGrid();
    solveCollisions();
    update_objects();
}
glm::vec2 PhysicsEngine::mapToWorldToGrid(const glm::vec2& worldCoord, glm::ivec2 gridSize) {
    // Assuming world coordinates range from -1 to 1
    float normalizedX = (worldCoord.x + 1.0f) / 2.0f;
    float normalizedY = (worldCoord.y + 1.0f) / 2.0f;

    // Mapping normalized coordinates to grid coordinates
    int gridX = static_cast<int>(normalizedX * gridSize.x);
    int gridY = static_cast<int>(normalizedY * gridSize.y);

    // Make sure the grid coordinates are within bounds
    gridX = std::max(0, std::min(gridX, gridSize.x - 1));
    gridY = std::max(0, std::min(gridY, gridSize.y - 1));

    return {static_cast<float>(gridX), static_cast<float>(gridY)};
}



void PhysicsEngine::positionBallsInGrid() {
    grid.clearGrid();

    uint32_t i{0};

    for(auto &gameObject: gameObjects) {
        glm::vec2 gridCoord = mapToWorldToGrid(glm::vec2{gameObject.x, gameObject.y}, getGridSize());
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

    const uint32_t thread_count = threadPool.size <= grid.height * grid.width ? threadPool.size: grid.height * grid.width; // 2

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

    }



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
    constexpr float restitution_coefficient = 0.8f;
    if (gameObjectId == otherBallId) {
        return;
    }
    GameObject& obj1 = gameObjects[gameObjectId];
    GameObject& obj2 = gameObjects[otherBallId];
    float dx = obj2.x - obj1.x;
    float dy = obj2.y - obj1.y;
    float distance2 = dx * dx + dy * dy;
    float r1 = obj1.radius;
    float r2 = obj2.radius;
    // Check if the objects are overlapping
    if (distance2 < (r1 + r2)*(r1 + r2)) {
        float distance = sqrt(distance2);
        obj1.collided = true;
        obj2.collided = true;
        // displace balls
        float overlap = 0.5f * (distance - obj1.radius - obj2.radius);
        obj1.x += overlap * dx / distance;
        obj1.y += overlap * dy / distance;
        obj2.x -= overlap * dx / distance;
        obj2.y -= overlap * dy / distance;

        // Calculate collision normal
        float nx = dx / distance;
        float ny = dy / distance;

        // Calculate relative velocity
        float relative_vx = obj2.vx - obj1.vx;
        float relative_vy = obj2.vy - obj1.vy;
        float relative_speed = relative_vx * nx + relative_vy * ny;

        // Check if objects are moving towards each other
        if (relative_speed < 0) {
            // Calculate impulse
            float impulse = (1 + restitution_coefficient) * relative_speed /
                            (1 / obj1.mass + 1 / obj2.mass);

            // Update velocities
            obj1.vx += impulse * nx / obj1.mass;
            obj1.vy += impulse * ny / obj1.mass;
            obj2.vx -= impulse * nx / obj2.mass;
            obj2.vy -= impulse * ny / obj2.mass;
        }
    }
}
