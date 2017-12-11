//
//  StreamEval.cpp
//  streamEval
//
//  Created by chenyu on 17/4/17.
//  Copyright © 2017年 chenyu. All rights reserved.
//

#include "StreamEval.hpp"
#include "Stream2File.hpp"
#include "FlvCheck.hpp"
#include "BitStreamCheck.hpp"

#if (defined (WIN32) || defined (WINDOWS) || defined(_WINDOWS_))  //windows
#if defined _DEBUG
#pragma comment(lib, "lib/lib_jsond.lib")
#else
#pragma comment(lib, "lib/lib_json.lib")
#endif
#endif

static vector<TAG_INFO> s_vTagsInfo;
static FILE* s_H264File = NULL;

//初始化全局变量
int Init(){
    s_vTagsInfo.clear();
    
    if(NULL != s_H264File) {
        fclose(s_H264File);
        s_H264File = NULL;
        if (0 != remove(H264_FILE_NAME)) {
            return -1;
        }
    }
    
    s_H264File = fopen(H264_FILE_NAME, "wb+");
    if (NULL == s_H264File) {
        return -1;
    }
    
    return 0;
}
//反初始化
int UnInit() {
    s_vTagsInfo.clear();
    
    if (NULL != s_H264File) {
        fclose(s_H264File);
        s_H264File = NULL;
        if (0 != remove(H264_FILE_NAME)) {
            return -1;
        }      
    }
    return 0;
}

//码流下行，并写buffer
unsigned int Stream2File(FILE* outFile, char* streamUrl, int model, uint32_t param) {
    if (NULL == outFile) {
        return FILE_NOT_EXSIT;
    }
    
    unsigned int iRet = STREAM_OK;
    do {
        if (string::npos != string(streamUrl).find("rtmp")) {
            iRet = RTMPStreamToFlv(outFile, streamUrl, model, param);
        }else {
            iRet = HTTPStreamToFlv(outFile, streamUrl, model, param);
        }
        if (STREAM_OK != iRet) {
            printf("HTTP/RTMP Stream download Failed: %s\n", GetErrorMsg(iRet).c_str());
            break;
        }
    }while(false);
    
    return iRet;
}

//解析FLV文件
unsigned int ParseMediaPackage(FILE* inFile, FLV_HEADER& header, vector<FLV_TAG>& vFlvTag, STREAM_ACTION action, int32_t action_time) {
    if (NULL == inFile) {
        return FILE_NOT_EXSIT;
    }
    
    fseek(inFile, 0, SEEK_END);
    long flvSize = ftell(inFile);
    
    unsigned int iRet = ParseFlvFile(inFile, &s_H264File, header, vFlvTag);
    if (STREAM_OK != iRet) {
        long errPos = ftell(inFile);
        printf("The Stream Packet Formated Flv Failed: %s\n", GetErrorMsg(iRet).c_str());
        printf("And Error Occured at %7ld , %7ld Byte left\n", errPos, flvSize - errPos);
    }
    
    //transform FLV_TAG to TAG_INFO and pushback to global vector
    FormatFlvTag2TagInfo(vFlvTag, s_vTagsInfo, action, action_time);
    
    return iRet;
}
unsigned int ParseMediaPackage(void* buffer, long length, FLV_HEADER& header, vector<FLV_TAG>& vFlvTag, STREAM_ACTION action, int32_t action_time) {
    if (NULL == buffer || length <= 0) {
        return DATA_BUFFER_NULL;
    }
    
    long buffer_read = 0;
    unsigned int iRet = ParseFlvFile(buffer, length, &s_H264File, header, vFlvTag, buffer_read);
    if (STREAM_OK != iRet) {
        printf("The Stream Packet Formated Flv Failed: %s\n", GetErrorMsg(iRet).c_str());
        printf("And Error Occured at %7ld , %7ld Byte left\n", buffer_read, length - buffer_read);
    }
    
    //transform FLV_TAG to TAG_INFO and pushback to global vector
    FormatFlvTag2TagInfo(vFlvTag, s_vTagsInfo, action, action_time);
    
    return iRet;
}

