#ifndef __FLV_CHECK_H__
#define __FLV_CHECK_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

using namespace std;

#include "common.h"

static int videoNum = 0;
static int audioNum = 0;
static int averageTime = 0;
static vector<uint32_t> vValidTime;			//视频有效时间间隔
static vector<uint32_t> aValidTime;			//音频有效时间间隔
static VIDEO_AUDIO_SYNCINFO syncInfo;
static FLV_TAG_INFO flvTagInfo;    	//flv tag info detail

#define STUTTER_NUM		250				//卡顿可识别的时间（单位：ms）
#define STUTTER_FPS		10				//卡顿可识别的帧率（fps）
#define STUTTER_TIME	1000			//音、视频不同步的时间间隔（单位：ms）


//校验Flv报文格式,并解析h264码流
unsigned int CheckFlvFile(FILE* flvFile, FILE** h264file);
//校验Flv Head
unsigned int CheckHead(FILE* flvFile, uint32_t& bodyType);
//校验Flv Body
unsigned int CheckBody(FILE* flvFile, FILE** h264File, uint32_t bodyType);
//校验Flv Body tag
unsigned int CheckBodyTag(FILE* flvFile, FILE** h264File, uint32_t tagType, uint32_t tagSize, int& videoNum, int& audioNum);
//timeStamp转时间
void TimeStamp2RealTime(uint32_t timeStamp, char** time);
//获取音视频封装信息
unsigned int CheckFlvDataInfo(FLV_STAT_INFO& statInfo);
FLV_TAG_INFO GetFlvTagInfo();
#endif
