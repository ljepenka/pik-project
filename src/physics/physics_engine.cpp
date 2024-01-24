#include <valarray>
#include <glm/vec2.hpp>
#include "physics_engine.h"

void PhysicsEngine::update_objects(float dt) {

    for (auto &gameObject: gameObjects) {
        gameObject.acceleration += gravity;
        gameObject.update(dt);
        resolveCollisionsWithWalls(gameObject);

    }

}

void PhysicsEngine::update(float dt) {

    const float sub_dt = dt / static_cast<float>(sub_steps);
    for (uint32_t i(sub_steps); i--;) {
        positionBallsInGrid();
        solveCollisions();
        grid.clearGrid();
        update_objects(sub_dt);
    }


}
std::vector<glm::ivec2> PhysicsEngine::mapWorldToMultipleGrid(const glm::vec2& worldCoord, glm::ivec2 gridSize, float radius) {
    std::vector<glm::ivec2> gridCoords = {};

//    // Assuming world coordinates range from -1 to 1

    float x = worldCoord.x;
    float y = worldCoord.y;
    float R = radius;
    int gridWidth = gridSize.x;
    int gridHeight = gridSize.y;

    // create a vector to store the cells that intersect with the circle
    int xMin = std::max(0, static_cast<int>(std::floor((x - R) * gridWidth / 2 + gridWidth / 2)));
    int yMin = std::max(0, static_cast<int>(std::floor((y - R) * gridHeight / 2 + gridHeight / 2)));
    int xMax = std::min(gridWidth - 1, static_cast<int>(std::floor((x + R) * gridWidth / 2 + gridWidth / 2)));
    int yMax = std::min(gridHeight - 1, static_cast<int>(std::floor((y + R) * gridHeight / 2 + gridHeight / 2)));

    if(xMin >= gridWidth){
        xMin = gridWidth-1;
    }
    if(yMin >= gridHeight){
        yMin = gridHeight-1;
    }
    if(xMax < 0){
        xMax = 0;
    }
    if(yMax < 0){
        yMax = 0;
    }
    // Add all cells in the bounding box that are fully or partially covered by the circle
    for (int yIndex = yMin; yIndex <= yMax; ++yIndex) {
        for (int xIndex = xMin; xIndex <= xMax; ++xIndex) {
            gridCoords.emplace_back(xIndex, yIndex);
        }
    }
    return gridCoords;
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

//    uint32_t i{0};
    std::vector<int> result = {};
    for(int i = start; i < end; i++) {
        GameObject& gameObject = gameObjects[i];
        glm::ivec2 gridCoord = mapWorldToGrid(gameObject.position, getGridSize());
        int gridIndex = grid.addObject(gridCoord.x, gridCoord.y, i);
        result.push_back(gridIndex);
        gameObject.gridIndex = gridIndex;

    }
}
void PhysicsEngine::positionBallsInGrid() {

    grid.clearGrid();
    const uint32_t  thread_count = threadCount;

    const uint32_t thread_zone_size = gameObjects.size() / thread_count;
    // Find collisions in two passes to avoid data races

    // First collision pass
    std::vector<std::thread> mythreads;
    for (int i = 0; i < thread_count; i++)
    {
        uint32_t const start = i * thread_zone_size;
        uint32_t const end = start + thread_zone_size;
        mythreads.emplace_back(&PhysicsEngine::positionBallsThreaded, this, start, end);
    }
    auto originalthread = mythreads.begin();
    //Do other stuff here.
    while (originalthread != mythreads.end()) {
        originalthread->join();
        originalthread++;
    }
    if(gameObjects.size() % thread_count != 0) {
        positionBallsThreaded(gameObjects.size() - (gameObjects.size() % thread_count), gameObjects.size());
    }
   //return positionBallsThreaded(0, gameObjects.size());
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
    const glm::vec2 o2_o1  = obj_1.position - obj_2.position;
    const float dist2 = o2_o1.x * o2_o1.x + o2_o1.y * o2_o1.y;
    if (dist2 < (obj_1.radius + obj_2.radius) * (obj_1.radius + obj_2.radius) && dist2 > eps ) {
        const float dist          = sqrt(dist2);
        // Radius are all equal to 1.0f
        const float delta  = response_coef * 0.5f * (obj_2.radius + obj_1.radius - dist);
        const glm::vec2 col_vec = (o2_o1 / dist) * delta;
        obj_1.position += col_vec;
        obj_2.position -= col_vec;
    }
}


void PhysicsEngine::solveCollisions() {

    const uint32_t  thread_count = threadCount;

    const uint32_t thread_zone_size = ceil((grid.width * grid.height) / thread_count);

    std::vector<std::thread> mythreads;
    for (int i = 0; i < thread_count; i++)
    {
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
    if(grid.width * grid.height % thread_count != 0) {
        solveCollisionThreaded(grid.width * grid.height - (grid.width * grid.height % thread_count), grid.width * grid.height);
    }
//    solveCollisionThreaded(0, grid.width * grid.height);

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


void PhysicsEngine::resolveCollisionsWithBalls(uint32_t gameObjectId, uint32_t otherBallId) {

    constexpr float restitution_coefficient = 0.7f;
    if (gameObjectId == otherBallId) {
        return;
    }
    GameObject& obj1 = gameObjects[gameObjectId];
    GameObject& obj2 = gameObjects[otherBallId];
    glm::vec2 relDist = obj1.position - obj2.position;
    float distance2  =  relDist.x * relDist.x + relDist.y * relDist.y;
    float r1 = obj1.radius;
    float r2 = obj2.radius;
    // Check if the objects are overlapping
    if (distance2 < (r1 + r2)*(r1 + r2)  ) {
        float distance = sqrt(distance2);
        obj1.collided = true;
        obj2.collided = true;
        const float delta  = 1.0f * 0.5f * (obj2.radius+ obj1.radius - distance);
        const glm::vec2 col_vec = (relDist / distance) * delta;
        obj1.position += col_vec;
        obj2.position -= col_vec;

    }
}

