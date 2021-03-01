#include "dataloader.h"

#include <fstream>
#include <clocale>
#include <stdexcept>

namespace DataLoader {

std::string readHeader(std::fstream &stream);
Point readPoint(const std::string &line);
std::vector<Point> readPoints(std::fstream &stream, std::string &error);

FileData loadMeasurementData(const std::string &fileName)
{
    FileData fileData;

    std::fstream in(fileName);
    if ( !in ) {
        fileData.error = "Can't open file: " + fileName;
        return fileData;
    }

    fileData.header = readHeader(in);
    fileData.points = readPoints(in, fileData.error);

    return fileData;
}

std::string readHeader(std::fstream &stream)
{
    std::string out, line;
    while (stream.peek() == '#') {
        if (std::getline(stream, line)) {
            out += line.substr(2);
            out += "\n";
        }
    }

    return out;
}

std::vector<Point> readPoints(std::fstream &stream, std::string &error)
{
    std::string line;
    std::vector<Point> vec;
    Point p;

    while (std::getline(stream, line)) {
        if (line.back() == '\r')    // linux
            line.resize(line.size()-1);
        if ( line.empty() )
            continue;

        try {
            p = readPoint(line);
            vec.push_back(p);
        }
        catch (std::invalid_argument &e) {
            error += "invalid_argument: " + line + " (" + e.what() + ")\n";
        }
        catch (std::out_of_range &e) {
            error += "out_of_range: " + line + " (" + e.what() + ")\n";
        }
        catch (std::length_error &e) {
            error += "std::vector length_error: " + line + " (" + e.what() + ")\n";
            break;
        }
    }

    return vec;
}

// std::from_chars поддержака добавлена недавно: https://gcc.gnu.org/pipermail/gcc-patches/2020-July/550331.html
Point readPoint(const std::string &line)
{
    Point p;
    std::string::size_type pos;

    const auto oldLocale=std::setlocale(LC_NUMERIC,nullptr);
    std::setlocale(LC_NUMERIC,"C");

    if ( (pos = line.find(" ")) != std::string::npos ) {
        p.timestamp = std::stod(line.substr(0, pos));
        p.value = std::stod(line.substr(pos+1));
    } else {
        throw std::invalid_argument("wrong format");
    }

    std::setlocale(LC_NUMERIC,oldLocale);

    return p;
}

}
