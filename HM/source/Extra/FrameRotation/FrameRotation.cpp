#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>

void rotateMat(const cv::Mat &src, double angle, cv::Mat &dst) {
    cv::Point2f center(src.cols / 2., src.rows / 2.);
    cv::Mat rotationMatrix = cv::getRotationMatrix2D(center, angle, 1.);
    cv::warpAffine(src, dst, rotationMatrix, src.size());
}

void cropToSquare(const cv::Mat &src, cv::Rect &roi, cv::Mat &dst) {
    cv::Mat croppedRef(src, roi);
    croppedRef.copyTo(dst);
}

void readFrame(int frameNumber, int width, int height, const char *inputFileName,
               std::vector<uint8_t> &Y, std::vector<uint8_t> &U, std::vector<uint8_t> &V) {
    long long frameSize = width * height * 3 / 2;
    Y.resize(width * height);
    U.resize(width * height / 4);
    V.resize(width * height / 4);

    FILE *inputFile = fopen(inputFileName, "rb");

    fseek(inputFile, frameNumber * frameSize, SEEK_SET);
    fread(&Y[0], sizeof(uint8_t), Y.size(), inputFile);
    fread(&U[0], sizeof(uint8_t), U.size(), inputFile);
    fread(&V[0], sizeof(uint8_t), V.size(), inputFile);

    fclose(inputFile);
}

void writeFrame(int width, int height, FILE *outputFile, const cv::Mat &Y_src,
                const cv::Mat &U_src, const cv::Mat &V_src) {
    fwrite(Y_src.datastart, sizeof(uint8_t), width * height, outputFile);
    fwrite(U_src.datastart, sizeof(uint8_t), width * height / 4, outputFile);
    fwrite(V_src.datastart, sizeof(uint8_t), width * height / 4, outputFile);
}

void createVideo(const char *inputFileName, int inputWidth, int inputHeight, int frameToRotate, double maxAngle,
                 const char *outputFileName, int outputFramesNumber) {
    std::vector<uint8_t> Y, U, V;

    cv::Mat Y_matrix, Y_rotated, Y_cropped;
    cv::Mat U_matrix, U_rotated, U_cropped;
    cv::Mat V_matrix, V_rotated, V_cropped;

    readFrame(frameToRotate, inputWidth, inputHeight, inputFileName, Y, U, V);

    Y_matrix = cv::Mat(inputHeight, inputWidth, CV_8UC1, Y.data());
    U_matrix = cv::Mat(inputHeight / 2, inputWidth / 2, CV_8UC1, U.data());
    V_matrix = cv::Mat(inputHeight / 2, inputWidth / 2, CV_8UC1, V.data());

    // Calculate the side of the cropped square
    int output_width;
    output_width = floor(std::min(inputHeight, inputWidth) / sqrt(2));
    output_width = (int) floor(output_width * 0.125) * 8; // Must be a multiple of the minimum CU size
//    output_width = (int) floor(output_width * 1 / 64) * 64; // Must be a multiple of the minimum CU size
//    output_width = 320;

    printf("Output square side: %d\n", output_width);

    // Calculate the coordinates of the lower left corner of the cropped square
    int rectX = (inputWidth - output_width) / 2;
    int rectY = (inputHeight - output_width) / 2;

    cv::Rect lumaROI(rectX, rectY, output_width, output_width);
    cv::Rect chromaROI(rectX / 2, rectY / 2, output_width / 2, output_width / 2);

    FILE *outputFile = fopen(outputFileName, "wb");

    for (int i = 0; i < outputFramesNumber; ++i) {
        double angle = maxAngle * i / (outputFramesNumber - 1);
        printf("Angle %d: %f\n", i, angle);

        rotateMat(Y_matrix, angle, Y_rotated);
        rotateMat(U_matrix, angle, U_rotated);
        rotateMat(V_matrix, angle, V_rotated);

        cropToSquare(Y_rotated, lumaROI, Y_cropped);
        cropToSquare(U_rotated, chromaROI, U_cropped);
        cropToSquare(V_rotated, chromaROI, V_cropped);

        writeFrame(output_width, output_width, outputFile, Y_cropped, U_cropped, V_cropped);
    }

    fclose(outputFile);
}

int main(int argc, char *argv[]) {
    const char *inputFileName = "/Users/lizarasho/itmo/diploma/rotation_videos/shields_source.yuv";
    int inputWidth = 1280;
    int inputHeight = 720;

    int frameToRotate = 173;
    double maxAngle = 90;
    const char *outputFileName = "/Users/lizarasho/itmo/diploma/rotation_videos/shields_rotated.yuv";
    int outputFramesNumber = 24;

    createVideo(inputFileName, inputWidth, inputHeight, frameToRotate, maxAngle, outputFileName, outputFramesNumber);
}
