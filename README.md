# LAS Reader
 A simple header-only LAS file reader. Reads a LIDAR file with a .las extension and lets you iterate through the point cloud data. It's small and simple but with very limited functionality. Check out this better library for more LIDAR file manipulation: https://github.com/LAStools/LAStools

Just copy `lasloader.h` into your own project and #include it wherever you want to use it.
Example usage (copied from example.cpp):
 ```cpp
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
 ```
