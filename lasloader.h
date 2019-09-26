namespace gsl
{
struct vec3
{
    float x;
    float y;
    float z;
};
}

#include <vector>
#include <fstream>
#include <iostream>
#include <ctime>

unsigned short getCurrentYear()
{
    auto t = std::time(nullptr);
    return std::localtime(&t)->tm_year + 1900;
}

/** LAS file loader
 * Reads an uncompressed LAS file and
 * extracts the points.
 * @authors andesyv, KriAB, Gammelsaeterg, Lillebenn
 */

//TODO update so both 1.2 and 1.4 will work
// NB: For 32 bit compilator
// 64 bit must use other types that are equal in bytesize.
class LASLoader
{
private:
    /** Standard type sizes:
     * char = 1
     * short = 2
     * int = 4
     * long = 4
     * long long = 8
     * float = 4
     * double = 8
     */

    struct HEADERDATATYPES
    {
        char filesignature[4]; //Sier filtypen og må være LASF for LAS
        unsigned short fileSourceID;
        unsigned short globalEncoding;
        unsigned long projectIDGuidData1; // Optional
        unsigned short projectIDGuidData2; // Optional
        unsigned short projectIDGuidData3; // Optional
        unsigned char projectIDGuidData4[8]; // Optional
        unsigned char versionMajor;
        unsigned char versionMinor;
        char systemIdentifier[32];
        char generatingSoftware[32];
        // 90 bytes thus far
        unsigned short fileCreationDayOfYear; // Optional
        unsigned short fileCreationYear; // Optional
        // 4 more optional bytes here
        unsigned short headerSize;
        unsigned long byteOffsetToPointData;
        unsigned long numberOfVarableLengthRecords;
        unsigned char pointDataRecordFormat;
        unsigned short pointDataRecordLength;
        unsigned long legacyNumberOfPointRecords;
        unsigned long legacyNumberOfPointsByReturns[5];
        // 37 more bytes
        double xScaleFactor;
        double yScaleFactor;
        double zScaleFactor;
        double xbyteOffset;
        double ybyteOffset;
        double zbyteOffset;
        double maxX;
        double minX;
        double maxY;
        double minY;
        double maxZ;
        double minZ;
        // 96 more bytes
        // For 1.3 format
        unsigned long long startOfWaveformDataPacketRecord;
        // 8 bytes
        // For 1.4 format
        unsigned long long startOfExtendVariableLength;
        unsigned long numberOfExtendedVariableLengthRecords;
        unsigned long long numberOfPointRecords;
        unsigned long long numberOfPointsByReturn[15];
        // 140 more bytes
    } header;

    struct VARIABLELENGTHHEADERDATA
    {
        unsigned short reserved;
        char UserID[16];
        unsigned short RecordID;
        unsigned short RecordLengthAfterHeader;
        char Description[32];
    };

    class PointDataRecordData
    {
    public:
        PointDataRecordData(unsigned char format) : mFormat{format} {}

        long X;
        long Y;
        long Z;
        unsigned short intensity; //Not required
        unsigned char ReturnNumber : 3;
        unsigned char NumberOfReturns : 3;
        unsigned char ScanDirectionFlag : 1;
        unsigned char EdgeOfFlightLine : 1;
        unsigned char classification;
        char scanAngleRank; //(-90 to + 90) - Left side
        unsigned char UserData; //Not required
        unsigned short pointSourceID;

        double& GPSTime() { return _GPSTime; }
        unsigned short& Red() { return (mFormat == 2) ? *((unsigned short*)(&_GPSTime)) : _Red; }
        unsigned short& Green() { return (mFormat == 2) ? *((unsigned short*)(&_GPSTime + sizeof(short))) : _Green; }
        unsigned short& Blue() { return (mFormat == 2) ? *((unsigned short*)(&_GPSTime + 2 * sizeof(short))) : _Blue; }

        // Other functions
        unsigned short getFormat() const { return static_cast<unsigned short>(mFormat); }
        unsigned short getFormatSize() const
        {
            switch (mFormat)
            {
                case 0:
                return 20;
                case 1:
                return 28;
                case 2:
                return 26;
                case 3:
                return 34;
                default:
                return 0;
            }
        }

    private:
        // Depending on format
        double _GPSTime;
        unsigned short _Red;
        unsigned short _Green;
        unsigned short _Blue;

        unsigned char mFormat;
    };

    enum GLOBALENCODINGFLAG
    {
        GPSTimeType = 1,
        WaveformDataPacketsInternal = 2,
        WaveformDataPacketsExternal = 4,
        ReturnNumbersHaveBeenSyntheticallyGenerated = 8,
        WKT = 16
    };

private:
    bool usingCreationDay = false;
    bool usingCreationYear = false;

public:
    LASLoader()
    {

    }


