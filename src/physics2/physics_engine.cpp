//
// Created by mario on 1/16/24.
//
#pragma once
#include "collision_grid2.cpp"
#include "../physics/game_object.h"
//#include "engine/common/utils.hpp"
//#include "engine/common/index_vector.hpp"
#include "../thread_pool/thread_pool2.cpp"
#include <glm/glm.hpp>
#include <iostream>

struct PhysicSolver
{
    std::vector<GameObject> objects;
    CollisionGrid          grid;
    glm::ivec2             world_size;
    glm::vec2              gravity = {0.0f, 0.0f};

    // Simulation solving pass count
    uint32_t        sub_steps;
    tp::ThreadPool& thread_pool;

    PhysicSolver(glm::ivec2 size, tp::ThreadPool& tp)
            : grid{size.x, size.y}
            , world_size{-1, 1}
            , sub_steps{8}
            , thread_pool{tp}
    {
        grid.clear();
        objects = std::vector<GameObject>();

    }

    // Checks if two atoms are colliding and if so create a new contact
    void solveContact(uint32_t atom_1_idx, uint32_t atom_2_idx)
    {
//        constexpr float response_coef = 1.0f;
//        constexpr float eps           = 0.0001f;
//        GameObject& obj_1 = objects[atom_1_idx];
//        GameObject& obj_2 = objects[atom_2_idx];
//        const glm::vec2 o2_o1  = obj_1.position - obj_2.position;
//        const float dist2 = o2_o1.x * o2_o1.x + o2_o1.y * o2_o1.y;
//        if (dist2 < 1.0f && dist2 > eps) {
//            const float dist          = sqrt(dist2);
//            // Radius are all equal to 1.0f
//            const float delta  = response_coef * 0.5f * (1.0f - dist);
//            const glm::vec2 col_vec = (o2_o1 / dist) * delta;
//            obj_1.position += col_vec;
//            obj_2.position -= col_vec;
//        }

        float velocityLoss = 1; // Adjust the velocity loss factor as needed
        GameObject& gameObject = objects[atom_1_idx];
        GameObject& otherBall = objects[atom_2_idx];

            if (&gameObject != &otherBall) { // Avoid self-collision
                float dx = gameObject.position.x - otherBall.position.x;
                float dy = gameObject.position.y - otherBall.position.y;
                float distance = std::sqrt(dx * dx + dy * dy);
                float minDistance = gameObject.radius + otherBall.radius;

                if (distance < minDistance) {
                    // Balls are overlapping, resolve the collision

                    // Calculate the unit normal and tangent
                    float normalX = dx / distance;
                    float normalY = dy / distance;

                    // Calculate relative velocity
                    float relativeVelocity = (gameObject.velocity.x - otherBall.velocity.x) * normalX +
                                             (gameObject.velocity.y - otherBall.velocity.y) * normalY;

                    // Resolve collision along the normal
                    if (relativeVelocity < 0) {
                        float impulse = (-(1) * relativeVelocity) /
                                        (1 / gameObject.mass + 1 / otherBall.mass);
                        gameObject.velocity.x += impulse / gameObject.mass * normalX;
                        gameObject.velocity.y += impulse / gameObject.mass * normalY;
                        otherBall.velocity.x -= impulse / otherBall.mass * normalX;
                        otherBall.velocity.y -= impulse / otherBall.mass * normalY;

                        // Apply velocity loss
                        gameObject.velocity.x *= velocityLoss;
                        gameObject.velocity.y *= velocityLoss;
                        otherBall.velocity.x *= velocityLoss;
                        otherBall.velocity.y *= velocityLoss;

                        // Separate the balls using minimum translation vector (MTV)
                        float overlap = minDistance - distance;
                        float moveX = overlap * normalX;
                        float moveY = overlap * normalY;

                        gameObject.position.x += 0.5 * moveX;
                        gameObject.position.y+= 0.5 * moveY;
                        otherBall.position.x -= 0.5 * moveX;
                        otherBall.position.y -= 0.5 * moveY;
                    }

            }
        }
    }

    void checkAtomCellCollisions(uint32_t atom_idx, const CollisionCell& c)
    {
        for (uint32_t i{0}; i < c.objects_count; ++i) {
            solveContact(atom_idx, c.objects[i]);
        }
    }

    void processCell(const CollisionCell& c, uint32_t index)
    {
        for (uint32_t i{0}; i < c.objects_count; ++i) {
            const uint32_t atom_idx = c.objects[i];
            if (index == 96) {
                std::cout << "atom index: " << atom_idx << std::endl;
            }
            std::cout << "index index: " << index << std::endl;
            checkAtomCellCollisions(atom_idx, grid.data[index - 1]);
            checkAtomCellCollisions(atom_idx, grid.data[index]);
            checkAtomCellCollisions(atom_idx, grid.data[index + 1]);
            checkAtomCellCollisions(atom_idx, grid.data[index + grid.height - 1]);
            checkAtomCellCollisions(atom_idx, grid.data[index + grid.height    ]);
            checkAtomCellCollisions(atom_idx, grid.data[index + grid.height + 1]);
            checkAtomCellCollisions(atom_idx, grid.data[index - grid.height - 1]);
            checkAtomCellCollisions(atom_idx, grid.data[index - grid.height    ]);
            checkAtomCellCollisions(atom_idx, grid.data[index - grid.height + 1]);
        }
    }

