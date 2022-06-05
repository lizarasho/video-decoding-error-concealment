#include <cmath>
#include <Utilities/TVideoIOYuv.h>
#include "Repairer.h"
#include "../image/ImageRotate.h"
#include "Way.h"

Void copyPart(TComPic *srcPic, TComPic *resPic, UInt uiCUAddr, UInt uiAbsZorderIdx, UInt scale, TComMv mv) {
    TComPicYuv* resYuv = resPic->getPicYuvRec();
    TComPicYuv* srcYuv = srcPic->getPicYuvRec();

    TComPicSym* pPicSym = resPic -> getPicSym();
    TComDataCU* pcCU = pPicSym->getCtu(uiCUAddr);
    //UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsZorderIdx] ];
    UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsZorderIdx] ];

    for (Int comp = 0; comp < resYuv->getNumberValidComponents(); comp++) {
        const ComponentID compId = ComponentID(comp);
        const UInt strideSrc = srcYuv->getStride(compId);
        const UInt strideRes = resYuv->getStride(compId);
        Int shiftHor=(2+srcPic->getComponentScaleX(compId));
        Int shiftVer=(2+srcPic->getComponentScaleY(compId));

        Int     refOffset  = (mv.getHor() >> shiftHor) + (mv.getVer() >> shiftVer) * strideSrc;

        Pel*    srcX     = (srcYuv->getAddr(compId, uiCUAddr, uiAbsZorderIdx)) + refOffset;

        //const Pel *srcX = (srcYuv->getAddr(compId, uiCUAddr, uiAbsZorderIdx)) - xOffset * sizeof(Pel);
        Pel *resX = (resYuv->getAddr(compId, uiCUAddr, uiAbsZorderIdx));
        UInt h = compId == 0 ? scale: scale >> 1;
        UInt w = compId == 0 ? scale: scale >> 1;
        //cout << "Copy part: " << uiLPelX << " " << uiTPelY << " Movement: " << (int) (mv.getHor()) << " " << (int)(mv.getVer()) << "\n";
        for (Int y = uiTPelY; y < uiTPelY + h; y++, srcX += strideSrc, resX += strideRes) {
            ::memcpy(resX, srcX, w * sizeof(Pel));
        }

    }
}

Void Repairer::zeroMove(TComPic *m_pcPic, TComPic *rpcPic, FILE *logFile) {
    TComPicSym *pPicSym = m_pcPic->getPicSym();
    Int brokenSlicesNumber = 0;
    for (UInt uiCUAddr = 0; uiCUAddr < pPicSym->getNumberOfCtusInFrame(); uiCUAddr++) {
        TComDataCU *pCtu = pPicSym->getCtu(uiCUAddr);
        if (pCtu->getSlice() == NULL) {
            ++brokenSlicesNumber;
            cout << uiCUAddr << " Broken Slice Found!!!\n";
            auto m_numCTUInWidth = (m_picWidth / m_maxCUWidth) + ((m_picWidth % m_maxCUWidth) ? 1 : 0);
            Int yPos = (uiCUAddr / m_numCTUInWidth) * m_maxCUHeight;
            Int xPos = (uiCUAddr % m_numCTUInWidth) * m_maxCUWidth;
            Int height = (yPos + m_maxCUHeight > m_picHeight) ? (m_picHeight - yPos) : m_maxCUHeight;
            Int width = (xPos + m_maxCUWidth > m_picWidth) ? (m_picWidth - xPos) : m_maxCUWidth;
            cout << xPos << "," << yPos << " " << xPos + width << "," << yPos + height << "\n";
            if (logFile) {
                fprintf(logFile, "%d %d %d %d %d\n", m_pcPic->getPOC(), xPos, yPos, xPos + width, yPos + height);
            }
            assert(rpcPic != m_pcPic);
            TComPicYuv *resYuv = m_pcPic->getPicYuvRec();
            TComPicYuv *srcYuv = rpcPic->getPicYuvRec();

            for (Int comp = 0; comp < resYuv->getNumberValidComponents(); comp++) {
                const ComponentID compId = ComponentID(comp);
                const UInt strideSrc = srcYuv->getStride(compId);
                const UInt strideRes = resYuv->getStride(compId);
                const Pel *srcX = (srcYuv->getAddr(compId, uiCUAddr));
                Pel *resX = (resYuv->getAddr(compId, uiCUAddr));
                UInt h = compId == 0 ? height : height >> 1;
                UInt w = compId == 0 ? width : width >> 1;
                for (Int y = yPos; y < yPos + h; y++, srcX += strideSrc, resX += strideRes) {
                    ::memcpy(resX, srcX, w * sizeof(Pel));
                    //::memset(resX, 0, w * sizeof(Pel));
                }

            }
        }
    }
}

