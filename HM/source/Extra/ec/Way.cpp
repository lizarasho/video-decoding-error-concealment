//
// Created by Nika on 12.05.2021.
//

#include <sstream>
#include <cmath>
#include <algorithm>
#include "Way.h"
std::vector<std::string> getNextLineAndSplitIntoTokens(std::istream& str)
{
    std::vector<std::string>   result;
    std::string                line;
    std::getline(str,line);

    std::stringstream          lineStream(line);
    std::string                cell;

    while(std::getline(lineStream,cell, ','))
    {
        result.push_back(cell);
    }
    // This checks for a trailing comma with no data after it.
    if (!lineStream && cell.empty())
    {
        // If there was a trailing comma then add an empty element.
        result.push_back("");
    }
    return result;
}


Way::Way(std::istream &str): stream(str){
    double X = 0;
    double Y = 0;
    std::vector<std::string> line = getNextLineAndSplitIntoTokens(stream);
    double prevX = stod(line[1]);
    double prevY = stod(line[2]);
    way.push_back(std::pair<int, std::pair<double, double>>(0, std::pair<double, double>(X,Y)));
    int cnt = 0;
    while (!stream.eof()) {
        cnt++;
        line = getNextLineAndSplitIntoTokens(stream);
        if (line.size() > 2) {
            double F1 = prevX;
            double F2 = stod(line[1]);
            double L1 = prevY;
            double L2 = stod(line[2]);

            double R=6.3781*(10e6);
            double dx = (F2-F1)*R;
            double dy = (L2-L1)*R*cos(F1);
            X = X - dy;
            Y = Y - dx;
            prevX  = F2;
            prevY = L2;
            way.push_back(std::pair<int, std::pair<double, double>>(cnt, std::pair<double, double>(X,Y)));
        }
    }
}
double distance(double x1, double y1, double x2, double y2)
{
    // Calculating distance
    return sqrt(pow(x2 - x1, 2) +
                pow(y2 - y1, 2) * 1.0);
}
std::vector<std::pair<int, std::pair<double, double>>> Way::srcList(int frame) {
    std::vector<std::pair<int, std::pair<double, double>>> res;
    std::pair<int, std::pair<double, double>> crnt = way[0];
    for (int i = 0; i < way.size(); ++i) {
        if (way[i].first < frame) {
            res.push_back(way[i]);
        }
        if (way[i].first == frame) {
            crnt = way[i];
        }
    }
    std::sort( res.begin( ), res.end( ), [&crnt]( const std::pair<int, std::pair<double, double>>& lhs, const std::pair<int, std::pair<double, double>>& rhs )
    {
        return distance(crnt.second.first, crnt.second.second,lhs.second.first, lhs.second.second ) <
                distance(crnt.second.first, crnt.second.second,rhs.second.first, rhs.second.second );
    });
    return res;
}
