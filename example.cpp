#include "lasloader.h"

int main()  {
    auto points = gsl::LASLoader::readLAS("testData.las");

    for (auto point : points)
    {
        std::cout << "Point: (" << point.x << ", " << point.y << ", " << point.z << ")" << std::endl;
    }

    return 0;
}
