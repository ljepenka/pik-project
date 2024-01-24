#include "../common/grid.cpp"
#include <cstdint>


class CollisionCell {
public:
    static constexpr uint32_t cell_capacity = 6;

    uint32_t objects_count = 0;
    uint32_t objects[cell_capacity] = {};

    CollisionCell() = default;

    void addObject(uint32_t objectId) {
        objects[objects_count] = objectId;
        objects_count += objects_count < cell_capacity - 1;
    }

    void clearGrid() {
        objects_count = 0;
    }

    void removeObject(uint32_t id) {
        for(uint32_t i{0}; i < objects_count; ++i) {
            if(objects[i] == id) {
                objects[i] = objects[objects_count - 1];
                --objects_count;
                return;
            }
        }
    }
};

class CollisionGrid: public Grid<CollisionCell>{
public:

    CollisionGrid(int width, int height) : Grid<CollisionCell>(width, height) {}


    int addObject(uint32_t x, uint32_t y, uint32_t object) {

        gridData[x * height + y].addObject(object);
        return x * height + y;
    }

    void clearGrid() {
        for(auto& cell : gridData) {
            cell.objects_count = 0;
        }
    }
};