Void Repairer::moveInterpolation(TComPic *m_pcPic,  TComPic *rpcPic) {

    TComPicSym* pPicSym = m_pcPic -> getPicSym();
    auto cuWidth = ((m_picWidth / m_maxCUWidth)  * (m_maxCUWidth / 4)) + ((m_picWidth % m_maxCUWidth) ? (m_picWidth % m_maxCUWidth) /4 : 0) + 1;
    auto cuHeight = ((m_picHeight / m_maxCUHeight) * (m_maxCUHeight /4)) + ((m_picHeight % m_maxCUHeight) ? (m_picHeight % m_maxCUHeight) /4 : 0) + 1;

    // Build matrix of existing motion vectors
    vector<vector<TComMv>> mvMatrix (cuHeight, vector<TComMv>(cuWidth, TComMv(-1000, -1000)));
    vector<vector<Bool>> mvExists (cuHeight, vector<Bool>(cuWidth, false));

    vector<UInt> brokenCtu;
    int inter_cu = 0;
    int intra_cu = 0;
    for ( UInt uiCUAddr = 0; uiCUAddr < pPicSym -> getNumberOfCtusInFrame(); uiCUAddr++ ) {
        TComDataCU* pCtu = pPicSym->getCtu(uiCUAddr);
        if (pCtu->getSlice() == NULL) {
            //cout << uiCUAddr << " Broken Slice Found!!!\n";
            brokenCtu.push_back(uiCUAddr);
        } else {
            for (UInt i = 0; i < 256; ++i) {
                UInt uiLPelX = pCtu->getCUPelX() + g_auiRasterToPelX[g_auiZscanToRaster[i]];
                UInt uiTPelY = pCtu->getCUPelY() + g_auiRasterToPelY[g_auiZscanToRaster[i]];

                if (uiLPelX < m_picWidth && uiTPelY < m_picHeight) {
                    //TComMv comMv0 = dpbForCtu.m_CUMvField[0].getMv(i);
                    if (pCtu -> getCUMvField(REF_PIC_LIST_0) -> getRefIdx(i) == 0) {
                        inter_cu++;
                        //cout << "!" <<i << " " << pCtu -> getCUMvField(REF_PIC_LIST_0) -> getRefIdx(i) << "\n";
                        TComMv comMv0 = pCtu->getCUMvField(REF_PIC_LIST_0)->getMv(i);
                        int ver = comMv0.getVer();
                        mvMatrix[uiTPelY >> 2][uiLPelX >> 2] = TComMv(comMv0.getHor(), ver);
                        mvExists[uiTPelY >> 2][uiLPelX >> 2] = true;
                    } else {

                        intra_cu++;
                        //cout << i << " " << pCtu -> getCUMvField(REF_PIC_LIST_0) -> getRefIdx(i) << "\n";
                    }
                }
                    //copyPart(rpcPic, m_pcPic, uiCUAddr, i, 4, comMv0);

            }
        }
    }
    //HORIZONTAL interpolation
    for (int i = 0; i < cuHeight; ++i) {
        for (int j = 0; j < cuWidth; ++j) {
            int start = -1;
            int end = -1;
            while (j < cuWidth && mvExists[i][j]) {
                ++j;
            }
            if (j >= cuWidth - 1) {
                break;
            }
            start = j;
            while (j < cuWidth && !mvExists[i][j]) {
                ++j;
            }
            //interpolation with one end exists
            if (j >= cuWidth - 1 || start <= 1) {
                break;
            }
            end = j;
            //block has two borders
            double middle = (end - start)/2.;
            vector<double> lagrX(end - start, 0);
            vector<double> lagrY(end - start, 0);
            for (int k = 0; k < end - start; ++k) {
                double x = - middle + k;
                double lagr0 = ((x - (-middle - 1))*(x - (middle + 2)) * (x - (middle + 1)))/
                            (((-middle - 2) - (-middle - 1))*((-middle - 2) - (middle + 2)) * ((-middle - 2)- (middle + 1)));
                double lagr1 = ((x - (-middle - 2))*(x - (middle + 2)) * (x - (middle + 1)))/
                            (((-middle - 1) - (-middle - 2))*((-middle - 1) - (middle + 2)) * ((-middle - 1)- (middle + 1)));
                double lagr2 = ((x - (-middle - 1))*(x - (middle + 2)) * (x - (-middle -2)))/
                            (((middle + 1) - (-middle - 1))*((middle + 1) - (middle + 2)) * ((middle + 1)- (-middle - 2)));
                double lagr3 = ((x - (-middle - 1))*(x - (-middle - 2)) * (x - (middle + 1)))/
                            (((middle + 2) - (-middle - 1))*((middle + 2) - (-middle - 2)) * ((middle + 2)- (middle + 1)));

                lagrX[k] = mvMatrix[i][start-2].getHor() * lagr0 + mvMatrix[i][start-1].getHor() * lagr1 +
                           mvMatrix[i][end].getHor() * lagr2 + mvMatrix[i][end+1].getHor() * lagr3;
                lagrY[k] = mvMatrix[i][start-2].getVer() * lagr0 + mvMatrix[i][start-1].getVer() * lagr1 +
                           mvMatrix[i][end].getVer() * lagr2 + mvMatrix[i][end+1].getVer() * lagr3;
            }

            for (int k = 0; k < end - start; ++k) {
                mvMatrix[i][start + k] = TComMv((Short)round(lagrX[k]), (Short)round(lagrY[k]));
            }
        }
    }
    //vertical interpolation

    for (int i = 0; i < cuWidth; ++i) {
        for (int j = 0; j < cuHeight; ++j) {
            int start = -1;
            int end = -1;
            while ( j < cuHeight && mvExists[j][i]) {
                ++j;
            }
            if (j >= cuHeight-1) {
                //cout << j << "\n";
                break;
            }
            start = j;
            while (j < cuHeight && !mvExists[j][i]) {
                ++j;
            }
            //interpolation with one end exists

            if (j == cuHeight || start < 2) {
                //cout << j << " " << start << "\n";
                break;
            }

            end = j;
            //block has two borders
            //cout << i << " " << j << "\n";
            double middle = (end - start)/2.;
            vector<double> lagrX(end - start, 0);
            vector<double> lagrY(end - start, 0);
            for (int k = 0; k < end - start; ++k) {
                double x = - middle + k;
                double lagr0 = ((x - (-middle - 1))*(x - (middle + 2)) * (x - (middle + 1)))/
                               (((-middle - 2) - (-middle - 1))*((-middle - 2) - (middle + 2)) * ((-middle - 2)- (middle + 1)));
                double lagr1 = ((x - (-middle - 2))*(x - (middle + 2)) * (x - (middle + 1)))/
                               (((-middle - 1) - (-middle - 2))*((-middle - 1) - (middle + 2)) * ((-middle - 1)- (middle + 1)));
                double lagr2 = ((x - (-middle - 1))*(x - (middle + 2)) * (x - (-middle -2)))/
                               (((middle + 1) - (-middle - 1))*((middle + 1) - (middle + 2)) * ((middle + 1)- (-middle - 2)));
                double lagr3 = ((x - (-middle - 1))*(x - (-middle - 2)) * (x - (middle + 1)))/
                               (((middle + 2) - (-middle - 1))*((middle + 2) - (-middle - 2)) * ((middle + 2)- (middle + 1)));

                lagrX[k] = mvMatrix[start-2][i].getHor() * lagr0 + mvMatrix[start-1][i].getHor() * lagr1 +
                           mvMatrix[end][i].getHor() * lagr2 + mvMatrix[end+1][i].getHor() * lagr3;
                lagrY[k] = mvMatrix[start-2][i].getVer() * lagr0 + mvMatrix[start-1][i].getVer() * lagr1 +
                           mvMatrix[end][i].getVer() * lagr2 + mvMatrix[end+1][i].getVer() * lagr3;
            }

            for (int k = 0; k < end - start; ++k) {
                if (mvMatrix[start + k][i].getVer() != -1000) {
                    auto hor = static_cast<short>(((Short) round(lagrX[k]) + mvMatrix[start + k][i].getHor()) / 2);
                    auto ver = static_cast<short>((Short) round(lagrY[k]) + mvMatrix[start + k][i].getVer())/2;
                    //cout << i << ": " << hor << " " << ver << "(" << lagrY[k] << " " << mvMatrix[start + k][i].getVer() << ")\n";
                    mvMatrix[start + k][i] = TComMv(hor, ver);
                } else {
                    mvMatrix[start + k][i] = TComMv((Short) round(lagrX[k]), (Short) round(lagrY[k]));
                }
            }
        }
    }

    //repair broken ctus
    for (int i = 0; i < brokenCtu.size(); ++i) {
        UInt id = brokenCtu[i];
        pPicSym->getCtu(id)->initCtu(m_pcPic, id);
        auto m_numCTUInWidth = (m_picWidth / m_maxCUWidth) + ((m_picWidth % m_maxCUWidth) ? 1 : 0);
        Int y = (id / m_numCTUInWidth) * m_maxCUHeight;
        Int x = (id % m_numCTUInWidth) * m_maxCUWidth;
        for (UInt j = 0; j < 256; ++j) {
            UInt uiLPelX = x + g_auiRasterToPelX[g_auiZscanToRaster[j]];
            UInt uiTPelY = y + g_auiRasterToPelY[g_auiZscanToRaster[j]];
            if (uiLPelX < m_picWidth && uiTPelY < m_picHeight) {
                //cout << x << " " <<  y << " " <<  (mvMatrix[uiTPelY >> 2][uiLPelX >> 2].getVer() >> 2) << " " << (mvMatrix[uiTPelY >> 2][uiLPelX >> 2].getHor() >> 2) << "\n";

                if (mvMatrix[uiTPelY >> 2][uiLPelX >> 2].getVer() != -1000 &&
                        (mvMatrix[uiTPelY >> 2][uiLPelX >> 2].getVer() >> 2)  <= 64 &&
                        (mvMatrix[uiTPelY >> 2][uiLPelX >> 2].getVer() >> 2)  >= -64 &&
                        (mvMatrix[uiTPelY >> 2][uiLPelX >> 2].getHor() >> 2) <= 64 &&
                        (mvMatrix[uiTPelY >> 2][uiLPelX >> 2].getHor() >> 2)  >= -64 ) {
                    //cout << "block fixed\n";
                    copyPart(rpcPic, m_pcPic, id, j, 4, mvMatrix[uiTPelY >> 2][uiLPelX >> 2]);
                    pPicSym->getCtu(id) -> getCUMvField(REF_PIC_LIST_0) ->setMv(j, mvMatrix[uiTPelY >> 2][uiLPelX >> 2]);

                } else {
                    copyPart(rpcPic, m_pcPic, id, j, 4, TComMv(0,0));
                    pPicSym->getCtu(id) -> getCUMvField(REF_PIC_LIST_0) ->setMv(j, TComMv(0,0));

                }
            }
        }
    }
}

