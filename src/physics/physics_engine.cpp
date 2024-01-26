#include <valarray>
#include <glm/vec2.hpp>
#include "physics_engine.h"


void PhysicsEngine::updateBallsThreaded(float dt){
    if(threadCount > 1) {
        const uint32_t thread_count = threadCount;

        const uint32_t thread_zone_size = gameObjects.size() / thread_count;

        std::vector<std::thread> my_threads;
        for (int i = 0; i < thread_count; i++) {
            uint32_t const start = i * thread_zone_size;
            uint32_t const end = start + thread_zone_size;
            my_threads.emplace_back(&PhysicsEngine::updateBalls, this, start, end, dt);
        }
        auto original_thread = my_threads.begin();
        while (original_thread != my_threads.end()) {
            original_thread->join();
            original_thread++;
        }

        if (gameObjects.size() % thread_count != 0) {
            updateBalls(gameObjects.size() - (gameObjects.size() % thread_count), gameObjects.size(), dt);
        }
    }
    else{
        updateBalls(0, gameObjects.size(), dt);
    }

}
void PhysicsEngine::updateBalls(int start, int end, float dt) {

    for (int i = start; i < end; i++) {
        GameObject& ball = gameObjects[i];
        ball.acceleration += gravity;
        ball.update(dt);
        resolveCollisionsWithWalls(ball);
        collisionCount += ball.numberOfCollision;
        collisionTestCount += ball.numberOfTestsForCollision;
    }

}

void PhysicsEngine::update(float dt) {

    const float sub_dt = dt / static_cast<float>(sub_steps);
    for (uint32_t i(sub_steps); i--;) {
        positionBallsInGrid();
        solveBallCollisions();
        updateBallsThreaded(sub_dt);
    }


}

glm::ivec2 PhysicsEngine::mapWorldToGrid(const glm::vec2& worldCoord, glm::ivec2 gridSize) {

    float normalizedX = (worldCoord.x + 1.0f) / 2.0f;
    float normalizedY = (worldCoord.y + 1.0f) / 2.0f;

    // Mapping normalized coordinates to grid coordinates
    int gridX = static_cast<int>(normalizedX * gridSize.x);
    int gridY = static_cast<int>(normalizedY * gridSize.y);

    // Making sure that grid coordinates are within bounds
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
        int gridIndex = grid.addBall(gridCoord.x, gridCoord.y, i, gameObject.cellSizeRatio);
        gameObject.gridIndex = gridIndex;

    }
}
void PhysicsEngine::positionBallsInGrid() {

     grid.clearGrid(threadCount);
     if(threadCount > 1) {
         const uint32_t thread_count = threadCount;

         const uint32_t thread_zone_size = gameObjects.size() / thread_count;

         std::vector<std::thread> my_threads;
         for (int i = 0; i < thread_count; i++) {
             uint32_t const start = i * thread_zone_size;
             uint32_t const end = start + thread_zone_size;
             my_threads.emplace_back(&PhysicsEngine::positionBallsThreaded, this, start, end);
         }
         auto original_thread = my_threads.begin();
         while (original_thread != my_threads.end()) {
             original_thread->join();
             original_thread++;
         }
         if (gameObjects.size() % thread_count != 0) {
             positionBallsThreaded(gameObjects.size() - (gameObjects.size() % thread_count), gameObjects.size());
         }
     }
     else {
         positionBallsThreaded(0, gameObjects.size());
     }
}

void PhysicsEngine::checkCellForBallCollisions(uint32_t atom_idx, const GridCell& c)
{
    for (uint32_t i{0}; i < c.ball_counter; ++i) {
        solveContact(atom_idx, c.balls[i]);
    }
}

bool checkIndex(uint32_t index, uint32_t width, uint32_t height) {
    return  index < width * height;
}

