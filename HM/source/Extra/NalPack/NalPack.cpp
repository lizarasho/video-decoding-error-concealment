//
// Created by Nika on 04.11.2020.
//

#include "NalPack.h"
#include <list>
#include <vector>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <fstream>
#include <iostream>

std::string typeToString(NalUnitType type) {
    if (type == NAL_UNIT_CODED_SLICE_TRAIL_N)
        return "TRAIL_N";
    else if (type == NAL_UNIT_CODED_SLICE_TRAIL_R)
        return "TRAIL_R";
    else if (type == NAL_UNIT_CODED_SLICE_TSA_N)
        return "TSA_N";
    else if (type == NAL_UNIT_CODED_SLICE_TSA_R)
        return "TSA_R";
    else if (type == NAL_UNIT_CODED_SLICE_STSA_N)
        return "STSA_N";
    else if (type == NAL_UNIT_CODED_SLICE_STSA_R)
        return "STSA_R";
    else if (type == NAL_UNIT_CODED_SLICE_RADL_N)
        return "RADL_N";
    else if (type == NAL_UNIT_CODED_SLICE_RADL_R)
        return "RADL_R";
    else if (type == NAL_UNIT_CODED_SLICE_RASL_N)
        return "RASL_N";
    else if (type == NAL_UNIT_CODED_SLICE_RASL_R)
        return "RASL_R";
    else if (type == NAL_UNIT_RESERVED_VCL_N10)
        return "RSV_VCL_N10";
    else if (type == NAL_UNIT_RESERVED_VCL_N12)
        return "RSV_VCL_N12";
    else if (type == NAL_UNIT_RESERVED_VCL_N14)
        return "RSV_VCL_N14";
    else if (type == NAL_UNIT_RESERVED_VCL_R11)
        return "RSV_VCL_R11";
    else if (type == NAL_UNIT_RESERVED_VCL_R13)
        return "RSV_VCL_R13";
    else if (type == NAL_UNIT_RESERVED_VCL_R15)
        return "RSV_VCL_R15";
    else if (type == NAL_UNIT_CODED_SLICE_BLA_W_LP)
        return "BLA_W_LP";
    else if (type == NAL_UNIT_CODED_SLICE_BLA_W_RADL)
        return "BLA_W_RADL";
    else if (type == NAL_UNIT_CODED_SLICE_BLA_N_LP)
        return "BLA_N_LP";
    else if (type == NAL_UNIT_CODED_SLICE_IDR_W_RADL)
        return "IDR_W_RADL";
    else if (type == NAL_UNIT_CODED_SLICE_IDR_N_LP)
        return "IDR_N_LP";
    else if (type == NAL_UNIT_CODED_SLICE_CRA)
        return "CRA_NUT";
    else if (type == NAL_UNIT_RESERVED_IRAP_VCL22 || type == NAL_UNIT_RESERVED_IRAP_VCL23)
        return "RSV_IRAP_VCL";
    else if (type == NAL_UNIT_RESERVED_VCL24 ||
             type == NAL_UNIT_RESERVED_VCL25 ||
             type == NAL_UNIT_RESERVED_VCL26 ||
             type == NAL_UNIT_RESERVED_VCL27 ||
             type == NAL_UNIT_RESERVED_VCL28 ||
             type == NAL_UNIT_RESERVED_VCL29 ||
             type == NAL_UNIT_RESERVED_VCL30 ||
             type == NAL_UNIT_RESERVED_VCL31)
        return "RSV_VCL";
    else if (type == NAL_UNIT_VPS)
        return "VPS";
    else if (type == NAL_UNIT_SPS)
        return "SPS";
    else if (type == NAL_UNIT_PPS)
        return "PPS";
    else if (type == NAL_UNIT_ACCESS_UNIT_DELIMITER)
        return "AUD";
    else if (type == NAL_UNIT_EOS)
        return "EOS";
    else if (type == NAL_UNIT_EOB)
        return "EOB";
    else if (type == NAL_UNIT_FILLER_DATA)
        return "FD";
    else if (type == NAL_UNIT_PREFIX_SEI)
        return "PREFIX_SEI";
    else if (type == NAL_UNIT_SUFFIX_SEI)
        return "SUFFIX_SEI";

    else if (type == NAL_UNIT_RESERVED_NVCL41 ||
             type == NAL_UNIT_RESERVED_NVCL42 ||
             type == NAL_UNIT_RESERVED_NVCL43 ||
             type == NAL_UNIT_RESERVED_NVCL44 ||
             type == NAL_UNIT_RESERVED_NVCL45 ||
             type == NAL_UNIT_RESERVED_NVCL46 ||
             type == NAL_UNIT_RESERVED_NVCL47)
        return "RSV_NVCL";
    else if (type == NAL_UNIT_INVALID)
        return "Error";
    else {
        return "UNSPEC";
    }
}

bool isSkippable(NalUnitType type) {
    return !(
            type == NAL_UNIT_CODED_SLICE_IDR_W_RADL ||
            type == NAL_UNIT_CODED_SLICE_IDR_N_LP ||
            type == NAL_UNIT_VPS ||
            type == NAL_UNIT_SPS ||
            type == NAL_UNIT_PPS
            );
}
NalUnitType typeFromInt(uint32_t type) {
    return static_cast<NalUnitType>(type);
}