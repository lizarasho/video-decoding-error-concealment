//
// Created by Nika on 01.05.2021.
//

#ifndef HM_IMAGEROTATE_H
#define HM_IMAGEROTATE_H

#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


class Move {
    int dx;
    int dy;
    int da;
public:
    Move(int dx, int dy, int da): dx(dx), dy(dy), da(da){};
    int getX() {
        return dx;
    }
    int getY() {
        return dy;
    }
    int getA() {
        return da;
    }
    bool operator==(const Move &other) const
    { return (dx == other.dx
              && dy == other.dy
              && da == other.da);
    }
    struct HashFunction
    {
        size_t operator()(const Move& move) const
        {
            size_t dxHash = std::hash<int>()(move.dx);
            size_t dyHash = std::hash<int>()(move.dy) << 1;
            size_t daHash = std::hash<int>()(move.da) << 2;
            return dxHash ^ dyHash ^ daHash;
        }
    };
};
class ImageRotate {

public:
    std::vector<unsigned char> readBytesFromFile(const char* filename, int height, int width, int offset);
    std::vector<unsigned char> readYFromFile(const char* filename, int height, int width, int offset);
    std::vector<unsigned char> readY(unsigned char* ptr, int height, int width);

    cv::Mat rotate(cv::Mat source, int angle);
    cv::Mat move(cv::Mat source, int x, int y);
    double compare(std::vector<unsigned char> img1, size_t width1, size_t height1, std::vector<unsigned char> img2, size_t width2, size_t height2);
    double compareWithMove(cv::Mat& img1, cv::Mat& img2, Move move);
    Move findMove(cv::Mat img1, cv::Mat img2);
    cv::Mat concatImage(cv::Mat img1, cv::Mat img2, Move move);
    cv::Mat repair(cv::Mat& res, std::vector<cv::Mat>& src);
    cv::Mat concatList(std::vector<cv::Mat> srcList);
    int getXShift(cv::Mat part, cv::Mat& all);
    int getYShift(cv::Mat part, cv::Mat& all);
};

#endif //HM_IMAGEROTATE_H
