#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>

using namespace std;


#define HTON16(x)  ((x>>8&0xff)|(x<<8&0xff00))  
#define HTON24(x)  ((x>>16&0xff)|(x<<16&0xff0000)|x&0xff00)  
#define HTON32(x)  ((x>>24&0xff)|(x>>8&0xff00)|(x << 8 & 0xff0000) | (x << 24 & 0xff000000))  
#define HTONTIME(x) ((x>>16&0xff)|(x<<16&0xff0000)|(x&0xff00)|(x&0xff000000))

typedef unsigned char byte;
typedef unsigned char ui_24[3];

	 
const unsigned int STREAM_OK						= 0;
const unsigned int FILE_NOT_EXSIT					= (0x01 << 1);		//文件指针为空
const unsigned int FILE_OPEN_FAILED					= (0x01 << 2);		//文件打开失败
const unsigned int FILE_READ_ERROR					= (0x01 << 3);		//文件读异常
const unsigned int FILE_WRITE_ERROR					= (0x01 << 4);		//文件写异常
const unsigned int BUFFER_READ_ERROR				= (0x01 << 5);		//buffer读错误
const unsigned int BUFFER_WRITE_ERROR				= (0x01 << 6);		//buffer写错误
const unsigned int BUFFER_MALLOC_FAILED				= (0x01 << 7);		//内存申请失败
const unsigned int STREAM_PARSE_FAILED				= (0x01 << 8);		//流url解析失败
const unsigned int STREAM_CONNECT_ERROR				= (0x01 << 9);		//流网络连接异常
const unsigned int STREAM_DOWNLOAD_FAILED			= (0x01 << 10);		//流数据下载失败
const unsigned int LIBSOCKET_LOAD_FAILED			= (0x01 << 11);		//socket库启动异常
const unsigned int LIBCURL_LOAD_FAILED				= (0x01 << 12);		//curl.dll加载失败
const unsigned int HTTP_403_FORBIDDEN               = (0x01 << 13);
const unsigned int HTTP_404_NOTFOUND                = (0x01 << 14);


const unsigned int FLV_HEADER_ERROR					= (0x01 << 16);		//flv报文头格式错误
const unsigned int FLV_BODY_ERROR					= (0x01 << 17);		//flv报文体格式错误
const unsigned int FLV_STREAMID_NOTZERO				= (0x01 << 18);		//flv tag streamid ！= 0
const unsigned int FLV_TAGSIZE_NOTMATCH				= (0x01 << 19);		//flv tag的大小前后标注不一致
const unsigned int FLV_VIDEOTAG_ERROR				= (0x01 << 20);		//flv文件中视频tag解析失败
const unsigned int FLV_VIDEOTAGSIZE_NOTMATCH		= (0x01 << 21);		//flv videotag大小不一致
const unsigned int FLV_VIDEOTAG_NO_NALU				= (0x01 << 22);		//flv文件中没有nalu
const unsigned int FLV_AUDIOTAG_ERROR				= (0x01 << 23);		//flv文件中音频tag解析失败

const unsigned int FLV_VIDEOSTAMP_EXCEPTION			= (0x01 << 24);		//flv文件中视频时间戳异常，视频效果是画面卡顿或花屏
const unsigned int FLV_VIDEOGOP_EXCEPTION			= (0x01 << 25);		//flv文件中视频帧GOP分布异常
const unsigned int FLV_FRAMESIZE_EXCEPTION			= (0x01 << 26);		//flv文件中视频帧大小不符合均匀分布
const unsigned int FLV_FPS_TOOLOW					= (0x01 << 27);		//flv文件中视频TAG中取出异常卡顿tag之后的平均帧率低于10
const unsigned int FLV_AUDIOSTAMP_EXCEPTION			= (0x01 << 28);		//flv文件中音频时间戳异常，视频效果是声音卡顿
const unsigned int FLV_AUDIO_ONLY					= (0x01 << 29);		//flv文件中有声音没有画面，视频效果，只有声音或者有一段只有声音
const unsigned int FLV_VIDEO_ONLY					= (0x01 << 30);		//flv文件中有画面没有声音


