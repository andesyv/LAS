#include "lasloader.h"

int main()  {
    gsl::LASLoader lasloader{"Complete\\Path\\To\\File.las"};

    // Iterate through all points:
    for (auto point : lasloader)
    {
        std::cout << "Point: (" << point.x << ", " << point.y << ", " << point.z << ")" << std::endl;
    }

    // Iterate in certain range:
    for (auto point = lasloader.begin() + 10; point != lasloader.end() - 25; ++point)
    {
        std::cout << "Point: (" << point->x << ", " << point->y << ", " << point->z << ")" << std::endl;
    }

    // Iterate in certain range with normalized output:
    for (auto point = lasloader.begin() + 10; point != lasloader.end() - 25; ++point)
    {
        std::cout << "Point: (" << point->xNorm() << ", " << point->yNorm() << ", " << point->zNorm() << ")" << std::endl;
    }

    return 0;
}
