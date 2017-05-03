//
//  FlvCheck.hpp
//  streamEval
//
//  Created by chenyu on 17/4/18.
//  Copyright © 2017年 chenyu. All rights reserved.
//

#ifndef FlvCheck_hpp
#define FlvCheck_hpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

using namespace std;

#include "Common.hpp"

#define VIDEO_STUTTER_NUM   250				//the stutter time which video lag could be feeled(ms)
#define AUDIO_STUTTER_NUM   150             //the stutter time which audio lag could be feeled(ms)
#define STUTTER_FPS         10              //the fps which video lag could be feeled
#define STUTTER_TIME        1000            //the min stutter time which audio and video unsync could be feeled(ms)

//public
/**
 * @ 解读Flv文件中数据，并提取视频编码(H264||H265)数据
 * @ in FILE*               :Flv文件指针
 * @ inout FILE**           :H264文件指针
 * @ inout FLV_HEADER&      :Flv Header
 * @ inout vector<TAG_INFO>&:Flv Tags
 * @ return Error Info Details
 */
unsigned int ParseFlvFile(FILE* flvFile, FILE** H264file, FLV_HEADER& header, vector<FLV_TAG>& vFlvTag);
unsigned int ParseFlvFile(void* buffer, long length, FILE** H264file, FLV_HEADER& header, vector<FLV_TAG>& vFlvTag, long& buffer_read);
/**
 * @ 校验单个Flv Tag信息，并汇总信息
 * @ inout vector<TAG_INFO>&    :所有Flv Tag信息
 * @ inout STATIC_INFO&         :Flv Tag的统计信息
 * @ in                         :0:静态校验；1:动态校验
 * @ return Error Info Details
 */
unsigned int CheckFlvData(vector<TAG_INFO>& flvTagInfo, STATIC_INFO& statInfo, int model = 0);
void FormatFlvTag2TagInfo(vector<FLV_TAG> vFlvTag, vector<TAG_INFO>& vTagInfo, STREAM_ACTION action = ACTION_NORMAL, uint32_t action_time = 0);
//private
/**
 * @ 提取FLV Header(仅仅是读取数据，内容可能不是Header)
 * @ in FILE*               :Flv文件指针
 * @ inout FLV_HEADER&      :Flv Header
 * @ return Error Info Details
 */
unsigned int ParseHead(FILE* flvFile, FLV_HEADER& header);
unsigned int ParseHead(void* buffer, long length, FLV_HEADER&header, long& bufferRead);
/**
 * @ 解析FLV Tags，并提取视频编码(H264||H265)数据
 * @ in FILE*               :Flv文件指针
 * @ inout FILE**           :H264文件指针
 * @ inout vector<FLV_TAG>& :Flv Tags
 * @ return Error Info Details
 */
unsigned int ParseBody(FILE* flvFile, FILE** H264file, vector<FLV_TAG>& vFlvTag);
unsigned int ParseBody(void* buffer, long length, FILE** H264file, vector<FLV_TAG>& vFlvTag, long& bufferRead);

unsigned int ParseBodyTag(FILE* flvFile, FILE** H264File, byte tagType, uint32_t tagSize);
unsigned int ParseBodyTag(byte* buffer, long length, FILE** H264File, byte tagType, uint32_t tagSize);
#endif /* FlvCheck_hpp */