typedef enum {
	TAG_TYPE_AUDIO = 8,
	TAG_TYPE_VIDEO = 9,
	TAG_TYPE_SCRIPT = 18,
}FLV_TAG_TYPE;
typedef enum {
	TAG_FRAME_KEY = 1,
	TAG_FRAME_INTER,
	TAG_FRAME_DISPOSABLE_INTER,
	TAG_FRAME_GENERATED_KEY,
	TAG_FRAME_VIDEO_INFO_COMMOND,
}FLV_TAG_FRAMETYPE;
typedef enum {
	TAG_CODECID_H263 = 2,
	TAG_CODECID_SCREEN_VIDEO,
	TAG_CODECID_ON2_VP6,
	TAG_CODECID_ON2_VP6_WITH_ALPHA,
	TAG_CODECID_SCREEN_VIDEO_VERSION,
	TAG_CODECID_AVC,
}FLV_TAG_CODECID;
typedef enum {
	FILE_H264 = 0,
	FILE_H265 = 1,
	FILE_UNK  = 2,
}FLV_VIDEO_AVCTYPE;

typedef struct {
	byte Signature[3];
	byte Version;
	byte Flags;
	uint32_t DataOffset;
}FLV_HEADER;

typedef struct {
	byte TagType;
	ui_24 DataSize;
	ui_24 Timestamp;
	byte TimestampExt;
	ui_24 StreamID;
}FLV_TAG;

// media info
typedef struct tagInfo{
	uint32_t tagtype;
	uint32_t tagsize;
	uint32_t timestamp;
    uint32_t timestamp_sub;

	tagInfo() {
		tagtype = 0;
		tagsize = 0;
		timestamp = 0;
		timestamp_sub = 0;
	}	
}TAG_INFO;
typedef struct flvTagInfo {
	uint32_t length;   //video length
	uint32_t size;     //media size

	uint32_t audioNum;
	uint32_t videoNum;
	vector<TAG_INFO> tagInfo;

	flvTagInfo() {
		length = 0;
		size = 0;
		videoNum = 0;
		audioNum = 0;
		tagInfo.clear();
	}
	void clear() {
		this->length = 0;
		this->size = 0;
		this->audioNum = 0;
		this->videoNum = 0;
		this->tagInfo.clear();
	}
}FLV_TAG_INFO;

typedef struct streamSliceInfo {
	uint32_t offset;
	uint32_t length;
	string startcode;
	string nalTypeInfo;
	string nalInfo;

	streamSliceInfo() {
		offset = 0;
		length = 0;
		startcode = "";
		nalTypeInfo = "";
		nalInfo = "";
	}
}STREAM_SLICE_INFO;

typedef struct mediaInfo {
	string pic_size;
	string video_format;
	string stream_type;
	string encoding_type;

	mediaInfo() {
		pic_size = "";
		video_format = "";
		stream_type = "";
		encoding_type = "";
	}
	void clear() {
		this->pic_size = "";
		this->video_format = "";
		this->stream_type = "";
		this->encoding_type = "";
	}
}MEDIA_INFO;

//stat info
typedef struct videoAudioSyncInfo {
	int videoLeadNum;			//视频TAG时间超前个数
	int videoLeadTimeAv;		//视频TAG平均超前时间
	int audioLeadNum;			//音频TAG时间超前个数
	int audioLeadTimeAv;		//音频TAG平均超前时间

	videoAudioSyncInfo() {
		videoLeadNum = 0;
		videoLeadTimeAv = 0;
		audioLeadNum = 0;
		audioLeadTimeAv = 0;
	}

	videoAudioSyncInfo& operator=(const videoAudioSyncInfo& rhs) {
		this->videoLeadNum = rhs.videoLeadNum;
		this->videoLeadTimeAv = rhs.videoLeadTimeAv;
		this->audioLeadNum = rhs.audioLeadNum;
		this->audioLeadTimeAv = rhs.audioLeadTimeAv;

		return *this;
	}

	void clear() {
		this->videoLeadNum = 0;
		this->videoLeadTimeAv = 0;
		this->audioLeadNum = 0;
		this->audioLeadTimeAv = 0;
	}
}VIDEO_AUDIO_SYNCINFO;