Void Repairer::stitching(TComPic *m_pcPic,  TComPic *rpcPic, TVideoIOYuv& videoIOYuv, Way& way) {
    // read frames as image;
    TComPicYuv *srcYuv = rpcPic->getPicYuvRec();
    ImageRotate ir;
    const UInt strideSrc = srcYuv->getStride(COMPONENT_Y);
    const Pel *srcX = (srcYuv->getAddr(COMPONENT_Y));
    vector<unsigned char> srcImage;

/*    if (m_pcPic -> getPOC() >= 2) {
        readFrame(1, m_picWidth, m_picHeight, videoIOYuv);
    }
    */
    for (Int y = 0; y < m_picHeight; y++, srcX += strideSrc) {
        vector<short> line(srcX, srcX + m_picWidth);
        for (vector<short>::const_iterator i=line.begin(); i!=line.end(); i++) {
            srcImage.push_back((unsigned char)*i);
        }

    }
    //ir.readY(srcImage.data(), m_picHeight, m_picWidth);
    vector<unsigned char> resImage;
    TComPicYuv *resYuv = m_pcPic->getPicYuvRec();

    const Pel *resX = (resYuv->getAddr(COMPONENT_Y));
    for (Int y = 0; y < m_picHeight; y++, resX += strideSrc) {
        vector<short> line(resX, resX + m_picWidth);
        for (vector<short>::const_iterator i=line.begin(); i!=line.end(); i++) {
            resImage.push_back((unsigned char)*i);
        }

    }

    // check if error exists
    TComPicSym* pPicSym = m_pcPic -> getPicSym();

    bool brockenFound = false;
    Move m(0,0,0);
    cv::Mat newSrc;
    for ( UInt uiCUAddr = 0; uiCUAddr < pPicSym -> getNumberOfCtusInFrame(); uiCUAddr++ ) {
        TComDataCU *pCtu = pPicSym->getCtu(uiCUAddr);
        int xShift, yShift;
        if (pCtu->getSlice() == NULL) {
            // if found first lost, calculate move for this compare to previous
            auto m_numCTUInWidth = (m_picWidth / m_maxCUWidth) + ((m_picWidth % m_maxCUWidth) ? 1 : 0);
            Int yPos = (uiCUAddr / m_numCTUInWidth) * m_maxCUHeight;
            Int xPos = (uiCUAddr % m_numCTUInWidth) * m_maxCUWidth;
            Int height = (yPos + m_maxCUHeight > m_picHeight) ? (m_picHeight - yPos) : m_maxCUHeight;
            Int width = (xPos + m_maxCUWidth > m_picWidth) ? (m_picWidth - xPos) : m_maxCUWidth;
            for (Int comp = 0; comp < resYuv->getNumberValidComponents(); comp++) {
                if (comp == 0) {
                    if (!brockenFound) {
                        brockenFound = true;
                        cv::Mat i1 = cv::Mat(m_picHeight, m_picWidth, CV_8UC1, (char *) resImage.data());
                        cv::Mat i2 = cv::Mat(m_picHeight, m_picWidth, CV_8UC1, (char *) srcImage.data());
                        //m = ir.findMove(i1, i2);
                        vector<cv::Mat> src;
                        vector<unsigned char> buffer;
                        src.push_back(i2);
                        auto idList = way.srcList(m_pcPic -> getPOC());
                        if (idList.size() > 1) {
                            int length = m_picWidth * m_picHeight * 3 / 2;
                            buffer = videoIOYuv.read(idList[1].first * length, length);

                            cv::Mat i3 = cv::Mat(m_picHeight, m_picWidth, CV_8UC1, buffer.data());
                            //memcpy(i3.data, buffer.data(), buffer.size() * sizeof(unsigned char));
                            //src.push_back(i3);
                        }

                        newSrc = ir.repair(i1, src);
                        xShift = ir.getXShift(i1, newSrc);
                        yShift = ir.getYShift(i1, newSrc);
                    }

                    Pel *cuX = (resYuv->getAddr(COMPONENT_Y, uiCUAddr));
                    const Pel *srcX = (srcYuv->getAddr(COMPONENT_Y, uiCUAddr));
                    vector<unsigned char> srcBuff;
                    srcBuff.assign((unsigned char *) newSrc.datastart, (unsigned char *) newSrc.dataend);
                    for (Int y = yPos; y < yPos + height; y++, cuX += strideSrc, srcX += strideSrc) {
                        vector<short> line(srcX, srcX + width);
                        vector<short> srcLine;
                        for (int i = (yShift + y) * newSrc.cols + xShift + xPos;
                             i <= (yShift + y) * newSrc.cols + xShift + xPos + width; ++i) {
                            int lineIndex = i - ((yShift + y) * newSrc.cols + xShift + xPos);
                            int srcBuffIndex = i;
                            if (srcBuff[i] != 0 && lineIndex < line.size() && srcBuffIndex < srcBuff.size()) {
                                line[i - ((yShift + y) * newSrc.cols + xShift + xPos)] = ((short) srcBuff[i]);
                            }
                            //srcLine.push_back()
                        }
                        std::memcpy(cuX, line.data(), width * sizeof(Pel));
                    }
                } else {
                    const ComponentID compId = ComponentID(comp);
                    const UInt strideSrc = srcYuv->getStride(compId);
                    const UInt strideRes = resYuv->getStride(compId);
                    const Pel *srcX = (srcYuv->getAddr(compId, uiCUAddr));
                    Pel *resX = (resYuv->getAddr(compId, uiCUAddr));
                    UInt h = compId == 0 ? height : height >> 1;
                    UInt w = compId == 0 ? width : width >> 1;
                    for (Int y = yPos; y < yPos + h; y++, srcX += strideSrc, resX += strideRes) {
                        ::memcpy(resX, srcX, w * sizeof(Pel));
                        //::memset(resX, 0, w * sizeof(Pel));
                    }
                }
            }
        }
    }
}

