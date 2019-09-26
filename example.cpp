#include "lasloader.hpp"

int main()  {
    auto points = LASLoader::readLAS("372\\data\\testDataLAS.las");

    for (auto point : points)
    {
        std::cout << "Point: (" << point.x << ", " << point.y << ", " << point.z << ")" << std::endl;
    }

    return 0;
}
