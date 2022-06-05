//
// Created by Nika on 04.11.2020.
//

#ifndef HM_NALINFO_H
#define HM_NALINFO_H


#include <cstdint>
#include <string>
#include "../NalPack/NalPack.h"

class NALPack {
public:
    int32_t number;
    int unitTypeNum;
    NalUnitType unitType;
    int32_t layerId;
    int32_t temporalId;
    size_t size;
    size_t start;
    std::string info(bool read_format);

    NALPack(int unitTypeNum, int32_t layerId, int32_t temporalId): unitTypeNum(unitTypeNum), layerId(layerId), temporalId(temporalId) {
        unitType = static_cast<NalUnitType >(unitTypeNum);
    }
};



#endif //HM_NALINFO_H