const static string PYTHON_PATH = "/Users/lizarasho/.conda/envs/python_scripts/bin/python";
const static string EVAL_SCRIPT_PATH = "/Users/lizarasho/itmo/diploma/python_scripts/prednet/eval_one_frame.py";
const static string EVAL_SCRIPT_PATH_OPT = "/Users/lizarasho/itmo/diploma/python_scripts/prednet/eval_one_frame_opt.py";
const static string TMP_FRAME_PATH = "/Users/lizarasho/itmo/diploma/python_scripts/prednet/temporary_frame.yuv";

Int Repairer::getFrameBufferShift(Int comp) {
    switch (comp) {
        case COMPONENT_Cb:
            return m_picWidth * m_picHeight;
        case COMPONENT_Cr:
            return m_picWidth * m_picHeight * 5 / 4;
        default:
            return 0;
    }
}

Void Repairer::baseRNN(TComPic *m_pcPic, TComPic *rpcPic, const std::string &reconFileName, FILE *logFile) {
    if (m_pcPic->getPOC() < 10) {
        zeroMove(m_pcPic, rpcPic, logFile);
    } else {
        bool hasBrokenSlice = false;
        TComPicSym *pPicSym = m_pcPic->getPicSym();
        for (UInt uiCUAddr = 0; uiCUAddr < pPicSym->getNumberOfCtusInFrame(); uiCUAddr++) {
            TComDataCU *pCtu = pPicSym->getCtu(uiCUAddr);
            if (pCtu->getSlice() == NULL) {
                cout << "At least one broken slice was found: ";
                auto m_numCTUInWidth = (m_picWidth / m_maxCUWidth) + ((m_picWidth % m_maxCUWidth) ? 1 : 0);
                Int yPos = (uiCUAddr / m_numCTUInWidth) * m_maxCUHeight;
                Int xPos = (uiCUAddr % m_numCTUInWidth) * m_maxCUWidth;
                Int height = (yPos + m_maxCUHeight > m_picHeight) ? (m_picHeight - yPos) : m_maxCUHeight;
                Int width = (xPos + m_maxCUWidth > m_picWidth) ? (m_picWidth - xPos) : m_maxCUWidth;
                cout << xPos << ',' << yPos << ' ' << xPos + width << ',' << yPos + height << '\n';
                assert(rpcPic != m_pcPic);
                hasBrokenSlice = true;
                break;
            }
        }
        if (hasBrokenSlice) {
            string command = PYTHON_PATH + " " + EVAL_SCRIPT_PATH + " -i " + reconFileName + " 2>/dev/null";
            std::cout << "Command: " << command << '\n';
            system(command.c_str());
            std::vector<UChar> frameBuffer(m_picWidth * m_picHeight * 3 / 2);
            std::ifstream frameInput(TMP_FRAME_PATH, std::ios::in | std::ifstream::binary);
            frameInput.read(reinterpret_cast<char *>(&frameBuffer[0]), frameBuffer.size() * sizeof(UChar));

            TComPicYuv *resYuv = m_pcPic->getPicYuvRec();
            for (Int comp = 0; comp < resYuv->getNumberValidComponents(); comp++) {
                const auto compId = ComponentID(comp);
                Pel *resX = (resYuv->getAddr(compId));
                UInt fullHeight = resYuv->getHeight(compId);
                UInt fullWidth = resYuv->getWidth(compId);
                Int bufferShift = getFrameBufferShift(comp);
                for (Int y = 0; y < fullHeight; y++, resX += resYuv->getStride(compId)) {
                    vector<Short> line(fullWidth);
                    for (Int x = 0; x < fullWidth; x++) {
                        line[x] = (Short) frameBuffer[bufferShift + y * fullWidth + x];
                    }
                    ::memcpy(resX, line.data(), fullWidth * sizeof(Pel));
                }
            }

            std::cout << '\n';
        }
    }
}

