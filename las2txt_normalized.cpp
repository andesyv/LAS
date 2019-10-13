#include <fstream>
#include "lasloader.h"

int main(int argc, char *argv[])
{
    for (int i{1}; i < argc; ++i)
    {
        std::string filename{argv[i]};

        gsl::LASLoader loader{filename};

        if (loader)
        {
            std::ofstream ouf{filename.substr(0, filename.find_last_of('.')).append(".txt"), ouf.out | ouf.trunc};
            if (ouf)
            {
                ouf << loader.pointCount() << std::endl;
                for (auto it = loader.begin(); it != loader.end(); ++it)
                {
                    ouf << it->xNorm() << ' ' << it->yNorm() << ' ' << it->zNorm() << std::endl;
                }
            }
        }
    }
}
