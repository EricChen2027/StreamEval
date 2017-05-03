//
//  StreamEval.hpp
//  streamEval
//
//  Created by chenyu on 17/4/17.
//  Copyright © 2017年 chenyu. All rights reserved.
//

#ifndef StreamEval_hpp
#define StreamEval_hpp

#include <stdio.h>
#include <vector>


using namespace std;

#include "Common.hpp"
#include "jsoncpp/json.h"

/**
 * @ 环境初始化，一次完整的校验开始前必须调用
 * @ return if init success(-1:failed; 0:success)
 */
int Init();
int UnInit();
/**
 * @ 下载指定码流到文件
 * @ inout FILE* :码流数据下载地址
 * @ in char*    :码流地址
 * @ in int      :下载模式(model=0:下载固定大小；model=1:下载固定时长)
 * @ in uint32_t :默认下载大小(默认下载2M)
 * @ return Error Info Details
 */
unsigned int Stream2File(FILE* outFile, char* streamUrl, int model = 0, uint32_t param = 2 * 1024);

/**
 * @ 对指定文件内的数据做校验，提取H264信息，并返回校验结果
 * @ in FILE*
 * @ out FLV_HEADER       
 * @ out vector<TAG_INFO>
 * @ in STREAM_ACTION   :流操作(pause || seek)
 * @ in uint32_t        :流操作时间(ms)
 * @ return Error Info Details
 */
unsigned int ParseMediaPackage(FILE* inFile, FLV_HEADER& header, vector<FLV_TAG>& vFlvTag, STREAM_ACTION action = ACTION_NORMAL, int32_t action_time = 0);
/**
 * @ 对指定buffer中的数据做校验，提取H264信息，返回校验结果
 * @ in void*               :buffer
 * @ in long                :buffer 长度
 * @ out FLV_HEADER&        : Flv报文头
 * @ out vector<TAG_INFO>&  : Flv tag信息
 * @ return Error Info Details
 */
unsigned int ParseMediaPackage(void* buffer, long length, FLV_HEADER& header, vector<FLV_TAG>& vFlvTag, STREAM_ACTION action = ACTION_NORMAL, int32_t action_time = 0);

//校验TS封装格式数据，暂时未实现
unsigned int ParseMediaPackage(FILE* inFile);
unsigned int ParseMediaPackage(void* buffer, long length);

/**
 * @ 解析H264/H265文件并返回校验结果
 * @ out MEDIA_INFO&
 * @ out STREAM_SLICE_INFO&
 * @ return Error Info Details
 */
unsigned int ParseMediaSlice(MEDIA_INFO& media_info, vector<NALU_t>& vNalu);

/**
 * @ 校验FLV封装格式的码流信息是否正确，返回校验结果
 * @ unsigned int&       :error number
 * @ in MEDIA_INFO       :根据H264/H265文件解析出来的媒体信息
 * @ in vector<NALU_t>   :根据H264/H265文件解析出来的nalu信息
 * @ in filePath         :媒体文件目录
 * @ in model            :校验模式(model=0:静态校验；model=1:动态校验)
 */
JSONCPP_STRING StreamEval(unsigned int& errNum, MEDIA_INFO mediaInfo, vector<NALU_t> vNalu, STATIC_INFO& statInfo, string filePath, int model = 0);
JSONCPP_STRING StreamEval(unsigned int& errNum, MEDIA_INFO mediaInfo, STATIC_INFO& statInfo, string filePath, int model = 0);
unsigned int StreamEval(unsigned int& errNum, MEDIA_INFO mediaInfo, vector<NALU_t> vNalu, STATIC_INFO& statInfo, int model = 0);
//校验TS封装格式数据，暂时未实现
JSONCPP_STRING StreamEval();
void Format2Text(unsigned int errNum, vector<NALU_t> vNalu, STATIC_INFO statInfo, MEDIA_INFO mediaInfo);

/****************************/
/*******  private   *********/
/****************************/
/**
 * @ 校验结果格式化为json
 * @ in unsigned int    :erro number
 * @ in FLV_TAG_INFO    :FLV Tag 信息
 * @ in vector<NALU_t>  :H264/H265 NALU信息
 * @ in FLV_STAT_INFO   :FLV Tag && NALU统计信息
 * @ in MEDIA_INFO      :根据H264/H265解码获得的media信息
 * @ in string          :音视频文件的存储目录
 */
JSONCPP_STRING Format2Json(unsigned int errNum, vector<TAG_INFO> flvTagInfo, vector<NALU_t> vNalu, STATIC_INFO statInfo, MEDIA_INFO mediaInfo, string fileInfo);
void TimeStamp2RealTime(uint32_t timeStamp, char** time);

#endif /* StreamEval_hpp */