Void Repairer::advancedRNN(TComPic *m_pcPic, TComPic *rpcPic, const std::string &reconFileName, FILE *logFile) {
    if (m_pcPic->getPOC() < 10) {
        zeroMove(m_pcPic, rpcPic, logFile);
    } else {
        TComPicSym *pPicSym = m_pcPic->getPicSym();
        for (UInt uiCUAddr = 0; uiCUAddr < pPicSym->getNumberOfCtusInFrame(); uiCUAddr++) {
            TComDataCU *pCtu = pPicSym->getCtu(uiCUAddr);
            if (pCtu->getSlice() == NULL) {
                cout << uiCUAddr << " broken slice was found: ";
                auto m_numCTUInWidth = (m_picWidth / m_maxCUWidth) + ((m_picWidth % m_maxCUWidth) ? 1 : 0);
                Int yPos = (uiCUAddr / m_numCTUInWidth) * m_maxCUHeight;
                Int xPos = (uiCUAddr % m_numCTUInWidth) * m_maxCUWidth;
                Int CU_H = (yPos + m_maxCUHeight > m_picHeight) ? (m_picHeight - yPos) : m_maxCUHeight;
                Int CU_W = (xPos + m_maxCUWidth > m_picWidth) ? (m_picWidth - xPos) : m_maxCUWidth;
                cout << xPos << ',' << yPos << ' ' << xPos + CU_W << ',' << yPos + CU_H << '\n';
                assert(rpcPic != m_pcPic);
                TComPicYuv *resYuv = m_pcPic->getPicYuvRec();
                string command = PYTHON_PATH + " " + EVAL_SCRIPT_PATH + " -i " + reconFileName + " 2>/dev/null";
                std::cout << "Command: " << command << '\n';
                system(command.c_str());
                std::vector<UChar> frameBuffer(m_picWidth * m_picHeight * 3 / 2);
                std::ifstream frameInput(TMP_FRAME_PATH, std::ios::in | std::ifstream::binary);
                frameInput.read(reinterpret_cast<char *>(&frameBuffer[0]), frameBuffer.size() * sizeof(UChar));

                for (Int comp = 0; comp < resYuv->getNumberValidComponents(); comp++) {
                    const auto compId = ComponentID(comp);
                    const UInt strideRes = resYuv->getStride(compId);
                    Pel *resX = (resYuv->getAddr(compId, uiCUAddr));
                    UInt CU_H_New = compId == 0 ? CU_H : CU_H >> 1;
                    UInt CU_W_New = compId == 0 ? CU_W : CU_W >> 1;
                    Int xPosNew = compId == 0 ? xPos : xPos >> 1;
                    Int yPosNew = compId == 0 ? yPos : yPos >> 1;
                    Int fullWidth = resYuv->getWidth(compId);
                    Int bufferShift = getFrameBufferShift(comp);
                    for (Int y = yPosNew; y < yPosNew + CU_H_New; y++, resX += strideRes) {
                        vector<Short> line(CU_W_New);
                        for (Int x = xPosNew; x < xPosNew + CU_W_New; ++x) {
                            line[x - xPosNew] = (Short) frameBuffer[bufferShift + y * fullWidth + x];
                        }
                        ::memcpy(resX, line.data(), CU_W_New * sizeof(Pel));
                    }
                }
            }
        }
    }
}

