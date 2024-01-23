#include<vector>
#include <cstdint>

template <typename T>
class Grid {
public:
    int32_t height, width;
    std::vector<T> gridData;

    Grid(int32_t width_, int32_t height_): height(height_), width(width_) {
        gridData.resize(width * height);
    }

    void resize(int32_t width_, int32_t height_) {
        height = height_;
        width = width_;
        gridData.resize(width * height);
    }
};