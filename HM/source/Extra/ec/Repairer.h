//
// Created by Nika on 28.03.2021.
//

#ifndef HM_REPAIRER_H
#define HM_REPAIRER_H

#include "TLibCommon/CommonDef.h"
#include "Way.h"
#include <TComPic.h>
#include <Utilities/TVideoIOYuv.h>

class Repairer {
public:
    // My fields for picture reconstruction
    Int m_picWidth;
    Int m_picHeight;
    Int m_maxCUWidth;
    Int m_maxCUHeight;
    Int m_cuDepth;


    Void repairPicture(TComPic *m_resPic,  TComPic *srcPic, string ecAlgo, TVideoIOYuv& videoIOYuv, string map);
    Void zeroMove(TComPic *m_resPic,  TComPic *srcPic);
    Void moveInterpolation(TComPic *m_resPic,  TComPic *srcPic);
    Void stitching(TComPic *m_resPic,  TComPic *srcPic, TVideoIOYuv& videoIOYuv, Way& way);
//    static Void buildMVVectors(TComPic *m_pcPic, UInt ctuAdr, UInt id, UInt depth);
};


#endif //HM_REPAIRER_H