typedef struct flvStatInfo{
	int videoNum;					//FLV文件中视频tag数
	int audioNum;					//FLV文件中音频tag数
	int avFps;						//FLV文件中根据视频的timeStamp计算的fps
	int exFps;						//FLV文件中除去异常的timeStamp间隔计算的fps
	vector<uint32_t> vAbnormal;		//FLV文件中视频tag中timeStamp异常的内容
	int frame;						//FLV文件中视频tag中frame总数
	int iFrame;						//FLV文件中视频tag中iframe个数
	int bFrame;						//FLV文件中视频tag中bframe个数
	int pFrame;						//FLV文件中视频tag中pframe个数
	int frameAbnormal;				//FLV文件中视频帧大小异常的个数
	vector<uint32_t> aAbnormal;		//FLV文件中音频tag中timeStamp异常的内容
	VIDEO_AUDIO_SYNCINFO syncInfo;  //FLV文件中音、视频同步信息
	
	flvStatInfo() {
		videoNum = 0;
		audioNum = 0;
		avFps = 0;
		exFps = 0;
		vAbnormal.clear();
		frame = 0;
		iFrame = 0;
		bFrame = 0;
		pFrame = 0;
		frameAbnormal = 0;
		aAbnormal.clear();
	} 
}FLV_STAT_INFO;

/************************************************************************/
/* 基础函数封装                                                           */
/************************************************************************/

/*read 1 byte*/
static inline int ReadU8(uint32_t* u8, FILE* fp) {
	if(fread(u8,1,1,fp)!=1)
		return 0;
	return 1;
}
/*read 2 byte*/
static inline int ReadU16(uint32_t* u16, FILE* fp) {
	if(fread(u16,2,1,fp)!=1)
		return 0;
	*u16=HTON16(*u16);
	return 1;
}
/*read 3 byte*/
static inline int ReadU24(uint32_t* u24, FILE* fp) {
	if(fread(u24,3,1,fp)!=1)
		return 0;
	*u24=HTON24(*u24);
	return 1;
}
/*read 4 byte*/
static inline int ReadU32(uint32_t* u32, FILE* fp) {
	if(fread(u32,4,1,fp)!=1)
		return 0;
	*u32=HTON32(*u32);
	return 1;
}
/*read 1 byte,and loopback 1 byte at once*/
static inline int PeekU8(uint32_t* u8, FILE* fp) {
	if(fread(u8,1,1,fp)!=1)
		return 0;
	fseek(fp,-1,SEEK_CUR);
	return 1;
}
/*read 4 byte and convert to time format*/
static inline int ReadTime(uint32_t* utime, FILE* fp) {
	if(fread(utime,4,1,fp)!=1)
		return 0;
	*utime=HTONTIME(*utime);
	return 1;
}

