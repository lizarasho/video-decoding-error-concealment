//
// Created by Nika on 30.11.2020.
//

#include "LoseBits.h"
#include "../info/NALinfo.h"
#include <list>
#include <vector>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

int main(int argc, char *argv[]) {

    ifstream bitstreamFile(argv[1], ifstream::in | ifstream::binary);
    if (!bitstreamFile) {
        fprintf(stderr, "\nfailed to open bitstream file `%s' for reading\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    ifstream infoFile(argv[2], ifstream::in );
    if (!infoFile) {
        fprintf(stderr, "\nfailed to open bitstream file `%s' for reading\n", argv[2]);
        exit(EXIT_FAILURE);
    }
    ofstream outFile(argv[3], std::ios::out | std::ios::binary );

    string line;
    if (outFile.is_open()) {
        size_t index = 0;
        while (getline(infoFile, line)) {
            if (line.empty()){
                break;
            }
            istringstream iss(line);
            uint32_t start, lId, tId, size, type;
            iss >> start >> lId >> tId >> size >> type;
            cout << index << " " << start << " " << size << "\n";
            while(index != start) {
                index++;
                bitstreamFile.get();
            }
            char * buffer;
            buffer = new char [size];
            index += size;
            bitstreamFile.read(buffer, size);
            outFile.write(buffer, size);
            delete[] buffer;
        }
    } else {
        cout << "Can't write into file";
    }
    outFile.close();
}