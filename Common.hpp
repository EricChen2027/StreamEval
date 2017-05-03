//
//  Common.h
//  streamEval
//
//  Created by chenyu on 17/4/17.
//  Copyright © 2017年 chenyu. All rights reserved.
//

#ifndef Common_hpp
#define Common_hpp

#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <time.h>

using namespace std;

#define HTON16(x)   ((x>>8&0xff)  | (x<<8&0xff00))
#define HTON24(x)   ((x>>16&0xff) | (x<<16&0xff0000)|x&0xff00)
#define HTON32(x)   ((x>>24&0xff) | (x>>8&0xff00)   |(x << 8 & 0xff0000) | (x << 24 & 0xff000000))
#define HTONTIME(x) ((x>>16&0xff) | (x<<16&0xff0000)|(x&0xff00)          | (x&0xff000000))

typedef unsigned char byte;
typedef unsigned char ui_24[3];

#define H264_FILE_NAME  "H264Temp.txt"
#define MIN_FLV_TAGSIZE 80              //小于80Byte的tag认为是对播放无效的tag，不列入统计

/************************************************************************/
/*                         Basic Functions                              */
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

/************************************************************************/
/*                         Basic Stuctures                              */
/************************************************************************/
//User actions
typedef enum {
    ACTION_NORMAL = 0,
    ACTION_PAUSE  = 1,
    ACTION_SEEK   = 2,
    ACTION_UNK    = 3,
}STREAM_ACTION;

//Package Type
typedef enum {
    PACKAGE_FLV = 0,
    PACKAGE_TS  = 1,
    PACKAGE_UNK = 2,
}MEDIA_PACKAGE_TYPE;

//Package_Flv
typedef enum {
    TAG_TYPE_AUDIO = 8,
    TAG_TYPE_VIDEO = 9,
    TAG_TYPE_SCRIPT = 18,
}FLV_TAG_TYPE;
typedef struct flvHeader{
    byte Signature[3];
    byte Version;
    byte Flags;
    uint32_t DataOffset;
}FLV_HEADER;
typedef struct flvTag{
    byte TagType;
    ui_24 DataSize;
    ui_24 Timestamp;
    byte TimestampExt;
    ui_24 StreamID;
}FLV_TAG;

//Codec
typedef enum {
    FILE_H264 = 0,
    FILE_H265 = 1,
    FILE_UNK  = 2,
}FLV_VIDEO_AVCTYPE;
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

typedef struct{
    int type;                       // 0 -- h.264; 1 -- h.265
    unsigned int num;               // ÐòºÅ
    unsigned int len;               // º¬ÆðÊ¼ÂëµÄ×ÜµÄ³¤¶È
    unsigned int offset;			// nal°üÔÚÎÄ¼þÖÐµÄÆ«ÒÆ
    int sliceType;					// Ö¡ÀàÐÍ
    int nalType;					// NALÀàÐÍ
    int startcodeLen;				// start code³¤¶È
    char startcodeBuffer[16];       // ÆðÊ¼Âë£¬×Ö·û´®ÐÎÊ½
} NALU_t;

//Static
typedef struct tagInfo{
    clock_t datatime;				//data receive time
	uint32_t datatime_sub;			//data receive time sub
    uint32_t tagtype;
    uint32_t tagsize;
    uint32_t timestamp;
    int32_t timestamp_sub;
    uint32_t action_time;           //pause||seek time
    STREAM_ACTION action;           //pause||seek action
    
    tagInfo() {
		this->datatime = 0;
		this->datatime_sub = 0;
		this->tagtype = 0;
		this->tagsize = 0;
		this->timestamp = 0;
		this->timestamp_sub = 0;
		this->action_time = 0;
		this->action = ACTION_NORMAL;
    }
	void clear() {
		this->datatime = 0;
		this->datatime_sub = 0;
		this->tagtype = 0;
		this->tagsize = 0;
		this->timestamp = 0;
		this->timestamp_sub = 0;
		this->action_time = 0;
		this->action = ACTION_NORMAL;
	}
	tagInfo& operator = (tagInfo src) {
		this->datatime = src.datatime;
		this->datatime_sub = src.datatime_sub;
		this->tagtype = src.tagtype;
		this->tagsize = src.tagsize;
		this->timestamp = src.timestamp;
		this->timestamp_sub = src.timestamp_sub;
		this->action_time = src.action_time;
		this->action = src.action;

		return *this;
	}
}TAG_INFO;