    void solveCollisionThreaded(uint32_t start, uint32_t end)
    {
        for (uint32_t idx{start}; idx < end; ++idx) {
            processCell(grid.data[idx], idx);
        }
    }

    // Find colliding atoms
    void solveCollisions()
    {
        // Multi-thread grid
        const uint32_t thread_count = thread_pool.m_thread_count;
        const uint32_t slice_count  = thread_count * 2;
        const uint32_t slice_size   = (grid.width / slice_count) * grid.height;
        const uint32_t last_cell    = (2 * (thread_count - 1) + 2) * slice_size;
        // Find collisions in two passes to avoid data races

        // First collision pass
        for (uint32_t i{0}; i < thread_count; ++i) {
            thread_pool.addTask([this, i, slice_size]{
                uint32_t const start{2 * i * slice_size};
                uint32_t const end  {start + slice_size};
                solveCollisionThreaded(start, end);
            });
        }
        // Eventually process rest if the world is not divisible by the thread count
        if (last_cell < grid.data.size()) {
            thread_pool.addTask([this, last_cell]{
                solveCollisionThreaded(last_cell, static_cast<uint32_t>(grid.data.size()));
            });
        }
        thread_pool.waitForCompletion();
        // Second collision pass
        for (uint32_t i{0}; i < thread_count; ++i) {
            thread_pool.addTask([this, i, slice_size]{
                uint32_t const start{(2 * i + 1) * slice_size};
                uint32_t const end  {start + slice_size};
                solveCollisionThreaded(start, end);
            });
        }
        thread_pool.waitForCompletion();
    }

    // Add a new object to the solver
    void addObject(const GameObject& object)
    {
         objects.push_back(object);
    }

//    // Add a new object to the solver
//    void createObject(glm::vec2 pos)
//    {
//        objects.emplace_back(pos);
//    }

    std::vector<GameObject> & getGameObjects(){
        return objects;
    }

    CollisionGrid getGrid(){
        return grid;
    }

    void clearGrid(){
        grid.clear();
    }

    void update(float dt)
    {
        // Perform the sub steps
        const float sub_dt = dt / static_cast<float>(sub_steps);
        for (uint32_t i(sub_steps); i--;) {
            addObjectsToGrid();
            solveCollisions();
            updateObjects_multi(sub_dt);
        }
    }

    glm::vec2 mapToWorldToGrid(const glm::vec2 worldCoord, const glm::ivec2 gridSize) {
    // Assuming world coordinates range from -1 to 1
    float normalizedX = (worldCoord.x + 1.0f) / 2.0f;
    float normalizedY = (worldCoord.y + 1.0f) / 2.0f;

    // Mapping normalized coordinates to grid coordinates
    int gridX = static_cast<int>(normalizedX * gridSize.x);
    int gridY = static_cast<int>(normalizedY * gridSize.y);

    // Make sure the grid coordinates are within bounds
    gridX = std::max(0, std::min(gridX, gridSize.x - 1));
    gridY = std::max(0, std::min(gridY, gridSize.y - 1));

    // check if the grid coordinates are within bounds and not on the border
//    if (gridX == 0 || gridX == gridSize.x - 1 || gridY == 0 || gridY == gridSize.y - 1) {
//        //std::cout << "Out of bounds" << std::endl;
//    }

    return {static_cast<float>(gridX), static_cast<float>(gridY)};
}

    void addObjectsToGrid()
    {
        grid.clear();
        // Safety border to avoid adding object outside the grid
        uint32_t i{0};
        for ( GameObject& obj : objects) {
            auto gridPosition = mapToWorldToGrid(obj.position, {grid.width, grid.height});
            if (gridPosition.x * 10 + gridPosition.y == 96){
                std::cout << "atom index: " << i << std::endl;
            }
            // make sure gridPosition is not on edge
            if (gridPosition.x == 0 || gridPosition.x == grid.width -1 || gridPosition.y == 0 || gridPosition.y == grid.height -1) {
                std::cout << "Out of bounds" << std::endl;
            }
            else{
                grid.addAtom(static_cast<int32_t>(gridPosition.x), static_cast<int32_t>(gridPosition.y), i);
                ++i;
                obj.gridIndex = gridPosition.x * grid.height + gridPosition.y;
            }

        }
    }

    void updateObjects_multi(float dt)
    {
        thread_pool.dispatch(static_cast<uint32_t>(objects.size()), [&](uint32_t start, uint32_t end){
            for (uint32_t i{start}; i < end; ++i) {
                GameObject& obj = objects[i];
                // Add gravity
//                obj.acceleration += gravity;
                // Apply Verlet integration
                obj.update(dt);
                // Apply map borders collisions
                const float margin = 2.0f / grid.width;
                if (obj.position.x > 1.0f - margin) {
                    obj.position.x = 1.0f - margin;
                    obj.velocity.x = -obj.velocity.x;
                }
                if (obj.position.x < -1.0f + margin) {
                    obj.position.x = -1.0f + margin;
                    obj.velocity.x = -obj.velocity.x;
                }
                if (obj.position.y > 1.0f - margin) {
                    obj.position.y = 1.0f - margin;
                    obj.velocity.y = -obj.velocity.y;
                }
                if (obj.position.y < -1.0f + margin) {
                    obj.position.y = -1.0f + margin;
                    obj.velocity.y = -obj.velocity.y;
                }

            }
        });
    }
};
