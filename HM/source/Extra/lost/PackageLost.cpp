//
// Created by Nika on 29.11.2020.
//

#include "PackageLost.h"
#include <fstream>
#include <sstream>
#include <string>
#include "../NalPack/NalPack.h"
#include <ctime> // для функции time()


using namespace std;
bool check;
int main(int argc, char *argv[]) {
    ifstream infoFile(argv[1], ifstream::in );
    string line;
    ofstream outFile(argv[2]);
    srand(time(0));
    bool skipped = false;
    double part =  atof( argv[3]);
    if (outFile.is_open()) {
        while (getline(infoFile, line)) {
            double r = (double) rand() / (RAND_MAX);
            istringstream iss(line);
            uint32_t start, lId, tId, size, type;
            iss >> start >> lId >> tId >> size >> type;
            NalUnitType nalType = typeFromInt(type);

            if (isSkippable(nalType)){
                skipped = true;
            }
            if (r >= part || (!isSkippable(nalType) && !skipped)) {
                outFile << line << endl;
            }
        }
    }

}