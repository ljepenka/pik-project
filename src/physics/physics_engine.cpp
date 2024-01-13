#include <valarray>
#include <iostream>
#include "physics_engine.h"
#include "point.h"

void PhysicsEngine::update() {
    positionBallsInGrid();
    solveCollisions();

    for (auto &gameObject : gameObjects) {
        // Update position based on velocity
        gameObject.x += gameObject.vx;
        gameObject.y += gameObject.vy;

        resolveCollisionsWithWalls(gameObject);



        // Apply gravity
        gameObject.vy += gravity;
    }

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
//        if(gameObject.x > 1.0f && gameObject.y > 1.0f) {
        Point gridCoord = mapToWorldToGrid(Point{gameObject.x, gameObject.y}, grid);
        grid.addObject(gridCoord.x, gridCoord.y, i);
//        }
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

void PhysicsEngine::solveCollisionThreaded(int cellNumber)
{
    processCell(grid.gridData[cellNumber], cellNumber);

}

void PhysicsEngine::solveCollisions()
{
    // Multi-thread grid
//    const uint32_t thread_count = threadPool.size;
//    const uint32_t slice_count  = thread_count * 2;
//    const uint32_t slice_size   = (grid.width / slice_count) * grid.height;
//    const uint32_t last_cell    = (2 * (thread_count - 1) + 2) * slice_size;
    // Find collisions in two passes to avoid data races

    // First collision pass
    for (uint32_t i{0}; i < grid.width * grid.height; ++i) {
        solveCollisionThreaded(i);
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