inline string GetErrorMsg(unsigned int error) {
	string sMsg = "";
	if (error == STREAM_OK) {
		sMsg = "STREAM_OK";

		return sMsg;
	}
	if ((error & FILE_NOT_EXSIT) == FILE_NOT_EXSIT) {
		sMsg += "FILE_NOT_EXSIT |";
	}
	if ((error & FILE_OPEN_FAILED) == FILE_OPEN_FAILED) {
		sMsg += "FILE_OPEN_FAILED |";
	}
	if ((error & FILE_READ_ERROR) == FILE_READ_ERROR) {
		sMsg += "FILE_READ_ERROR |";
	}
	if ((error & FILE_WRITE_ERROR) == FILE_WRITE_ERROR) {
		sMsg += "FILE_WRITE_ERROR |";
	}
	if ((error & BUFFER_READ_ERROR) == BUFFER_READ_ERROR) {
		sMsg += "BUFFER_READ_ERROR |";
	}
	if ((error & BUFFER_WRITE_ERROR) == BUFFER_WRITE_ERROR) {
		sMsg += "BUFFER_WRITE_ERROR |";
	}
	if ((error & BUFFER_MALLOC_FAILED) == BUFFER_MALLOC_FAILED) {
		sMsg += "BUFFER_MALLOC_FAILED |";
	}
	if ((error & STREAM_PARSE_FAILED) == STREAM_PARSE_FAILED) {
		sMsg += "STREAM_PARSE_FAILED |";
	}
	if ((error & STREAM_CONNECT_ERROR) == STREAM_CONNECT_ERROR) {
		sMsg += "STREAM_CONNECT_ERROR |";
	}
	if ((error & STREAM_DOWNLOAD_FAILED) == STREAM_DOWNLOAD_FAILED) {
		sMsg += "STREAM_DOWNLOAD_FAILED |";
	}
	if ((error & HTTP_403_FORBIDDEN) == HTTP_403_FORBIDDEN) {
		sMsg += "HTTP_403_FORBIDDEN |";
	}
	if ((error & HTTP_404_NOTFOUND) == HTTP_404_NOTFOUND) {
		sMsg += "HTTP_404_NOTFOUND |";
	}
	if ((error & LIBSOCKET_LOAD_FAILED) == LIBSOCKET_LOAD_FAILED) {
		sMsg += "LIBSOCKET_LOAD_FAILED |";
	}
	if ((error & LIBCURL_LOAD_FAILED) == LIBCURL_LOAD_FAILED) {
		sMsg += "LIBCURL_LOAD_FAILED |";
	}
	if ((error & FLV_HEADER_ERROR) == FLV_HEADER_ERROR) {
		sMsg += "FLV_HEADER_ERROR |";
	}
	if ((error & FLV_BODY_ERROR) == FLV_BODY_ERROR) {
		sMsg += "FLV_BODY_ERROR |";
	}
	if ((error & FLV_STREAMID_NOTZERO) == FLV_STREAMID_NOTZERO) {
		sMsg += "FLV_STREAMID_NOTZERO |";
	}
	if ((error & FLV_TAGSIZE_NOTMATCH) == FLV_TAGSIZE_NOTMATCH) {
		sMsg += "FLV_TAGSIZE_NOTMATCH |";
	}
	if ((error & FLV_VIDEOTAG_ERROR) == FLV_VIDEOTAG_ERROR) {
		sMsg += "FLV_VIDEOTAG_ERROR |";
	}
	if ((error & FLV_VIDEOTAGSIZE_NOTMATCH) == FLV_VIDEOTAGSIZE_NOTMATCH) {
		sMsg += "FLV_VIDEOTAGSIZE_NOTMATCH |";
	}
	if ((error & FLV_VIDEOTAG_NO_NALU) == FLV_VIDEOTAG_NO_NALU) {
		sMsg += "FLV_VIDEOTAG_NO_NALU |";
	}
	if ((error & FLV_AUDIOTAG_ERROR) == FLV_AUDIOTAG_ERROR) {
		sMsg += "FLV_AUDIOTAG_ERROR |";
	}
	if ((error & FLV_VIDEOSTAMP_EXCEPTION) == FLV_VIDEOSTAMP_EXCEPTION) {
		sMsg += "FLV_VIDEOSTAMP_EXCEPTION |";
	}
	if ((error & FLV_VIDEOGOP_EXCEPTION) == FLV_VIDEOGOP_EXCEPTION) {
		sMsg += "FLV_VIDEOGOP_EXCEPTION |";
	}
	if ((error & FLV_FRAMESIZE_EXCEPTION) == FLV_FRAMESIZE_EXCEPTION) {
		sMsg += "FLV_FRAMESIZE_EXCEPTION |";
	}
	if ((error & FLV_FPS_TOOLOW) == FLV_FPS_TOOLOW) {
		sMsg += "FLV_FPS_TOOLOW |";
	}
	if ((error & FLV_AUDIOSTAMP_EXCEPTION) == FLV_AUDIOSTAMP_EXCEPTION) {
		sMsg += "FLV_AUDIOSTAMP_EXCEPTION |";
	}
	if ((error & FLV_AUDIO_ONLY) == FLV_AUDIO_ONLY) {
		sMsg += "FLV_AUDIO_ONLY |";
	}
	if ((error & FLV_VIDEO_ONLY) == FLV_VIDEO_ONLY) {
		sMsg += "FLV_VIDEO_ONLY |";
	}

	return sMsg;
}
#endif