void PhysicsEngine::checkNeighbourCells(const GridCell& c, uint32_t index)
{
    for (uint32_t i{0}; i < c.ball_counter; ++i) {
        const uint32_t atom_idx = c.balls[i];
        int indexToCheck = index;

        int cellMultiplier = ceil(c.maxRadiusRatio);
        for(int j = -cellMultiplier; j < 2*cellMultiplier; j++){
            for(int k = -cellMultiplier * grid.height; k < cellMultiplier * grid.height; k+=grid.height){
                indexToCheck = index + j + k;
                if (checkIndex(indexToCheck, grid.width, grid.height)) {
                    checkCellForBallCollisions(atom_idx, grid.gridCells[indexToCheck]);
                }
            }
        }

    }
}

void PhysicsEngine::threadedCollisionSolve(uint32_t start, uint32_t end)
{
    for(uint32_t i = start; i < end; i++) {
        checkNeighbourCells(grid.gridCells[i], i);
    }
}

// Check if two gameObjects are colliding and if so create a new contact
void PhysicsEngine::solveContact(uint32_t atom_1_idx, uint32_t atom_2_idx)
{
    if(atom_1_idx == atom_2_idx) {
        return;
    }
    constexpr float response_coef = 1.0f;
    constexpr float eps           = 0.00001f;
    GameObject& ball_1 = gameObjects[atom_1_idx];
    GameObject& ball_2 = gameObjects[atom_2_idx];
    ball_1.numberOfTestsForCollision++;
    ball_2.numberOfTestsForCollision++;
    const glm::vec2 o2_o1  = ball_1.position - ball_2.position;
    const float dist2 = o2_o1.x * o2_o1.x + o2_o1.y * o2_o1.y;
    if (dist2 < (ball_1.radius + ball_2.radius) * (ball_1.radius + ball_2.radius) && dist2 > eps ) {
        const float dist          = sqrt(dist2);
        // Radius are all equal to 1.0f
        const float delta  = response_coef * 0.5f * (ball_2.radius + ball_1.radius - dist);
        const glm::vec2 col_vec = (o2_o1 / dist) * delta;
        ball_1.position += col_vec;
        ball_2.position -= col_vec;
        ball_1.numberOfCollision++;
        ball_2.numberOfCollision++;
    }
}


void PhysicsEngine::solveBallCollisions() {
    if(threadCount > 1 ) {
        const uint32_t thread_count = threadCount;

        const uint32_t thread_zone_size = ceil((grid.width * grid.height) / thread_count);

        std::vector<std::thread> my_threads;
        for (int i = 0; i < thread_count; i++) {
            uint32_t const start = i * thread_zone_size;
            uint32_t const end = start + thread_zone_size;
            my_threads.emplace_back(&PhysicsEngine::threadedCollisionSolve, this, start, end);
        }
        auto original_thread = my_threads.begin();
        //Do other stuff here.
        while (original_thread != my_threads.end()) {
            original_thread->join();
            original_thread++;
        }
        if (grid.width * grid.height % thread_count != 0) {
            threadedCollisionSolve(grid.width * grid.height - (grid.width * grid.height % thread_count),
                                   grid.width * grid.height);
        }
    }
    else {
        threadedCollisionSolve(0, grid.width * grid.height);
    }

}



void PhysicsEngine::resolveCollisionsWithWalls(GameObject& gameObject) {
    // Check for collision with walls
    float damping = 0.95;
    // Horizontal walls
    if (gameObject.position.x - gameObject.radius < -1.0) {
        gameObject.position.x = -1.0 + gameObject.radius;  // Adjust position to be just outside the left wall
    } else if (gameObject.position.x + gameObject.radius > 1.0) {
        gameObject.position.x = 1.0 - gameObject.radius;  // Adjust position to be just outside the right wall
    }

    // Vertical walls
    if (gameObject.position.y - gameObject.radius < -1.0) {
        gameObject.position.y = -1.0 + gameObject.radius;  // Adjust position to be just outside the bottom wall
    } else if (gameObject.position.y + gameObject.radius > 1.0) {
        gameObject.position.y = 1.0 - gameObject.radius;  // Adjust position to be just outside the top wall
    }
}


void PhysicsEngine::resizeGrid(int32_t width, int32_t height) {
    grid.resize(width, height);
    for(auto& gameObject: gameObjects) {
       gameObject.setRadius(gameObject.radius, getCellSize());
    }
}


