#include <valarray>
#include "physics_engine.h"

void PhysicsEngine::update() {
    for (auto &gameObject : gameObjects) {
        // Update position based on velocity
        gameObject.x += gameObject.vx;
        gameObject.y += gameObject.vy;

        positionBallsInGrid();
        solveCollisions();


        // Apply gravity
        gameObject.vy += gravity;
    }

}

void PhysicsEngine::positionBallsInGrid() {
    grid.clearGrid();

    uint32_t i{0};
    for(auto &gameObject: gameObjects) {
        if(gameObject.x > 1.0f && gameObject.y > 1.0f) {
            grid.addObject(gameObject.x, gameObject.y, i);
        }
        i++;
    }
}

void PhysicsEngine::checkAtomCellCollisions(uint32_t atom_idx, const CollisionCell& c)
{
    for (uint32_t i{0}; i < c.objects_count; ++i) {
        resolveCollisionsWithBalls(gameObjects[atom_idx]);
        resolveCollisionsWithWalls(gameObjects[atom_idx]);
    }
}

void PhysicsEngine::processCell(const CollisionCell& c, uint32_t index)
{
    for (uint32_t i{0}; i < c.objects_count; ++i) {
        const uint32_t atom_idx = c.objects[i];
        checkAtomCellCollisions(atom_idx, grid.gridData[index - 1]);
        checkAtomCellCollisions(atom_idx, grid.gridData[index]);
        checkAtomCellCollisions(atom_idx, grid.gridData[index + 1]);
        checkAtomCellCollisions(atom_idx, grid.gridData[index + grid.height - 1]);
        checkAtomCellCollisions(atom_idx, grid.gridData[index + grid.height    ]);
        checkAtomCellCollisions(atom_idx, grid.gridData[index + grid.height + 1]);
        checkAtomCellCollisions(atom_idx, grid.gridData[index - grid.height - 1]);
        checkAtomCellCollisions(atom_idx, grid.gridData[index - grid.height    ]);
        checkAtomCellCollisions(atom_idx, grid.gridData[index - grid.height + 1]);
    }
}

void PhysicsEngine::solveCollisionThreaded(uint32_t start, uint32_t end)
{
    for (uint32_t idx = 0; idx < end; ++idx) {
        processCell(grid.gridData[idx], idx);
    }
}

void PhysicsEngine::solveCollisions()
{
    // Multi-thread grid
    const uint32_t thread_count = threadPool.size;
    const uint32_t slice_count  = thread_count * 2;
    const uint32_t slice_size   = (grid.width / slice_count) * grid.height;
    const uint32_t last_cell    = (2 * (thread_count - 1) + 2) * slice_size;
    // Find collisions in two passes to avoid data races

    // First collision pass
    for (uint32_t i{0}; i < thread_count; ++i) {
        threadPool.addTask([this, i, slice_size]{
            uint32_t const start{2 * i * slice_size};
            uint32_t const end  {start + slice_size};
            solveCollisionThreaded(start, end);
        });
    }
    // Eventually process rest if the world is not divisible by the thread count
    if (last_cell < grid.gridData.size()) {
        threadPool.addTask([this, last_cell]{
            solveCollisionThreaded(last_cell, grid.gridData.size());
        });
    }
    threadPool.waitTaskCompletion();
    // Second collision pass
    for (uint32_t i{0}; i < thread_count; ++i) {
        threadPool.addTask([this, i, slice_size]{
            uint32_t const start{(2 * i + 1) * slice_size};
            uint32_t const end  {start + slice_size};
            solveCollisionThreaded(start, end);
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

void PhysicsEngine::resolveCollisionsWithBalls(GameObject& gameObject) {
    float velocityLoss = 1; // Adjust the velocity loss factor as needed

    for (auto& otherBall : gameObjects) {
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
}

