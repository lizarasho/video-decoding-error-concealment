//
// Created by Nika on 12.05.2021.
//

#ifndef HM_WAY_H
#define HM_WAY_H


#include <istream>
#include <vector>

class Way {
    std::istream& stream;
    std::vector<std::pair<int, std::pair<double,double>>> way;
public:
    std::vector<std::pair<int, std::pair<double,double>>> srcList(int i);
    Way(std::istream& str);
};


#endif //HM_WAY_H
