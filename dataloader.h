#ifndef DATALOADER_H
#define DATALOADER_H

#include <string>
#include <vector>

namespace DataLoader {

struct Point {
    Point() = default;
    Point(double t, double v) : timestamp(t), value(v) {}
    double timestamp;
    double value;
};

struct FileData {
    std::string header;
    std::vector<Point> points;
    std::string error;
};

FileData loadMeasurementData(const std::string &fileName);

}

#endif // DATALOADER_H