typedef struct streamNaluInfo {
    uint32_t offset;
    uint32_t length;
    string startcode;
    string nalTypeInfo;
    string nalInfo;
    
    streamNaluInfo() {
        this->offset = 0;
        this->length = 0;
        this->startcode = "";
        this->nalTypeInfo = "";
        this->nalInfo = "";
    }
}STREAM_NALU_INFO;

typedef struct mediaInfo {
	string file_info;			//H264 | H265
    string pic_size;
    string video_format;
    string profile_info;
    string encoding_type;
    
    mediaInfo() {
		this->file_info = "";
        this->pic_size = "";
        this->video_format = "";
        this->profile_info = "";
        this->encoding_type = "";
    }
    void clear() {
		this->file_info = "";
        this->pic_size = "";
        this->video_format = "";
        this->profile_info = "";
        this->encoding_type = "";
    }
}MEDIA_INFO;

//stat info
typedef struct staticInfo {
    //total
    uint32_t mediaSize;                 //size of all tags(Byte)
    uint32_t mediaLength;               //time the media can play(ms)
    uint32_t avFPS;                     //average fps
	uint32_t realFPS;					//real fps
    //flv static info
    uint32_t videoTagNum;               //valid video tag num
    uint32_t audioTagNum;               //valid audio tag num
    uint32_t audioOnly;                 //audio only(timestamp > video timeStamp + 800);
	uint32_t videoOnly;					//video only
    vector<uint32_t> vAbnormal;         //video lag times
    vector<uint32_t> aAbnormal;         //audio lag times
	vector<uint32_t> vDynamicAb;		//video dynamic lag times
	vector<uint32_t> aDynamicAb;		//audio dynamic lag times
    vector<TAG_INFO> vInvalid;          //Invalid video tag
    vector<TAG_INFO> aInvalid;          //Invalid audio tag
    
    FLV_VIDEO_AVCTYPE avctype;          //编码类型
    //H264 slice static info
    uint32_t slice_i;
    uint32_t slice_p;
    uint32_t slice_b;
    uint32_t slice_si;
    uint32_t slice_sp;
    uint32_t idr;
    
    staticInfo() {
        this->mediaSize = 0;
        this->mediaLength = 0;
        this->avFPS = 0;
		this->realFPS = 0;
        this->videoTagNum = 0;
        this->audioTagNum = 0;
		this->audioOnly = 0;
        this->videoOnly = 0;
        this->vAbnormal.clear();
        this->aAbnormal.clear();
		this->vDynamicAb.clear();
		this->aDynamicAb.clear();
        this->vInvalid.clear();
        this->aInvalid.clear();
        
        this->avctype = FILE_H264;
        this->slice_i = 0;
        this->slice_p = 0;
        this->slice_b = 0;
        this->slice_si = 0;
        this->slice_sp = 0;
        this->idr = 0;
    }
}STATIC_INFO;

/************************************************************************/
/*                         Error Info Details                           */
/************************************************************************/
const unsigned int STREAM_OK						= 0;                //Everything is ok