//为TS预留
unsigned int ParseMediaPackage(FILE* inFile) {
    return STREAM_OK;
}
unsigned int ParseMediaPackage(void* buffer, long length) {
    return STREAM_OK;
}

//解析H264/H265 Slice信息
unsigned int ParseMediaSlice(MEDIA_INFO& media_info, vector<NALU_t>& vNalu){
    if (NULL == s_H264File) {
        return FILE_NOT_EXSIT;
    }
    
    fseek(s_H264File, 0, SEEK_SET);
    unsigned int iRet = ParseBitStream(s_H264File, media_info, vNalu);
    if (STREAM_OK != iRet) {
        printf("The H264/H265 StreamData Formated Failed: %s\n", GetErrorMsg(iRet).c_str());
    }
    
    return iRet;
}

JSONCPP_STRING StreamEval(unsigned int& errNum, MEDIA_INFO mediaInfo, vector<NALU_t> vNalu, STATIC_INFO& statInfo, string filePath, int model) {
    //check Flv tag
	if (s_vTagsInfo.size() > 0) {
		errNum |= CheckFlvData(s_vTagsInfo, statInfo, model);
	}
    //check FLV NALU
	if (vNalu.size() > 0) {
		errNum |= CheckNalu(vNalu, statInfo);
	}
    //format to json
    JSONCPP_STRING eval_result = Format2Json(errNum, s_vTagsInfo, vNalu, statInfo, mediaInfo, filePath);

    return eval_result;
}
/**
 * @ 只校验Flv tag相关信息，不校验编码信息
 *
 */
JSONCPP_STRING StreamEval(unsigned int& errNum, MEDIA_INFO mediaInfo, STATIC_INFO& statInfo, string filePath, int model) {
	//check Flv tag
	if (s_vTagsInfo.size() > 0) {
		errNum |= CheckFlvData(s_vTagsInfo, statInfo, model);
	}

    vector<NALU_t> vNalu;    
    JSONCPP_STRING eval_result = Format2Json(errNum, s_vTagsInfo, vNalu, statInfo, mediaInfo, filePath);
    
    return eval_result;
}
unsigned int StreamEval(unsigned int& errNum, MEDIA_INFO mediaInfo, vector<NALU_t> vNalu, STATIC_INFO& statInfo, int model) {
	//check Flv tag
	if (s_vTagsInfo.size() > 0) {
		errNum |= CheckFlvData(s_vTagsInfo, statInfo, model);
	}
	//check FLV NALU
	if (vNalu.size() > 0) {
		errNum |= CheckNalu(vNalu, statInfo);
	}

	return errNum;
}
JSONCPP_STRING StreamEval() {
    JSONCPP_STRING eval_result;

    return eval_result;
}
/**
 * @ 校验结果格式化为json
 * @ in unsigned int    :erro number
 * @ in FLV_TAG_INFO    :FLV Tag 信息
 * @ in vector<STREAM_SLICE_INFO>  :H264/H265 NALU信息
 * @ in FLV_STAT_INFO   :FLV Tag && NALU统计信息
 * @ in MEDIA_INFO      :根据H264/H265解码获得的media信息
 * @ in string          :音视频文件的存储目录
 */