    // File with full path
    static std::vector<gsl::vec3> readLAS(const std::string& file)
    {
        std::ifstream fstrm;
        LASLoader loader{};
        std::vector <gsl::vec3> vertices;

        fstrm.open(file.c_str(), std::ifstream::in | std::ifstream::binary);
        if (fstrm.is_open())
        {
            std::cout << "LAS file read: " << file << std::endl;
            // Read in first 90 bytes of header
            fstrm.read((char *) &loader.header, 90);

            // Attempt to read in creationday and creationyear
            int readerCheckpoint = fstrm.tellg();
            unsigned short creationDayYear[2]{1000, 3000};
            fstrm.read((char*) creationDayYear, 2 * sizeof(unsigned short));
            if (creationDayYear[0] < 367)
            {
                if (creationDayYear[0] != 0)
                {
                    loader.header.fileCreationDayOfYear = creationDayYear[0];
                    loader.usingCreationDay = true;
                }
                else
                {
                    loader.usingCreationDay = false;
                }

                if (creationDayYear[1] > 2000 && creationDayYear[1] <= getCurrentYear())
                {
                    loader.header.fileCreationYear = creationDayYear[1];
                    loader.usingCreationYear = true;
                }
                else
                {
                    loader.usingCreationYear = false;
                }
            }
            else
            {
                fstrm.seekg(readerCheckpoint, fstrm.beg);
                loader.usingCreationDay = false;
                loader.usingCreationYear = false;
            }

            // Read 37 more byte
            fstrm.read((char*) &loader.header.headerSize, 37);

            // And now 96 more bytes
            fstrm.read((char*) &loader.header.xScaleFactor, 96);

            // For newer types:
            if (loader.header.versionMajor == 1 && loader.header.versionMinor >= 3)
            {
                fstrm.read((char*) &loader.header.startOfWaveformDataPacketRecord, 8);
            }

            if (loader.header.versionMajor == 1 && loader.header.versionMinor == 4)
            {
                fstrm.read((char*) &loader.header.startOfExtendVariableLength, 140);
            }


            // Done reading header.
            std::cout << "System Identifier: " << loader.header.systemIdentifier << ", Generating Software: "<< loader.header.generatingSoftware <<std::endl;
            std::cout << "LAS version: " << static_cast<int>(loader.header.versionMajor) << "." << static_cast<int>(loader.header.versionMinor) << std::endl;
            // std::cout << "variableLengthRecords:  " << loader.header.numberOfVarableLengthRecords << std::endl;
            if (loader.usingCreationDay)
                std::cout << "Creation day of year: " << loader.header.fileCreationDayOfYear << std::endl;
            if (loader.usingCreationYear)
                std::cout << "Creation year: " << loader.header.fileCreationYear << std::endl;
            std::cout << "Header size: " << loader.header.headerSize << std::endl;


            // Set stream to start of point data position
            fstrm.seekg(loader.header.byteOffsetToPointData, fstrm.beg); // byteOffset from beginning
            unsigned int currentPointSize{0};


            // Read points
            for (unsigned int pointIndex{0}; currentPointSize < loader.header.pointDataRecordLength; ++pointIndex)
            {
                if (loader.header.pointDataRecordFormat > 3)
                {
                    std::cout << "ERROR PointDataFormat not supported!" << std::endl;
                    return {};
                }
                PointDataRecordData point{loader.header.pointDataRecordFormat};
                fstrm.read((char*) &point, point.getFormatSize());

                // Do some configuration on first point
                if (pointIndex == 0)
                {
                    auto pointAmount = loader.header.pointDataRecordLength/point.getFormatSize();
                    std::cout << "Pointdata format size: " << point.getFormatSize() << ", pointdata amount: " << loader.header.pointDataRecordLength/point.getFormatSize() << std::endl;
                    vertices.reserve(pointAmount);
                }
                // std::cout << "Unformatted x: " << point.X << ", y: " << point.Y << ", z: " << point.Z << std::endl;

                gsl::vec3 coord;
                coord.x = (point.X * loader.header.xScaleFactor) + loader.header.xbyteOffset;
                coord.y = (point.Y * loader.header.yScaleFactor) + loader.header.ybyteOffset;
                coord.z = (point.Z * loader.header.zScaleFactor) + loader.header.zbyteOffset;
                vertices.push_back(coord);
                // std::cout << "Formatted point: x: " << coord.x << ", y: " << coord.y << ", z: " << coord.z << std::endl;
            }

            fstrm.close();
        }
        else
        {
            std::cout << "Could not open file for reading: " << file << std::endl;
        }

        return vertices;
    }

};
