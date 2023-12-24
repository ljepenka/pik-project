#include <valarray>
#include "physics_engine.h"

void PhysicsEngine::update() {
    for (auto &gameObject : gameObjects) {
        // Update position based on velocity
        gameObject.x += gameObject.vx;
        gameObject.y += gameObject.vy;

        resolveCollisionsWithBalls(gameObject);
        resolveCollisionsWithWalls(gameObject);


        // Apply gravity
        gameObject.vy += gravity;
    }

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

