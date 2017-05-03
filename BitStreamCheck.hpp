//
//  BitStreamCheck.hpp
//  streamEval
//
//  Created by chenyu on 17/4/18.
//  Copyright © 2017年 chenyu. All rights reserved.
//

#ifndef BitStreamCheck_hpp
#define BitStreamCheck_hpp

#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "h264bitstream/h264_stream.h"
#include "Common.hpp"
using namespace std;

const int MAX_NAL_NUM	= 1000;
const int MAX_NAL_SIZE	= 1*1024*1024;
const int OUTPUT_SIZE	= 512*1024;

//public:
unsigned int ParseBitStream(FILE* H264File, MEDIA_INFO& media_info, vector<NALU_t>& vNalu);
/**
 * @ 暂时只用于统计，不做校验功能
 *
 */
unsigned int CheckNalu(vector<NALU_t> vNalu, STATIC_INFO& statInfo);
void CopyNalu2Info(vector<NALU_t> vNalu, vector<STREAM_NALU_INFO>& streamInfo);

//private:
unsigned int probeNALU(FILE* h264File, h264_stream_t* m_hH264, vector<NALU_t>& vNal, int num);
int getAnnexbNALU(FILE* fp, h264_stream_t* m_hH264, NALU_t* nalu);
int findFirstNALU(FILE* fp, int* startcodeLenght);
inline int findStartcode3(unsigned char *buffer) {
    return (buffer[0]==0 && buffer[1]==0 && buffer[2]==1);
}
inline int findStartcode4(unsigned char *buffer) {
    return (buffer[0]==0 && buffer[1]==0 && buffer[2]==0 && buffer[3]==1);
}


#endif /* BitStreamCheck_hpp */
