//
// Created by Nika on 01.05.2021.
//

#include "ImageRotate.h"
#include <vector>
#include <opencv2/core/core.hpp>
#include <iostream>
#include <vector>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <unordered_map>


using namespace std;

std::vector<unsigned char> ImageRotate::readBytesFromFile(const char* filename, int height, int width, int offset)
{
    std::vector<unsigned char> result;

    FILE* f = fopen(filename, "rb");

    fseek(f, offset, SEEK_SET);  // Jump to the end of the file
    //long length = ftell(f); // Get the current byte offset in the file
    //rewind(f);              // Jump back to the beginning of the file

    long length = width * height * 3 / 2;
    result.resize(length);

    char* ptr = reinterpret_cast<char*>(&(result[0]));
    fread(ptr, length, 1, f); // Read in the entire file
    fclose(f); // Close the file

    return result;
}

std::vector<unsigned char> ImageRotate::readYFromFile(const char* filename, int height, int width, int offset)
{
    std::vector<unsigned char> result;

    FILE* f = fopen(filename, "rb");

    long frameSize = width * height * 3 / 2;
    fseek(f, offset * frameSize, SEEK_SET);
    //long length = ftell(f); // Get the current byte offset in the file
    //rewind(f);              // Jump back to the beginning of the file

    long length = width * height;
    result.resize(length);

    char* ptr = reinterpret_cast<char*>(&(result[0]));
    fread(ptr, length, 1, f); // Read in the entire file
    fclose(f); // Close the file

    return result;
}
std::vector<unsigned char> ImageRotate::readY(unsigned char* pointer, int height, int width)
{
    unsigned int length = width * height;
   // vector<short> values(pointer, pointer + length);
    vector<unsigned char> result(pointer, pointer + length);
    cv::Mat resImg( height, width,  CV_8UC1, (char*) result.data());
    cv::imshow("rotated_im.png", resImg);
    cv::waitKey(0);


    return result;
}


cv::Mat ImageRotate::rotate(cv::Mat src, int angle) {
    cv::Point2f center((src.cols-1)/2.0, (src.rows-1)/2.0);
    cv::Mat rot = cv::getRotationMatrix2D(center, angle, 1.0);
    // determine bounding rectangle, center not relevant
    cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), src.size(), angle).boundingRect2f();
    // adjust transformation matrix
    rot.at<double>(0,2) += bbox.width/2.0 - src.cols/2.0;
    rot.at<double>(1,2) += bbox.height/2.0 - src.rows/2.0;
    cv::Mat dst;
    cv::warpAffine(src, dst, rot, bbox.size());
    return dst;
}
cv::Mat ImageRotate::move(cv::Mat src, int x, int y) {
    cv::Mat trans_mat = (cv::Mat_<double>(2,3) << 1, 0, max(x, 0), 0, 1, max(y, 0));
    cv::Mat dst(cv::Mat::zeros(src.rows + abs(y),src.cols + abs(x), src.type()));
    warpAffine(src,dst,trans_mat, dst.size());
    return dst;
}
double ImageRotate::compare(
        std::vector<unsigned char> img1, size_t width1, size_t height1,
        std::vector<unsigned char> img2,size_t width2, size_t height2)
{
    size_t minWidth, img1HorBorder, img2HorBorder;
    size_t minHeight, img1VerBorder, img2VerBorder;
    if (width2 < width1) {
        minWidth = width2;
        img1HorBorder = (width1 - width2) / 2;
        img2HorBorder = 0;
    } else {
        minWidth = width1;
        img1HorBorder = 0;
        img2HorBorder = (width2 - width1) / 2;
    }
    if (height2 < height1) {
        minHeight = height2;
        img1VerBorder = (height1 - height2) / 2;
        img2VerBorder = 0;
    } else {
        minHeight = height1;
        img1VerBorder = 0;
        img2VerBorder = (height2 - height1) / 2;
    }
    long cnt = 0;
    long absSum = 0;
    for (size_t i = 0; i < minHeight; ++i) {
        for (size_t j = 0; j < minWidth; ++j) {
            unsigned char img1_y = img1[(img1VerBorder + i) * width1 + img1HorBorder + j];
            unsigned char img2_y = img2[(img2VerBorder + i) * width2 + img2HorBorder + j];
            if (!img1_y || ! img2_y) {
                continue;
            }
            cnt++;
            absSum += abs(img1_y - img2_y);
        }
    }
    return (double)absSum / cnt;
}
double ImageRotate::compareWithMove(cv::Mat& img1, cv::Mat& img2, Move move) {
    cv::Mat i1Mat;
    cv::resize(img1, i1Mat, cv::Size(), 0.5, 0.5);
    vector<unsigned char> i1(i1Mat.begin<unsigned char>(), i1Mat.end<unsigned char>());
    cv::Mat rot = this->rotate(img2, move.getA());
    cv::Mat mvd = this -> move(rot, 2*move.getX(), 2*move.getY());
    cv::Mat i2Mat;
    cv::resize(mvd, i2Mat, cv::Size(), 0.5, 0.5);

    vector<unsigned char> i2(i2Mat.begin<unsigned char>(), i2Mat.end<unsigned char>());
    //cv::imshow("rotated_im.png", mvd);
    //cv::waitKey(0);
    return compare(i1, i1Mat.cols, i1Mat.rows, i2, i2Mat.cols, i2Mat.rows);
}
Move ImageRotate::findMove(cv::Mat img1, cv::Mat img2) {
    int angleDiff = 10;
    int motionDiff = 10;
    double diff = compareWithMove(img1, img2, Move(0,0,0));
    double lastDiff = 0;
    Move move(0,0,0);
    Move bestMove(0,0,0);
    std::unordered_map<Move, double, Move::HashFunction> umap;
    umap[Move(0,0,0)] = diff;
    while(!(diff == lastDiff && angleDiff == 0 && motionDiff == 0)) {
        //cout << "dx: " << move.getX() << " dy: " << move.getY() << " da: " << move.getA() << "\n";
        //cout << diff << " " << angleDiff << " " << motionDiff << "\n";
        lastDiff = diff;
        for (int angle = -1; angle <=1; angle++) {
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    Move crntMove = Move(move.getX() + dx * motionDiff, move.getY() + dy * motionDiff, move.getA() + angle * angleDiff);
                    double comp;
                    if (umap.find(crntMove) == umap.end()) {
                        comp = compareWithMove(img1, img2, crntMove);
                        umap[crntMove] = comp;
                    } else {
                        comp = umap.find(crntMove) -> second;
                    }
                    if (diff > comp) {
                        diff = comp;
                        bestMove = crntMove;
                    }

                }
            }
        }
        if (bestMove.getX() == move.getX() && bestMove.getY() == move.getY() && bestMove.getA() == move.getA()){
                angleDiff /= 2;
                motionDiff /= 2;

        }
        move = bestMove;
    }
    return bestMove;
}

