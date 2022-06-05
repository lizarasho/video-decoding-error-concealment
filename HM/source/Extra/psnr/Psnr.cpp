//
// Created by Nika on 18.05.2021.
//

#include <fstream>
#include <stdio.h>
#include <math.h>
#include <vector>
#include "Psnr.h"
#include <filesystem>
#include <iostream>
#include <numeric>
#include <sstream>

using namespace std;
double psnr(char* file1, string file2, int height, int width) {
    int frameLength = width * height * 3 / 2;
    std::ifstream f1(file1, ios::in | ios::binary );
    std::ifstream f2(file2.c_str(), ios::in | ios::binary );
    f1.seekg(0, f1.end);
    unsigned long len1 = (unsigned long)f1.tellg();
    f1.seekg(0, f1.beg);
    f2.seekg(0, f2.end);
    unsigned long len2 = (unsigned long)f2.tellg();
    f2.seekg(0, f2.beg);

    unsigned long len = min(len1, len2);
    int frames = len / frameLength;

    char* b1 = new char[frameLength];
    char* b2 = new char[frameLength];

    double mse = 0;
    for (int i = 0; i < frames; ++i) {
        f1.read(b1, frameLength);
        f2.read(b2, frameLength);
        for (int j = 0; j < frameLength; ++j) {
            //cout << (unsigned int)b1[j] << " " << (unsigned int)b2[j] << "\n";
            mse += (((unsigned char)b1[j] - (unsigned char)b2[j]) * ((unsigned char)b1[j] - (unsigned char)b2[j]));
        }
    }
    mse = mse / (frameLength * frames);
    double psnr = 10 * log10(255 * 255/mse);
    delete[](b1);
    delete[](b2);
    return psnr;

}
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
void printWay(std::istream &stream) {
    ofstream myfile;
    myfile.open ("way.txt");

    double X = 0;
    double Y = 0;
    std::vector<std::string> line = getNextLineAndSplitIntoTokens(stream);
    double prevX = stod(line[1]);
    double prevY = stod(line[2]);
    myfile << X << " " << Y << "\n";
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
            myfile << X << " " << Y << "\n";
        }
    }

}
size_t split(const std::string &txt, std::vector<std::string> &strs, char ch)
{
    size_t pos = txt.find( ch );
    size_t initialPos = 0;
    strs.clear();

    // Decompose statement
    while( pos != std::string::npos ) {
        strs.push_back( txt.substr( initialPos, pos - initialPos ) );
        initialPos = pos + 1;

        pos = txt.find( ch, initialPos );
    }

    // Add the last one
    strs.push_back( txt.substr( initialPos, std::min( pos, txt.size() ) - initialPos + 1 ) );

    return strs.size();
}
std::string getLastLine(std::ifstream& in)
{
    std::string line;
    std::string prev_line;
    std::string p2_line;

    while (in >> std::ws && std::getline(in, line)) {
        if (line.find("Total") != std::string::npos) {
            return line;
        }
    } // skip empty lines
        ;

    return p2_line;
}

double time(string file) {
    std::ifstream f1(file);
    string lastLine = getLastLine(f1);
    std::vector<std::string> v;

    split( lastLine, v, ' ' );
    return stod(v[v.size() - 2]);
}
int main(int argc, char *argv[]) {
/*    vector<string> dirs = {"./2/", "./4/", "./6/", "./8/", "./10/","./12/", "./14/", "./16/", "./18/"};
    int amount = 10;

    for (int i = 0; i < dirs.size(); ++i) {
        auto d = dirs[i];
        double c1 =0, c2=0, c3=0;
        for (int j = 1; j <= amount; ++j) {
            string p = d + to_string(j) + "/";
            c1 += time(p + "stat1");
            c2 += time(p + "stat2");
            c3 += time(p + "stat3");
            //c4 += time(p + "stat4");
        }
        cout << "[" << c1/amount << "," << c2/amount << "," << c3/amount  << "]\n";
    }
*/

    int height = std::atoi(argv[1]);
    int width = std::atoi(argv[2]);

    char *f = argv[3];
    vector<string> dirs = {"./2/", "./4/", "./6/", "./8/", "./10/","./12/", "./14/", "./16/", "./18/"};
    int amount = 3;

    //cout << psnr(f, "rec.yuv", height, width) << "\n";
    for (int i = 0; i < dirs.size(); ++i) {
        vector<double> inter;
        vector<double> frCopy;
        vector<double> noPred;
        vector<double> dron;
        auto d = dirs[i];
        //cout << d << "\n";
        for (int j = 1; j <= amount; ++j) {
            string p = d + to_string(j) + "/";
            noPred.push_back(psnr(f, p + "dec_green.yuv", height, width));
            frCopy.push_back(psnr(f, p + "dec.yuv", height, width));
            inter.push_back(psnr(f, p + "dec_inter.yuv", height, width));
            //dron.push_back(psnr(f, p + "dec_dron.yuv", height, width));
        }
        float a1 = accumulate( noPred.begin(), noPred.end(), 0.0)/noPred.size();
        cout  << a1 << " ";
        float a2 = accumulate( frCopy.begin(), frCopy.end(), 0.0)/frCopy.size();
        cout << a2 << " ";
        float a3 = accumulate( inter.begin(), inter.end(), 0.0)/inter.size();
        cout  << a3 << "\n";
    }
}