JSONCPP_STRING Format2Json(unsigned int errNum, vector<TAG_INFO> flvTagInfo, vector<NALU_t> vNalu, STATIC_INFO statInfo, MEDIA_INFO mediaInfo, string fileInfo) {
    Json::Value root;
    // errono
    root["error_num"] = errNum;
    
    // error_info
    Json::Value error_info;
    error_info["error_msg"] = GetErrorMsg(errNum);
	Json::Value fps;
	fps["fps_average"] = statInfo.avFPS;
	fps["fps_realplay"] = statInfo.realFPS;
    error_info["fps"] = fps;
    Json::Value lag;
    Json::Value videolag;
    for (vector<int32_t>::iterator it = statInfo.vAbnormal.begin(); it != statInfo.vAbnormal.end(); ++it) {
        videolag.append(*it);
    }
    Json::Value audiolag;
    for (vector<int32_t>::iterator it = statInfo.aAbnormal.begin(); it != statInfo.aAbnormal.end(); ++it) {
        audiolag.append(*it);
    }
    lag["video_lag"] = videolag;
    lag["audio_lag"] = audiolag;
    error_info["lag_info"] = lag;
	Json::Value sync_info;
	sync_info["video_only"] = statInfo.videoOnly;
	sync_info["audio_only"] = statInfo.audioOnly;
    error_info["sync_info"] = sync_info;
	Json::Value order_info;
	Json::Value audio_error;
	for (vector<int32_t>::iterator it = statInfo.aTransmitAb.begin(); it != statInfo.aTransmitAb.end(); ++it) {
		audio_error.append(*it);
	}
	Json::Value video_error;
	for (vector<int32_t>::iterator it = statInfo.vTransmitAb.begin(); it != statInfo.vTransmitAb.end(); ++it) {
		video_error.append(*it);
	}
	order_info["audio_error"] = audio_error;
	order_info["video_error"] = video_error;
	error_info["order_info"] = order_info;
    root["error_info"] = error_info;
    
    // media_info
    Json::Value media_info;
    media_info["size"] = statInfo.mediaSize;
    media_info["length"] = statInfo.mediaLength / 1000;
    media_info["fps"]  = fps;
    if (statInfo.mediaLength!= 0) {
        media_info["bitrate"]  = (statInfo.mediaSize + 9) / (statInfo.mediaLength / 1000) * 8 / 1024;
    }else {
        media_info["bitrate"] = 0;
    }
	media_info["mux_form"] = "flv";
	media_info["file_info"] = mediaInfo.file_info;
	media_info["pic_size"] = mediaInfo.pic_size;
    media_info["video_format"] = mediaInfo.video_format;
    media_info["profile_info"] = mediaInfo.profile_info;
    media_info["encoding_type"] = mediaInfo.encoding_type;
    media_info["file_path"] = fileInfo;
    root["media_info"] = media_info;
    
    // packet info
    Json::Value packet_info;
    packet_info["videoTagNum"] = statInfo.videoTagNum;
    packet_info["audioTagNum"] = statInfo.audioTagNum;
    Json::Value tag_info;
    for(vector<TAG_INFO>::iterator it = flvTagInfo.begin(); it != flvTagInfo.end(); ++it) {
        Json::Value packetItem;
        packetItem["tagtype"] = (*it).tagtype;
        packetItem["tagsize"] = (*it).tagsize;
        packetItem["timestamp"] = (*it).timestamp;
        packetItem["timestamp_sub"] = (*it).timestamp_sub;
        tag_info.append(packetItem);
    }
    packet_info["tag_info"] = tag_info;
    root["packet_info"] = packet_info;
    
    // frame info
    Json::Value frame_info;
    frame_info["slice_i"] = statInfo.slice_i;
    frame_info["slice_p"] = statInfo.slice_p;
    frame_info["slice_b"] = statInfo.slice_b;
    frame_info["slice_si"] = statInfo.slice_si;
    frame_info["slice_sp"] = statInfo.slice_sp;
    frame_info["idr"] = statInfo.idr;
    Json::Value slice_info;
    vector<STREAM_NALU_INFO> streamNaluInfo;
    CopyNalu2Info(vNalu, streamNaluInfo);
    for(vector<STREAM_NALU_INFO>::iterator it = streamNaluInfo.begin(); it != streamNaluInfo.end(); ++it) {
        Json::Value sliceItem;
        sliceItem["offset"] = (*it).offset;
        sliceItem["length"] = (*it).length;
        sliceItem["startcode"] = (*it).startcode;
        sliceItem["naltype"] = (*it).nalTypeInfo;
        sliceItem["info"] = (*it).nalInfo;
        slice_info.append(sliceItem);
    }
    frame_info["slice_info"] = slice_info;
    root["frame_info"] = frame_info;
    
    JSONCPP_STRING eval_result = root.toStyledString();
    return eval_result;
}