cv::Mat ImageRotate::concatImage(cv::Mat img1, cv::Mat img2, Move move) {
    vector<unsigned char> i1(img1.begin<unsigned char>(), img1.end<unsigned char>());
    cv::Mat rot = this->rotate(img2, move.getA());
    cv::Mat mvd = this -> move(rot, 2*move.getX(), 2*move.getY());
    vector<unsigned char> i2(mvd.begin<unsigned char>(), mvd.end<unsigned char>());
    int width1 = img1.cols;
    int width2 = mvd.cols;
    int height1 = img1.rows;
    int height2 = mvd.rows;
    int maxWidth, maxHeight, img1HorBorder, img2HorBorder, img1VerBorder, img2VerBorder;

    if (width2 < width1) {
        maxWidth = width1;
        img2HorBorder = (width1 - width2) / 2;
        img1HorBorder = 0;
    } else {
        maxWidth = width2;
        img2HorBorder = 0;
        img1HorBorder = (width2 - width1) / 2;
    }
    if (height2 < height1) {
        maxHeight = height1;
        img2VerBorder = (height1 - height2) / 2;
        img1VerBorder = 0;
    } else {
        maxHeight = height2;
        img2VerBorder = 0;
        img1VerBorder = (height2 - height1) / 2;
    }
    vector<unsigned char> res;
    for (size_t i = 0; i < maxHeight; ++i) {
        for (size_t j = 0; j < maxWidth; ++j) {
            int cnt = 0;
            int sum = 0;
            if (j >= img1HorBorder && j < img1HorBorder + width1 && i >= img1VerBorder && i < img1VerBorder + height1){
                if (i1[(i - img1VerBorder) * width1 + j - img1HorBorder] != 0) {
                    cnt++;
                    sum += i1[(i - img1VerBorder) * width1 + j - img1HorBorder];
                }
            }
            if (j >= img2HorBorder && j < img2HorBorder + width2 && i >= img2VerBorder && i < img2VerBorder + height2){
                if (i2[(i - img2VerBorder) * width2 + j - img2HorBorder] != 0) {
                    cnt++;
                    sum += i2[(i - img2VerBorder) * width2 + j - img2HorBorder];
                }
            }
            res.push_back((unsigned char)(cnt == 0? 0: sum/cnt));
        }
    }
    cv::Mat resImg = cv::Mat( maxHeight, maxWidth,  CV_8UC1);
    memcpy(resImg.data, res.data(), res.size()*sizeof(unsigned char));
    return resImg;
}