const unsigned int FILE_NOT_EXSIT					= (0x01 << 1);		//File Not Exist
const unsigned int FILE_OPEN_FAILED					= (0x01 << 2);		//File Open Failed
const unsigned int FILE_READ_ERROR					= (0x01 << 3);		//File Read Error
const unsigned int FILE_WRITE_ERROR					= (0x01 << 4);		//File Write Error
const unsigned int BUFFER_READ_ERROR				= (0x01 << 5);		//Buffer Rread Error
const unsigned int BUFFER_WRITE_ERROR				= (0x01 << 6);		//Buffer Write Error
const unsigned int BUFFER_MALLOC_FAILED				= (0x01 << 7);		//Buffer Alloc Failed
const unsigned int STREAM_PARSE_FAILED				= (0x01 << 8);		//LibRtmp Parse Stream Failed
const unsigned int STREAM_CONNECT_ERROR				= (0x01 << 9);		//LibRtmp Connet to Stream Failed
const unsigned int STREAM_DOWNLOAD_FAILED			= (0x01 << 10);		//Stream Download Failed
const unsigned int LIBSOCKET_LOAD_FAILED			= (0x01 << 11);		//LibRtmp Load Socket Failed
const unsigned int LIBCURL_LOAD_FAILED				= (0x01 << 12);		//LibCurl Load Failed
const unsigned int HTTP_403_FORBIDDEN               = (0x01 << 13);     //LibCurl Return 403
const unsigned int HTTP_404_NOTFOUND                = (0x01 << 14);     //LibCurl Return 404
const unsigned int DATA_BUFFER_NULL                 = (0x01 << 15);     //Data Buffer is NULL

const unsigned int FLV_HEADER_ERROR					= (0x01 << 16);		//Flv Header Error
const unsigned int FLV_BODY_ERROR					= (0x01 << 17);		//Flv Body Error
const unsigned int FLV_STREAMID_NOTZERO				= (0x01 << 18);		//Flv TagStreamID != 0
const unsigned int FLV_TAGSIZE_NOTMATCH				= (0x01 << 19);		//FLv TagSize != Expected
const unsigned int FLV_VIDEOTAG_UNKNOWN				= (0x01 << 20);		//unknown tag type
const unsigned int FLV_VIDEOTAGSIZE_NOTMATCH		= (0x01 << 21);		//Flv VideoTagSize != 6
const unsigned int FLV_VIDEOTAG_NO_NALU				= (0x01 << 22);		//Flv VideoTag is H264(NALU)
const unsigned int FLV_AUDIOTAG_ERROR				= (0x01 << 23);

const unsigned int FLV_VIDEOSTAMP_EXCEPTION			= (0x01 << 24);		//Flv VideoTag TimeStamp Exception
const unsigned int FLV_VIDEOGOP_EXCEPTION			= (0x01 << 25);		//Flv Video Gop Exception
const unsigned int FLV_FRAMESIZE_EXCEPTION			= (0x01 << 26);		//Flv Video Frame Slice Size Exception
const unsigned int FLV_FPS_TOOLOW					= (0x01 << 27);		//Flv Video FrameRate < 10
const unsigned int FLV_AUDIOSTAMP_EXCEPTION			= (0x01 << 28);		//Flv AudioTag TimeStamp Exception
const unsigned int FLV_VIDEO_ONLY		            = (0x01 << 29);     //Flv Video Only
const unsigned int FLV_AUDIO_ONLY					= (0x01 << 30);     //Flv Audio Only

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
    if ((error & DATA_BUFFER_NULL) == DATA_BUFFER_NULL) {
        sMsg += "DATA_BUFFER_NULL |";
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
    if ((error & FLV_VIDEOTAG_UNKNOWN) == FLV_VIDEOTAG_UNKNOWN) {
        sMsg += "FLV_VIDEOTAG_UNKNOWN |";
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
	if ((error & FLV_VIDEO_ONLY) == FLV_VIDEO_ONLY) {
		sMsg += "FLV_VIDEO_ONLY |";
	}
	if ((error & FLV_AUDIO_ONLY) == FLV_AUDIO_ONLY) {
		sMsg += "FLV_AUDIO_ONLY |";
	}
    
    return sMsg;
}


#endif /* Common_h */
