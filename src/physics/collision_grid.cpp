#include "../common/grid.cpp"
#include <cstdint>
#include <thread>
#include <cmath>


class CollisionCell {
public:
    static constexpr uint32_t cell_capacity = 6;

    uint32_t objects_count = 0;
    uint32_t objects[cell_capacity] = {};
    float maxRadiusRatio = 0.0f;

    CollisionCell() = default;

    void addObject(uint32_t objectId, float radiusRatio = 0.0f) {
        objects[objects_count] = objectId;
        objects_count += objects_count < cell_capacity - 1;
        maxRadiusRatio = radiusRatio > maxRadiusRatio ? radiusRatio : maxRadiusRatio;
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


    int addObject(uint32_t x, uint32_t y, uint32_t object, float radiusRatio) {

        gridData[x * height + y].addObject(object, radiusRatio);
        return x * height + y;
    }

    void clearGrid(int threadCount) {
        if(threadCount > 1){
            const uint32_t thread_count = threadCount;

            const uint32_t thread_zone_size = ceil((width * height) / thread_count);

            std::vector<std::thread> mythreads;
            for (int i = 0; i < thread_count; i++) {
                uint32_t const start = i * thread_zone_size;
                uint32_t const end = start + thread_zone_size;
                mythreads.emplace_back(&CollisionGrid::clearGridThreded, this, start, end);
            }
            auto originalthread = mythreads.begin();
            //Do other stuff here.
            while (originalthread != mythreads.end()) {
                originalthread->join();
                originalthread++;
            }
            if (width * height % thread_count != 0) {
                clearGridThreded(width * height - (width * height % thread_count),
                                       width * height);
            }
        }
        else{
            clearGridThreded(0, width * height);

        }
    }

    void clearGridThreded(int start, int end){
        for(int i = start; i < end; ++i) {
            gridData[i].objects_count = 0;
        }
    }
};