Void Repairer::optimizedRNN(TComPic *m_pcPic, TComPic *rpcPic, const std::string &reconFileName) {
    TComPicSym *pPicSym = m_pcPic->getPicSym();
    for (UInt uiCUAddr = 0; uiCUAddr < pPicSym->getNumberOfCtusInFrame(); uiCUAddr++) {
        TComDataCU *pCtu = pPicSym->getCtu(uiCUAddr);
        if (pCtu->getSlice() == NULL) {
            cout << uiCUAddr << " broken slice was found: ";
            auto m_numCTUInWidth = (m_picWidth / m_maxCUWidth) + ((m_picWidth % m_maxCUWidth) ? 1 : 0);
            Int yPos = (uiCUAddr / m_numCTUInWidth) * m_maxCUHeight;
            Int xPos = (uiCUAddr % m_numCTUInWidth) * m_maxCUWidth;
            Int CU_H = (yPos + m_maxCUHeight > m_picHeight) ? (m_picHeight - yPos) : m_maxCUHeight;
            Int CU_W = (xPos + m_maxCUWidth > m_picWidth) ? (m_picWidth - xPos) : m_maxCUWidth;
            cout << xPos << ',' << yPos << ' ' << xPos + CU_W << ',' << yPos + CU_H << '\n';
            assert(rpcPic != m_pcPic);
            TComPicYuv *resYuv = m_pcPic->getPicYuvRec();

            std::string command = std::string(PYTHON_PATH).append(" ").append(EVAL_SCRIPT_PATH_OPT)
                    .append(" -i ").append(reconFileName)
                    .append(" --width ").append(std::to_string(m_picWidth))
                    .append(" --height ").append(std::to_string(m_picHeight))
                    .append(" --x1 ").append(std::to_string(xPos))
                    .append(" --y1 ").append(std::to_string(yPos))
                    .append(" --x2 ").append(std::to_string(xPos + CU_W))
                    .append(" --y2 ").append(std::to_string(yPos + CU_H))
                    .append(" 2>/dev/null");

            std::cout << "Command: " << command << '\n';
            system(command.c_str());
            std::vector<UChar> CUBuffer(CU_W * CU_H * 3 / 2);
            std::ifstream frameInput(TMP_FRAME_PATH, std::ios::in | std::ifstream::binary);
            frameInput.read(reinterpret_cast<char *>(&CUBuffer[0]), CUBuffer.size() * sizeof(UChar));

            for (Int comp = 0; comp < resYuv->getNumberValidComponents(); comp++) {
                const auto compId = ComponentID(comp);
                const UInt strideRes = resYuv->getStride(compId);
                Pel *resX = (resYuv->getAddr(compId, uiCUAddr));
                UInt CU_H_New = compId == 0 ? CU_H : CU_H >> 1;
                UInt CU_W_New = compId == 0 ? CU_W : CU_W >> 1;
                Int xPosNew = compId == 0 ? xPos : xPos >> 1;
                Int yPosNew = compId == 0 ? yPos : yPos >> 1;
                Int bufferShift = 0;
                if (comp == 1) {
                    bufferShift = CU_W * CU_H;
                } else if (comp == 2) {
                    bufferShift = CU_W * CU_H * 5 / 4;
                }
                for (Int y = yPosNew; y < yPosNew + CU_H_New; y++, resX += strideRes) {
                    vector<Short> line(CU_W_New);
                    for (Int x = xPosNew; x < xPosNew + CU_W_New; ++x) {
                        line[x - xPosNew] = (Short) CUBuffer[bufferShift + (y - yPosNew) * CU_W_New + (x - xPosNew)];
                    }
                    ::memcpy(resX, line.data(), CU_W_New * sizeof(Pel));
                }
            }
        }
    }
}

Void Repairer::repairPicture(TComPic *m_pcPic, TComPic *rpcPic, string ecAlgo, TVideoIOYuv &videoIOYuv, string map,
                             const string &reconFileName, FILE *logFile) {
    if (ecAlgo == "1") {
        return;
    } else if (ecAlgo == "2") {
        zeroMove(m_pcPic, rpcPic, logFile);
        return;
    } else if (ecAlgo == "3") {
        moveInterpolation(m_pcPic, rpcPic);
    } else if (ecAlgo == "4") {
        std::ifstream file(map);
        Way way(file);
        stitching(m_pcPic, rpcPic, videoIOYuv, way);
    } else if (ecAlgo == "5") {
        baseRNN(m_pcPic, rpcPic, reconFileName, logFile);
    } else if (ecAlgo == "6") {
        advancedRNN(m_pcPic, rpcPic, reconFileName, logFile);
    } else if (ecAlgo == "7") {
        optimizedRNN(m_pcPic, rpcPic, reconFileName);
    }
}