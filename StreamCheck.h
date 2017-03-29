#ifndef __STREAM_CHECK_H__
#define __STREAM_CHECK_H__

#include <stdlib.h>
#include <vector>

#include "h264bitstream/h264_stream.h"
#include "common.h"
using namespace std;

const int MAX_NAL_NUM	= 1000;
const int MAX_NAL_SIZE	= 1*1024*1024;
const int OUTPUT_SIZE	= 512*1024;

//����ͳ����Ϣ���ͱ��Ľ����޹�
static int iFrame = 0;
static int pFrame = 0;
static int bFrame = 0;
static int validPFrame = 0;
static int validBFrame = 0;
static vector<STREAM_SLICE_INFO> streamInfo;
static MEDIA_INFO g_media_info;

typedef struct {
	int pFrameNum;
	int bFrameNum;
	long gopSize; 
}GOP_INFO;
static GOP_INFO gopInfo;
static vector<unsigned int> iFrameSize;
static vector<unsigned int> bFrameSize;
static vector<unsigned int> pFrameSize;

typedef struct
{
	int type;                       // 0 -- h.264; 1 -- h.265
	unsigned int num;               // ���
	unsigned int len;               // ����ʼ����ܵĳ���
	unsigned int offset;			// nal�����ļ��е�ƫ��
	int sliceType;					// ֡����
	int nalType;					// NAL����
	int startcodeLen;				// start code����
	char startcodeBuffer[16];       // ��ʼ�룬�ַ�����ʽ
} NALU_t;

unsigned int CheckBitStream(FILE* h264File);
unsigned int probeNALU(FILE* h264File, h264_stream_t* m_hH264, vector<NALU_t>& vNal, int num);
int getAnnexbNALU(FILE* fp, h264_stream_t* m_hH264, NALU_t* nalu);
int findFirstNALU(FILE* fp, int* startcodeLenght);
void PrintNALInfo(vector<NALU_t> vNal);
void PrintStreamInfo(h264_stream_t* m_hH264);
//У��Gop�ֲ��Ƿ����
unsigned int CheckStreamDataInfo(int& vAbnormal);
//I/P/B֡ƽ����С��I֮֡���P��B֡�������Ƿ����쳣�ֲ�
int GetVideoStreamInfo(FLV_STAT_INFO& statInfo);
vector<STREAM_SLICE_INFO> GetStreamInfo();
MEDIA_INFO GetMediaInfo();

inline int findStartcode3(unsigned char *buffer) {
	return (buffer[0]==0 && buffer[1]==0 && buffer[2]==1);
}
inline int findStartcode4(unsigned char *buffer) {
	return (buffer[0]==0 && buffer[1]==0 && buffer[2]==0 && buffer[3]==1);
}

#endif
