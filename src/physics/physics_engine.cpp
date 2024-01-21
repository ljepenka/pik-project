#include <valarray>
#include <iostream>
#include "physics_engine.h"
#include "point.h"

void PhysicsEngine::update_objects() {

    threadPool.dispatch(gameObjects.size(), [&](uint32_t start, uint32_t end) {
        for (uint32_t i = start; i < end; i++) {
            GameObject &gameObject = gameObjects[i];
            gameObject.x += gameObject.vx;
            gameObject.y += gameObject.vy;
            resolveCollisionsWithWalls(gameObject);

            float damping = 0.99;
            // Apply gravity
            gameObject.vx *= damping;
            gameObject.vy *= damping;

            gameObject.vy += gravity;

        }
    });
}


void PhysicsEngine::update() {

    positionBallsInGrid();
    solveCollisions();
    update_objects();
}

Point mapToWorldToGrid(const Point& worldCoord, const CollisionGrid& grid) {
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
        Point gridCoord = mapToWorldToGrid(Point{gameObject.x, gameObject.y}, grid);
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
    float velocityLoss = 1; // Adjust the velocity loss factor as needed
    GameObject& gameObject = gameObjects[gameObjectId];
    GameObject& otherBall = gameObjects[otherBallId];
    if (&gameObject != &otherBall) { // Avoid self-collision
        float dx = gameObject.x - otherBall.x;
        float dy = gameObject.y - otherBall.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        float minDistance = gameObject.radius + otherBall.radius;

        if (distance < minDistance) {
            // Balls are overlapping, resolve the collision

            // Calculate the unit normal and tangent
            float normalX = dx / distance;
            float normalY = dy / distance;

            // Calculate relative velocity
            float relativeVelocity = (gameObject.vx - otherBall.vx) * normalX +
                                     (gameObject.vy - otherBall.vy) * normalY;

            // Resolve collision along the normal
            if (relativeVelocity < 0) {
                float impulse = (-(1) * relativeVelocity) /
                                (1 / gameObject.mass + 1 / otherBall.mass);
                gameObject.vx += impulse / gameObject.mass * normalX;
                gameObject.vy += impulse / gameObject.mass * normalY;
                otherBall.vx -= impulse / otherBall.mass * normalX;
                otherBall.vy -= impulse / otherBall.mass * normalY;

                // Apply velocity loss
                gameObject.vx *= velocityLoss;
                gameObject.vy *= velocityLoss;
                otherBall.vx *= velocityLoss;
                otherBall.vy *= velocityLoss;

                // Separate the balls using minimum translation vector (MTV)
                float overlap = minDistance - distance;
                float moveX = overlap * normalX;
                float moveY = overlap * normalY;

                gameObject.x += 0.5 * moveX;
                gameObject.y += 0.5 * moveY;
                otherBall.x -= 0.5 * moveX;
                otherBall.y -= 0.5 * moveY;
            }
        }

    }
}

