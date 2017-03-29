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

//用于统计信息，和报文解析无关
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
	unsigned int num;               // 序号
	unsigned int len;               // 含起始码的总的长度
	unsigned int offset;			// nal包在文件中的偏移
	int sliceType;					// 帧类型
	int nalType;					// NAL类型
	int startcodeLen;				// start code长度
	char startcodeBuffer[16];       // 起始码，字符串形式
} NALU_t;

unsigned int CheckBitStream(FILE* h264File);
unsigned int probeNALU(FILE* h264File, h264_stream_t* m_hH264, vector<NALU_t>& vNal, int num);
int getAnnexbNALU(FILE* fp, h264_stream_t* m_hH264, NALU_t* nalu);
int findFirstNALU(FILE* fp, int* startcodeLenght);
void PrintNALInfo(vector<NALU_t> vNal);
void PrintStreamInfo(h264_stream_t* m_hH264);
//校验Gop分布是否均匀
unsigned int CheckStreamDataInfo(int& vAbnormal);
//I/P/B帧平均大小，I帧之间的P、B帧个数；是否有异常分布
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
