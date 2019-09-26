#ifndef LASLOADER_H
#define LASLOADER_H

#include <vector>
#include <fstream>
#include <iostream>
#include <ctime>

namespace gsl
{

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

// TODO update so both 1.2 and 1.4 will work
// - Add 1.3 and 1.4 formats
// NB: For 32 bit compilator
// 64 bit must use other types that are equal in bytesize.
class LASLoader
{
private:
    /** Standard type sizes for 32 bit compiler:
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
        double maxunformattedX;
        double minunformattedX;
        double maxY;
        double minY;
        double maxunformattedZ;
        double minunformattedZ;
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

        long unformattedX;
        long unformattedY;
        long unformattedZ;
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

        double getX(LASLoader* loader)
        {
            if (loader == nullptr)
                return 0.0;
            return unformattedX * loader->header.xScaleFactor + loader->header.xbyteOffset;
        }
        double getY(LASLoader* loader)
        {
            if (loader == nullptr)
                return 0.0;
            return unformattedY * loader->header.yScaleFactor + loader->header.ybyteOffset;
        }
        double getZ(LASLoader* loader)
        {
            if (loader == nullptr)
                return 0.0;
            return unformattedZ * loader->header.zScaleFactor + loader->header.zbyteOffset;
        }

    private:
        // Depending on format
        double _GPSTime;
        unsigned short _Red;
        unsigned short _Green;
        unsigned short _Blue;

        unsigned char mFormat;

    public:
        double x{};
        double y{};
        double z{};
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
    bool fileOpened = false;
    unsigned int currentPointSize{0};

    std::ifstream fstrm{};

public:
    LASLoader()
    {

    }

    LASLoader(const std::string& file)
    {
        open(file);
    }

    bool open(const std::string& file)
    {
        if (fstrm.is_open())
        {
            fstrm.close();
        }

        fstrm.open(file.c_str(), std::ifstream::in | std::ifstream::binary);
        if (fstrm.is_open())
        {
            std::cout << "LAS file opened: " << file << std::endl;
            // Read in first 90 bytes of header
            fstrm.read((char *) &header, 90);

            // Attempt to read in creationday and creationyear
            int readerCheckpoint = fstrm.tellg();
            unsigned short creationDayYear[2]{1000, 3000};
            fstrm.read((char*) creationDayYear, 2 * sizeof(unsigned short));
            if (creationDayYear[0] < 367)
            {
                if (creationDayYear[0] != 0)
                {
                    header.fileCreationDayOfYear = creationDayYear[0];
                    usingCreationDay = true;
                }
                else
                {
                    usingCreationDay = false;
                }

                if (creationDayYear[1] > 2000 && creationDayYear[1] <= getCurrentYear())
                {
                    header.fileCreationYear = creationDayYear[1];
                    usingCreationYear = true;
                }
                else
                {
                    usingCreationYear = false;
                }
            }
            else
            {
                fstrm.seekg(readerCheckpoint, fstrm.beg);
                usingCreationDay = false;
                usingCreationYear = false;
            }

            // Read 37 more byte
            fstrm.read((char*) &header.headerSize, 37);

            // And now 96 more bytes
            fstrm.read((char*) &header.xScaleFactor, 96);

            // For newer types:
            if (header.versionMajor == 1 && header.versionMinor >= 3)
            {
                fstrm.read((char*) &header.startOfWaveformDataPacketRecord, 8);
            }

            if (header.versionMajor == 1 && header.versionMinor == 4)
            {
                fstrm.read((char*) &header.startOfExtendVariableLength, 140);
            }


            // Done reading header.
            std::cout << "System Identifier: " << header.systemIdentifier << ", Generating Software: "<< header.generatingSoftware <<std::endl;
            std::cout << "LAS version: " << static_cast<int>(header.versionMajor) << "." << static_cast<int>(header.versionMinor) << std::endl;
            // std::cout << "variableLengthRecords:  " << loader.header.numberOfVarableLengthRecords << std::endl;
            if (usingCreationDay)
                std::cout << "Creation day of year: " << header.fileCreationDayOfYear << std::endl;
            if (usingCreationYear)
                std::cout << "Creation year: " << header.fileCreationYear << std::endl;
            std::cout << "Header size: " << header.headerSize << std::endl;


            // Set stream to start of point data position
            fstrm.seekg(header.byteOffsetToPointData, fstrm.beg); // byteOffset from beginning
            currentPointSize = 0;
            fileOpened = true;
        }
        else
        {
            std::cout << "Could not open file for reading: " << file << std::endl;
            fileOpened = false;
        }

        return fileOpened;
    }


    // File with full path
    static std::vector<PointDataRecordData> readLAS(const std::string& file)
    {
        std::ifstream fstrm;
        LASLoader loader{};
        std::vector <PointDataRecordData> points;

        if (loader.open(file))
        {
            // Read points
            for (unsigned int pointIndex{0}; loader.currentPointSize < loader.header.pointDataRecordLength; ++pointIndex)
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
                    points.reserve(pointAmount);
                }
                // std::cout << "Unformatted x: " << point.unformattedX << ", y: " << point.unformattedY << ", z: " << point.unformattedZ << std::endl;

                point.x = point.getX(&loader);
                point.y = point.getY(&loader);
                point.z = point.getZ(&loader);

                points.push_back(point);
                // std::cout << "Formatted point: x: " << coord.x << ", y: " << coord.y << ", z: " << coord.z << std::endl;
            }

            fstrm.close();
        }
        else
        {

        }

        return points;
    }

};

}

#endif // LASLOADER_H