int ImageRotate::getXShift(cv::Mat part, cv::Mat& all) {
    return (all.cols - part.cols)/2;
}

int ImageRotate::getYShift(cv::Mat part, cv::Mat& all) {
    return (all.rows - part.rows)/2;
}

cv::Mat ImageRotate::concatList(std::vector<cv::Mat> srcList) {
    int maxWidth = 0, maxHeight = 0, imgHorBorder, imgVerBorder;
    for (int i = 0; i < srcList.size(); i++) {
        maxWidth = max(maxWidth, srcList[i].cols);
        maxHeight = max(maxHeight, srcList[i].rows);
    }
    vector<unsigned char> res;
    vector<int> cnt(maxHeight * maxWidth);
    vector<int> sum(maxHeight * maxWidth);
    for(int n = 0; n < srcList.size(); ++n) {
        vector<unsigned char> crntFrame(srcList[n].begin<unsigned char>(), srcList[n].end<unsigned char>());
        imgHorBorder = (maxWidth - srcList[n].cols)/2;
        imgVerBorder = (maxHeight - srcList[n].rows)/2;
        for (size_t i = imgHorBorder; i < imgHorBorder + srcList[n].cols; ++i) {
            for (size_t j = imgVerBorder; j < imgVerBorder + srcList[n].rows; ++j) {
                if (crntFrame[(j - imgVerBorder) * srcList[n].cols + (i - imgHorBorder)] != 0) {
                    sum[j * maxWidth + i] += crntFrame[(j - imgVerBorder) * srcList[n].cols + (i - imgHorBorder)];
                    cnt[j * maxWidth + i] += 1;
                }
            }
        }
    }
    for (int i = 0; i < maxHeight * maxWidth; ++i) {
        if (cnt[i] != 0) {
            res.push_back(sum[i]/cnt[i]);
        } else {
            res.push_back(0);
        }
    }
    cv::Mat resImg = cv::Mat( maxHeight, maxWidth,  CV_8UC1);
    memcpy(resImg.data, res.data(), res.size()*sizeof(unsigned char));
    return resImg;
}

cv::Mat ImageRotate::repair(cv::Mat& res, std::vector<cv::Mat>& src) {
    vector<cv::Mat> srcList;
    srcList.push_back(res);
    for (int i = 0; i < src.size(); i++) {
        //cv::imshow("src"+to_string(i), src[i]);
        //cv::waitKey(0);

        Move move = findMove(res, src[i]);
        //vector<unsigned char> i1(src[i].begin<unsigned char>(), src[i].end<unsigned char>());
        cv::Mat rot = this->rotate(src[i], move.getA());
        cv::Mat mvd = this -> move(rot, 2*move.getX(), 2*move.getY());
        srcList.push_back(mvd);
    }
    return concatList(srcList);
}
/*
int main(int argc, char *argv[]) {
    int height = 512;
    int width = 640;
    ImageRotate ir;

    auto y1 = ir.readYFromFile("dron1.yuv", height, width, 31);
    auto y2 = ir.readYFromFile("dron1.yuv", height, width, 32);
    auto y3 = ir.readYFromFile("dron1.yuv", height, width, 33);
    auto y4 = ir.readYFromFile("dron1.yuv", height, width, 34);
    cv::Mat src1(height, width, CV_8UC1, (char*) y1.data());
    cv::Mat src2(height, width, CV_8UC1, (char*) y2.data());
    cv::Mat src3(height, width, CV_8UC1, (char*) y3.data());
    cv::Mat src4(height, width, CV_8UC1, (char*) y4.data());
    vector<cv::Mat> srcList;
    srcList.push_back(src2);
    srcList.push_back(src3);
    srcList.push_back(src4);
    auto res = ir.repair(src1, srcList);
    //cout << ir.compareWithMove(src1, src2, Move(0,0,0)) << "\n";
    //cout << ir.compareWithMove(src1, src2, Move(5,0,0)) << "\n";
    //Move b = ir.findMove(src1, src2);
    //auto res = ir.concatImage(src1, src2, b);
    cv::imshow("rotated_im.png", res);
    cv::waitKey(0);
    //cout << b.getX() << " " << b.getY() << " " << b.getA() << "\n";


    //cv::waitKey(0);
}
*/

