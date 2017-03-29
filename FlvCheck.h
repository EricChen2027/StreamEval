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
static vector<uint32_t> vValidTime;			//��Ƶ��Чʱ����
static vector<uint32_t> aValidTime;			//��Ƶ��Чʱ����
static VIDEO_AUDIO_SYNCINFO syncInfo;
static FLV_TAG_INFO flvTagInfo;    	//flv tag info detail

#define STUTTER_NUM		250				//���ٿ�ʶ���ʱ�䣨��λ��ms��
#define STUTTER_FPS		10				//���ٿ�ʶ���֡�ʣ�fps��
#define STUTTER_TIME	1000			//������Ƶ��ͬ����ʱ��������λ��ms��


//У��Flv���ĸ�ʽ,������h264����
unsigned int CheckFlvFile(FILE* flvFile, FILE** h264file);
//У��Flv Head
unsigned int CheckHead(FILE* flvFile, uint32_t& bodyType);
//У��Flv Body
unsigned int CheckBody(FILE* flvFile, FILE** h264File, uint32_t bodyType);
//У��Flv Body tag
unsigned int CheckBodyTag(FILE* flvFile, FILE** h264File, uint32_t tagType, uint32_t tagSize, int& videoNum, int& audioNum);
//timeStampתʱ��
void TimeStamp2RealTime(uint32_t timeStamp, char** time);
//��ȡ����Ƶ��װ��Ϣ
unsigned int CheckFlvDataInfo(FLV_STAT_INFO& statInfo);
FLV_TAG_INFO GetFlvTagInfo();
#endif
