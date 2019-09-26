#include "lasloader.h"

int main()  {
    gsl::LASLoader lasloader{"C:\\Users\\andes\\Desktop\\372\\data\\1.las"};
    
    for (auto point = lasloader.end() - 2; point != lasloader.end(); ++point)
    {
        std::cout << "Point: (" << point->x << ", " << point->y << ", " << point->z << ")" << std::endl;
    }

    return 0;
}