void Format2Text(unsigned int errNum, vector<NALU_t> vNalu, STATIC_INFO statInfo, MEDIA_INFO mediaInfo){
    /******************/
    /** Flv Tag Info **/
    /******************/
	printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
	printf("|                               Flv Tag Info                                         |\n");
	printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
	printf("|TagNum    |TagType   |TagSize    |TimeStamp   |timeStampExt |TimeStampSub |StreamID |\n");
    
    int tagNum = 1;
#ifdef TIMESTAMP_TO_REALTIME
    char* realTime = (char*)malloc(20);
    memset(realTime, 0, 20);
#endif
    for(vector<TAG_INFO>::iterator it = s_vTagsInfo.begin(); it != s_vTagsInfo.end(); ++it) {
#ifdef TIMESTAMP_TO_REALTIME
        TimeStamp2RealTime((*it).timestamp, &realTime);
		printf("|%10d|%10d|%11d|%s|%13d|%9d|\n", tagNum, (*it).tagtype, (*it).tagsize, realTime, (*it).timestampExt, (*it).streamID);
        //printf("|%10d|%10d|%11d|%12d|%13d|\n", tagNum, (*it).tagtype, (*it).tagsize, realTime, (*it).timestamp_sub);
#else
		printf("|%10d|%10d|%11d|%12d|%13d|%13d|%9d|\n", tagNum, (*it).tagtype, (*it).tagsize, (*it).timestampSrc, (*it).timestampExt, (*it).timestamp_sub, (*it).streamID);
        //printf("|%10d|%10d|%11d|%12d|%13d|\n", tagNum, (*it).tagtype, (*it).tagsize, (*it).timestamp, (*it).timestamp_sub);
#endif
        tagNum++;
    }
	printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");

#ifdef TIMESTAMP_TO_REALTIME
    if (realTime) {
        free(realTime);
        realTime = NULL;
    }
#endif
    /******************/
    /*** NALU Info ***/
    /******************/
    if (vNalu.size() > 0) {
        if (FILE_H264 == vNalu[0].type) {
            printf("The Video CodecID is 7 and the code type is H.264\n");
        }else {
            printf("The Video CodecID is 7 and the code type is H.265\n");
        }
        printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
        printf("|                            Video  Tag  Information                                       |\n");
        printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
        printf("|No.   |Offset    |Length |Start Code  |NAL Type                             |Info         |\n");
        
        vector<STREAM_NALU_INFO> streamNaluInfo;
        CopyNalu2Info(vNalu, streamNaluInfo);
        int nalu_num = 1;
        for (vector<STREAM_NALU_INFO>::iterator it = streamNaluInfo.begin(); it != streamNaluInfo.end(); ++it) {
            printf("|%-6d|%08X  |%-7d|%-12s|%-37s|%-13s|\n", nalu_num, (*it).offset, (*it).length, (*it).startcode.c_str(), (*it).nalTypeInfo.c_str(), (*it).nalInfo.c_str());
            nalu_num++;
        }
        printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
        
    }
    
    /******************/
    /*** Media Info ***/
    /******************/
    printf("\n\n");
    printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
    printf("|                            Video               Info                                      |\n");
    printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
    if (vNalu.size() > 0) {
        printf("%s File Information\r\n", mediaInfo.file_info.c_str());
        printf("Picture Size \t: %s\n", mediaInfo.pic_size.c_str());
        printf("Video Format \t: %s\n", mediaInfo.video_format.c_str());
        printf("Stream Type \t: %s\n", mediaInfo.profile_info.c_str());
        printf("Encoding Type \t: %s\n", mediaInfo.encoding_type.c_str());
    }
    
    /******************/
    /*** Static Info **/
    /******************/
    printf("\n\n");
    printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
    printf("|                            Video     Statistic      Info                                 |\n");
    printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
    printf("- Video Tag Num.                       : %d\n", statInfo.videoTagNum);
    printf("  - Video slice info\n");
	printf("    Video iidr Num.                    : %d\n", statInfo.idr);
	printf("    Video pSlice Num.                  : %d\n", statInfo.slice_p);
    printf("    Video bSlice Num.                  : %d\n", statInfo.slice_b);
	printf("    Video iSlice Num.                  : %d\n", statInfo.slice_i);
    printf("    Video siSlice Num.                 : %d\n", statInfo.slice_si);
    printf("    Video spSlice Num.                 : %d\n", statInfo.slice_sp);	   
    printf("  - Video Fps Average                  : %d(fps)\n", statInfo.avFPS);
	printf("  - Video Fps RealPlay                 : %d(fps)\n", statInfo.realFPS);
    //video lag and invalid video tag
    long lVAbnormal = statInfo.vAbnormal.size();
    if (lVAbnormal > 0) {
        printf("  - Video TimeStamp Abnormal           : %ld\n", lVAbnormal);
        for (vector<int32_t>::iterator it = statInfo.vAbnormal.begin(); it != statInfo.vAbnormal.end(); ++it) {
            printf("    Abnormal TimeInterval              : %d(ms)\n", *it);
        }
    }
    long lVInvalid = statInfo.vInvalid.size();
    if (lVInvalid > 0) {
        printf("  - Video Tag Invalid                  : %ld\n", lVInvalid);
        for (vector<TAG_INFO>::iterator it = statInfo.vInvalid.begin(); it != statInfo.vInvalid.end(); ++it) {
            printf("    Invalid Video Tag Size             : %d(Byte)\n", (*it).tagsize);
	        }
    }
    //audio lag and invalid audio tag
	printf("- Audio Tag Num.                       : %d\n", statInfo.audioTagNum);
    long lAAbnormal = statInfo.aAbnormal.size();
    if (lAAbnormal > 0) {
        printf("  - Audio TimeStamp Abnormal           : %ld\n", lAAbnormal);
        for (vector<int32_t>::iterator it = statInfo.aAbnormal.begin(); it != statInfo.aAbnormal.end(); ++it) {
            printf("    Abnormal TimeInterval              : %d(ms)\n", *it);
        }
    }
    long lAVInvalid = statInfo.aInvalid.size();
    if (lAVInvalid > 0) {
        printf("  - Audio Tag Invalid                  : %ld\n", lAVInvalid);
        for (vector<TAG_INFO>::iterator it = statInfo.aInvalid.begin(); it != statInfo.aInvalid.end(); ++it) {
            printf("    Invalid Audio Tag Size             : %d(Byte)\n", (*it).tagsize);
        }
    }
    //nuber of unsync
    uint32_t video_only = statInfo.videoOnly;
    if (video_only > 0) {
		printf("\n");
        printf("- There are %d(Total:%d) audio tags that could not play with the video!\n", video_only, statInfo.audioTagNum);
    }
	uint32_t audio_only = statInfo.audioOnly;
	if (audio_only > 0) {
		printf("\n");
		printf("- There are %d(Total:%d) video tags that could not play with the audio!\n", audio_only, statInfo.videoTagNum);
	}
    
    /******************/
    /*** Eval Result **/
    /******************/
    if (STREAM_OK != errNum) {
        printf("\n");
        printf("The Statistic Information of Stream Got an Exception: %s\n", GetErrorMsg(errNum).c_str());
    }else {
		printf("\n");
		printf("The Live Stream is Normal~");
	}
    return;
}

void TimeStamp2RealTime(uint32_t timeStamp, char** time) {
    int hour = timeStamp/(3600*1000);
    timeStamp-= hour * 3600*1000;
    int minutes = timeStamp/(60*1000);
    timeStamp-= minutes * 60*1000;
    int seconds = timeStamp/1000;
    timeStamp-= seconds*1000;
    
    sprintf(*time, "%2d:%2d:%2d:%3d", hour, minutes, seconds, timeStamp);
    
    return;
}
