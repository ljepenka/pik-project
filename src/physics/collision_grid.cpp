#include <cstdint>
#include <thread>
#include <cmath>
#include <vector>


class GridCell {
public:
    static constexpr uint32_t cell_capacity = 8;

    uint32_t ball_counter = 0;
    uint32_t balls[cell_capacity] = {};
    float maxRadiusRatio = 0.0f;

    GridCell() = default;

    void addBall(uint32_t objectId, float radiusRatio = 0.0f) {
        balls[ball_counter] = objectId;
        ball_counter += ball_counter < cell_capacity;
        maxRadiusRatio = radiusRatio > maxRadiusRatio ? radiusRatio : maxRadiusRatio;
    }
};

class Grid{
public:

    int32_t height, width;
    std::vector<GridCell> gridCells;

    Grid(int32_t width_, int32_t height_): height(height_), width(width_) {
        gridCells.resize(width * height);
    }

    void resize(int32_t width_, int32_t height_) {
        height = height_;
        width = width_;
        gridCells.resize(width * height);
    }
    int addBall(uint32_t x, uint32_t y, uint32_t object, float radiusRatio) {

        gridCells[x * height + y].addBall(object, radiusRatio);
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
                mythreads.emplace_back(&Grid::clearGridThreaded, this, start, end);
            }
            auto originalthread = mythreads.begin();

            while (originalthread != mythreads.end()) {
                originalthread->join();
                originalthread++;
            }
            if (width * height % thread_count != 0) {
                clearGridThreaded(width * height - (width * height % thread_count),
                                  width * height);
            }
        }
        else{
            clearGridThreaded(0, width * height);

        }
    }

    void clearGridThreaded(int start, int end){
        for(int i = start; i < end; ++i) {
            gridCells[i].ball_counter = 0;
        }
    }
};