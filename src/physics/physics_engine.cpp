#include <valarray>
#include <iostream>
#include <glm/vec2.hpp>
#include "physics_engine.h"

// #define THREADED
void PhysicsEngine::update_objects(float dt) {
#ifdef THREADED
        threadPool.dispatch(gameObjects.size(), [&](uint32_t start, uint32_t end) {
            for (uint32_t i = start; i < end; i++) {
                GameObject &gameObject = gameObjects[i];
                gameObject.x += gameObject.vx;
                gameObject.y += gameObject.vy;
                resolveCollisionsWithWalls(gameObject);


                gameObject.vy += gravity;
            }
        });

#else
    for (auto &gameObject: gameObjects) {
//         gameObject.x += gameObject.vx;
//            gameObject.y += gameObject.vy;
        gameObject.acceleration += gravity;
        gameObject.update(dt);
        resolveCollisionsWithWalls(gameObject);

    }

#endif
}

void PhysicsEngine::update(float dt) {


    positionBallsInGrid();
    solveCollisions();
    solveCollisions();

    update_objects(dt);


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


void PhysicsEngine::positionBallsThreaded(int start, int end){

//    uint32_t i{0};

    for(int i = start; i < end; i++) {
        GameObject& gameObject = gameObjects[i];
        glm::vec2 gridCoord = mapToWorldToGrid(gameObject.position, getGridSize());
        int gridIndex = grid.addObject(gridCoord.x, gridCoord.y, i);
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
}

void PhysicsEngine::checkAtomCellCollisions(uint32_t atom_idx, const CollisionCell& c)
{
    for (uint32_t i{0}; i < c.objects_count; ++i) {
        //resolveCollisionsWithBalls(atom_idx, c.objects[i]);
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
    constexpr float eps           = 0.000001f;
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


//        for (uint32_t i{0}; i < thread_count; ++i) {
//            std::cout << "Thread with id " << std::this_thread::get_id() << " is adding one task " << std::endl;
//            threadPool.addTask([this, i, thread_zone_size] {
//                uint32_t const start = i * thread_zone_size;
//                uint32_t const end = start + thread_zone_size;
//                solveCollisionThreaded(start, end);
//            });
//        }
//
//        uint32_t lastCell = thread_count * thread_zone_size;
//        if (lastCell < grid.gridData.size()) {
//            std::cout << "Thread with id " << std::this_thread::get_id() << " is adding 1  tasks " << std::endl;
//
//            threadPool.addTask([this, lastCell] {
//                solveCollisionThreaded(lastCell, grid.gridData.size());
//            });
//        }
//
//        threadPool.waitForCompletion();
#ifdef THREADED

const uint32_t  thread_count = 2;

        const uint32_t thread_zone_size = ceil((grid.width * grid.height) / thread_count);  // 36 / 2 = 16
        // Find collisions in two passes to avoid data races

        // First collision pass
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
#else
    const uint32_t  thread_count = threadCount;

    const uint32_t thread_zone_size = ceil((grid.width * grid.height) / thread_count);  // 36 / 2 = 16
    // Find collisions in two passes to avoid data races

    // First collision pass
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
//    solveCollisionThreaded(0, grid.width * grid.height);
//        const uint32_t thread_count =
//                (int) threadPool.size <= grid.height * grid.width ? threadPool.size : grid.height * grid.width; // 2

#endif

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
//    GameObject obj1 = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.f};
//    GameObject obj2 = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.f};
//    float dx = obj2.position.x - obj1.x;
//    float dy = obj2.y - obj1.y;
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
//        obj1.x -= col_vec.x;
//        obj1.y -= col_vec.y;
//        obj2.x += col_vec.x;
//        obj2.y += col_vec.y;
        obj1.position += col_vec;
        obj2.position -= col_vec;
//        glm::vec2 normal = glm::normalize(relDist);
//        glm::vec2 relVel = obj2.velocity - obj1.velocity;
        // Calculate the relative velocity along the normal vector
//        float velAlongNormal = glm::dot(relVel, normal);
//
//        // Calculate the impulse (change in velocity) along the normal vector
//        float j = 0.95f * (2.0f * obj1.mass * obj2.mass * velAlongNormal) / (obj1.mass + obj2.mass);
////        obj1.vx = normal.x;
////        obj1.vy = normal.y;
////        obj2.vx = normal.x;
////        obj2.vy = normal.y;
//        obj1.velocity += j * normal / obj1.mass;
//        obj2.velocity -= j * normal / obj2.mass;
        //obj2.velocity *= normal;
        // displace balls
//        float overlap = 1.2f * 0.5f * (distance - obj1.radius - obj2.radius);
//        obj1.x += overlap * dx / distance;
//        obj1.y += overlap * dy / distance;
//        obj2.x -= overlap * dx / distance;
//        obj2.y -= overlap * dy / distance;
//
//        // Calculate collision normal
//        float nx = dx / distance;
//        float ny = dy / distance;
//
//        // Calculate relative velocity
//        float relative_vx = obj2.vx - obj1.vx;
//        float relative_vy = obj2.vy - obj1.vy;
//        float relative_speed = relative_vx * nx + relative_vy * ny;
//
//        // Check if objects are moving towards each other
//        if (relative_speed < 0) {
//            // Calculate impulse
//            float impulse = (1 + restitution_coefficient) * relative_speed /
//                            (1 / obj1.mass + 1 / obj2.mass);
//
//            // Update velocities
//            obj1.vx += impulse * nx / obj1.mass;
//            obj1.vy += impulse * ny / obj1.mass;
//            obj2.vx -= impulse * nx / obj2.mass;
//            obj2.vy -= impulse * ny / obj2.mass;
//        }
    }
}
