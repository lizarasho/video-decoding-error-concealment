//
// Created by Nika on 04.11.2020.
//


#include <list>
#include <vector>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <fstream>
#include <iostream>

#include "NALinfo.h"

using namespace std;

const string info_header = "num  start lId tId size type";

NALPack read_nal_header(ifstream &bitstreamFile) {
    int32_t strem = (bitstreamFile.get() << 8) + bitstreamFile.get();
    assert(strem >> 15 == 0);
    return {(strem >> 9), (strem >> 3) & 63, (strem & 7) - 1};
}

uint32_t readFirst3(ifstream &bitstreamFile) {
    uint32_t bytes = 0;
    bytes += bitstreamFile.get() << 16;
    bytes += bitstreamFile.get() << 8;
    bytes += bitstreamFile.get();
    return bytes;
}

void writeToFile(vector<NALPack> &packs, ofstream &out, bool read_format) {
    if (read_format) {
        out << info_header << "\n";
    }
    cout << info_header << "\n";
    for (auto pack: packs) {
        out << pack.info(read_format);
        cout << pack.info(true);
    }
}

int main(int argc, char *argv[]) {
    ifstream bitstreamFile(argv[1], ifstream::in | ifstream::binary);
    if (!bitstreamFile) {
        fprintf(stderr, "\nfailed to open bitstream file `%s' for reading\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    uint32_t bytesStream = readFirst3(bitstreamFile);
    vector<NALPack> allPacks;

    int cnt = 0;
    size_t sizeOfPack = 3;
    size_t position = 3;
    while (!!bitstreamFile) {
        if (bytesStream == 0x00000001) {
            if (!allPacks.empty()) {
                allPacks.back().size = sizeOfPack - 4;
            }
//cout << "4 byte prefix" << "\n";
            ++cnt;
            allPacks.push_back(read_nal_header(bitstreamFile));
            allPacks.back().number = allPacks.size();
            allPacks.back().start = position - 4;

            sizeOfPack = 6;
            position += 2;
        } else if ((bytesStream & 0x00FFFFFF) == 0x00000001) {
            //cout << "3 byte prefix" << "\n";
            if (!allPacks.empty()) {
                allPacks.back().size = sizeOfPack - 3;
            }
            ++cnt;
            allPacks.push_back(read_nal_header(bitstreamFile));
            allPacks.back().number = allPacks.size();
            allPacks.back().start = position - 3;
            sizeOfPack = 5;
            position += 2;
        }

        bytesStream = (bytesStream << 8) + bitstreamFile.get();
        sizeOfPack++;
        position++;
    }
    if (!allPacks.empty()) {
        allPacks.back().size = sizeOfPack - 1;
    }
    cout << "Total number of packs: " << cnt << "\n";

    ofstream out(argv[2]);
    bool format = (argv[3] && strcmp(argv[3], "-r") == 0);
    if (out.is_open()) {
        writeToFile(allPacks, out, format);
        out.close();
    } else {
        cout << "Can't write into file";
    }
}


string NALPack::info(bool read_format) {
    char buffer[50];
    int n;
    if (read_format) {
        n = sprintf(buffer, "%3u %6u %3u %3u %5u %3u", number, start, layerId, temporalId, size, unitTypeNum) ;
        string type = typeToString(unitType);
        return string(buffer, n) + " " +  type + "\n";
    } else {
        n = sprintf(buffer, "%u %u %u %u %u", start, layerId, temporalId, size, unitTypeNum);
        return string(buffer, n) + "\n";
    }

}
