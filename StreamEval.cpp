#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"
#include "Stream2File.h"
#include "FlvCheck.h"
#include "StreamCheck.h"
#include "StreamEval.h"

//#define RTMP_URL "rtmp://111.206.23.136/liveshow/rnjy_5f8c0c855787fa707a9dc855fc83e7d08daacde4"
//#define RTMP_URL "rtmp://relay.live.video.qiyi.com:1935/liveshow/rnjy_75e8863e45763891003f151d56818bb9ae396517"
//#define RTMP_URL "rtmp:\/\/live.hkstv.hk.lxdns.com/live/hks"
//#define RTMP_URL "rtmp://10.15.243.70/myapp/livestream"
//#define RTMP_URL "http:\/\/relay.live.video.qiyi.com:1935/liveshow/rtcq_130246e42c04f6d6068a654566cc4cd436de7875.flv&ip=10.221.32.43"
//#define RTMP_URL "http://relay.live.video.qiyi.com:1935/liveugc/rodxtbj3_dkhh1c.flv&ip=10.15.110.62"
//#define RTMP_URL "rtmp://10.11.50.195:1935/liveshow/rtcq_ddf066656871a016650f53e4f731c186600d0242"
//熊猫TV
//#define RTMP_URL "http://pl8.live.panda.tv/live_panda/d8bad1ee3e3556ae04c288e3203c4770.flv?sign=f02d7ae65b0212bc03c12ea52b282e2f&ts=584e7259&rid=39914870"
//花椒
//#define RTMP_URL "http://124.165.216.248/pl1.live.huajiao.com/live_huajiao_v2/_LC_ps1_non_2321080014815964061963833_SX.flv?wshc_tag=1&wsiphost=ipdbm"
//虎牙,猜测是协议不支持
//#define RTMP_URL "http://hls.huanjuyun.com/newlive/82393037_2516755710.url?appId=0&ex_channelid=0&ex_spkuid=0&ex_coderate=0&ex_proto=bin&ex_cdn=1&ex_clientver=1437759&type=proxy&org=web&ex_proxy=62&ex_client=8"

#define MAX_DOWNLOAD_SIZE 2 * 1024 * 1024
#define FLV_FILE "/home/chenyu/test/StreamEval/receive.flv"
#define H264_FILE "/home/chenyu/test/StreamEval/h264file.h264"
#define OUT_FILE "/home/chenyu/test/StreamEval/outFile.txt"

using namespace std;

void PrintStaticInfo(FLV_STAT_INFO* statInfo) {

	printf("\n\n");
	printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
	printf("|                            Video     Statistic      Info                                 |\n");
	printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
	//打印视频信息
	printf("- Video Tag Num                       : %d\n", statInfo->videoNum);
	printf("  - Video frame  Num                  : %d\n", statInfo->frame);
	printf("    Video iFrame Num                  : %d\n", statInfo->iFrame);
	printf("    Video pFrame Num                  : %d\n", statInfo->pFrame);
	printf("    Video bFrame Num                  : %d\n", statInfo->bFrame);
	printf("  - Video Fps Average\n");
	printf("    Fps Average                       : %d(fps)\n", statInfo->avFps);
	printf("    Fps Real                          : %d(fps)\n", statInfo->exFps);
	if (statInfo->vAbnormal.size() > 0) {
		printf("  - Video TimeStamp Abnormal          : %d\n", statInfo->vAbnormal.size());
		for(vector<uint32_t>::iterator it = statInfo->vAbnormal.begin(); it != statInfo->vAbnormal.end(); it++) {
			printf("    Abnormal TimeInterval             : %d(ms)\n", *it);
		}
		
	}
	if (statInfo->frameAbnormal > 0) {
		printf("   Video Frame Abnormal               : %d\n", statInfo->frameAbnormal);
	}
	//打印音频信息
	printf("- Audio Tag Num                       : %d\n", statInfo->audioNum);
	if (statInfo->aAbnormal.size() > 0) {
		printf("  - Audio TimeStamp Abnormal          : %d\n", statInfo->aAbnormal.size());
		for(vector<uint32_t>::iterator it = statInfo->aAbnormal.begin(); it != statInfo->aAbnormal.end(); it++) {
			printf("    Abnormal TimeInterval             : %d(ms)\n", *it);
		}
	}
	//打印音、视频同步信息
	int audioLeadNum = statInfo->syncInfo.audioLeadNum;
	int videoLeadNum = statInfo->syncInfo.videoLeadNum;
	if (audioLeadNum > 0 || videoLeadNum > 0) {
		printf("- Audio And Video TimeStamp Not Match \n");
		if (audioLeadNum > 0) {
			printf("  - AudioTime Ahead Of VideoTime Num. : %d\n", audioLeadNum);
			printf("    AudioTime Leads VideoTime         : %d(ms)\n", statInfo->syncInfo.audioLeadTimeAv / audioLeadNum);
		}
		if (videoLeadNum > 0) {
			printf("  - VideoTime Ahead Of AudioTime Num. : %d\n", videoLeadNum);
			printf("    VideoTime Lead AudioTime          : %d\n", statInfo->syncInfo.videoLeadTimeAv / videoLeadNum);
		}
	}
}

JSONCPP_STRING GenMediaInfo(unsigned int eRet, FLV_TAG_INFO flvTagInfo, vector<STREAM_SLICE_INFO> streamInfo, FLV_STAT_INFO statInfo, MEDIA_INFO mediaInfo, string fileInfo) {
	Json::Value root;
	// errono
	root["errno"] = eRet;

	// error_info
	Json::Value error_info;
	error_info["fps"] = statInfo.avFps;
	Json::Value lag;
	Json::Value videolag;
	for (vector<uint32_t>::iterator it = statInfo.vAbnormal.begin(); it != statInfo.vAbnormal.end(); ++it) {
		if (*it > 250) {
			videolag.append(*it);
		}
	}
	Json::Value audiolag;
	for (vector<uint32_t>::iterator it = statInfo.aAbnormal.begin(); it != statInfo.aAbnormal.end(); ++it) {
		if (*it > 150) {
			audiolag.append(*it);
		}
	}
	lag["videolag"] = videolag;
	lag["audiolag"] = audiolag;
	error_info["lag"] =  lag;
	Json::Value sync;
	Json::Value video_ahead;
	int video_ahead_num = statInfo.syncInfo.videoLeadNum;
	if (video_ahead_num > 0) {
		video_ahead["video_ahead_num"] = video_ahead_num;
		video_ahead["video_ahead_time"] = statInfo.syncInfo.videoLeadTimeAv / video_ahead_num;
	}
	Json::Value audio_ahead;
	int audio_ahead_num = statInfo.syncInfo.audioLeadNum;
	if (audio_ahead_num > 0) {
		audio_ahead["audio_ahead_num"] = audio_ahead_num;
		audio_ahead["audio_ahead_time"] = statInfo.syncInfo.audioLeadTimeAv / audio_ahead_num;
	}
	sync["video_ahead"] = video_ahead;
	sync["audio_ahead"] = audio_ahead;
	error_info["sync"] =  sync;
	root["error_info"] = error_info;

	// media_info
	Json::Value media_info;
	media_info["size"] = flvTagInfo.size;
	media_info["length"] = flvTagInfo.length / 1000;
	media_info["mux_form"] = "flv";
	media_info["encode_form"] = "H.264";
	media_info["pic_size"] = mediaInfo.pic_size;
	Json::Value fps;
	fps["fps_real"] = statInfo.exFps;
	fps["fps_avg"] = statInfo.avFps;
	media_info["fps"]  = fps;
	if (flvTagInfo.length / 1000 != 0) {
        media_info["bitrate"]  = (flvTagInfo.size + 9) / (flvTagInfo.length / 1000) * 8 / 1024;
	}else {
        media_info["bitrate"] = 0;
	}
	media_info["video_format"] = mediaInfo.video_format;
	media_info["stream_type"] = mediaInfo.stream_type;
	media_info["encoding_type"] = mediaInfo.encoding_type;
	media_info["file_path"] = fileInfo;
	root["media_info"] = media_info;

	// packet info
	Json::Value packet_info;
	packet_info["videonum"] = statInfo.videoNum;
	packet_info["audionum"] = statInfo.audioNum;
	Json::Value tag_info;
	for(vector<TAG_INFO>::iterator it = flvTagInfo.tagInfo.begin(); it != flvTagInfo.tagInfo.end(); ++it) {
		Json::Value packetItem;
		packetItem["tagtype"] = (*it).tagtype;
		packetItem["tagsize"] = (*it).tagsize;
		packetItem["tagstamp"] = (*it).timestamp;
		packetItem["tagstamp_sub"] = (*it).timestamp_sub;
		tag_info.append(packetItem);
	}
	packet_info["tag_info"] = tag_info;
	root["packet_info"] = packet_info;

	// frame info
	Json::Value frame_info;
	frame_info["islice"] = statInfo.iFrame;
	frame_info["pslice"] = statInfo.pFrame;
	frame_info["bslice"] = statInfo.bFrame;
	Json::Value slice_info;
	for(vector<STREAM_SLICE_INFO>::iterator it = streamInfo.begin(); it != streamInfo.end(); ++it) {
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

	JSONCPP_STRING result_info = root.toStyledString();
	
	return result_info;
}

JSONCPP_STRING StreamEval(char* RTMP_URL, const char* filePath, char* fileSubName, int model, uint32_t param) {
	unsigned int eRet = STREAM_OK;
	FILE* flvFile = NULL;
	FILE* h264File = NULL;
	FILE* outFile = NULL;
	char flvFileName[100] = {'0'};
	char h264FileName[100] = {'0'};
	char outFileName[100] = {'0'};
	sprintf(flvFileName, "%s%s.flv", filePath, fileSubName);
	sprintf(h264FileName, "%s%s.h264", filePath, fileSubName);
	sprintf(outFileName, "%s%s.result", filePath, fileSubName);
	JSONCPP_STRING mediaEvalInfo;
	
	do {
		// 靠靠
		if (2 == model) {
			curl_global_init(CURL_GLOBAL_DEFAULT);
      		CURL* curl = curl_easy_init();
			int length = 0;
			char* decode_url = curl_easy_unescape(curl, RTMP_URL, string(RTMP_URL).length(), &length);
			flvFile = fopen(decode_url, "rb+");
			curl_free(decode_url);
			curl_easy_cleanup(curl);
			curl_global_cleanup();
			if (!flvFile) {
				eRet = FILE_OPEN_FAILED;
				break;
			}
		}else {
			flvFile = fopen(flvFileName, "wb+");
			if (!flvFile) {
				eRet = FILE_OPEN_FAILED;
				break;
			}
			if (string::npos != string(RTMP_URL).find("rtmp")) {
				eRet = RTMPStreamToFlv(flvFile, RTMP_URL, model, param);
			}else {
				eRet = HTTPStreamToFlv(flvFile, RTMP_URL, model, param);
			}
		
		}
			
		outFile = freopen(outFileName, "w+", stdout);
		if (!outFile) {
			eRet = FILE_OPEN_FAILED;
			break;
		}
		
		if (STREAM_OK != eRet) {
			printf("HTTP/RTMP Stream download Failed: %s\n", GetErrorMsg(eRet).c_str());
			break;
		}
		
		//校验flv报文格式，并提取h264bitstream
		fseek(flvFile, 0, SEEK_END);
		long flvSize = ftell(flvFile);
		long errPos = 0;
		fseek(flvFile, 0, SEEK_SET);

		h264File = fopen(h264FileName, "wb+");
		if (!h264File) {
			eRet = FILE_OPEN_FAILED;
			break;
		}
#if 1
		eRet = CheckFlvFile(flvFile, &h264File);
		if (STREAM_OK != eRet) {
			errPos = ftell(flvFile);
			printf("The Stream Packet Formated Flv Failed: %s\n", GetErrorMsg(eRet).c_str());
			printf("And Error Occured at %5d , %5d Byte left\n", errPos, flvSize - errPos);
		}
#endif

#if 1	
		//解析h264bitstream
		fseek(h264File, 0, SEEK_SET);
		eRet = CheckBitStream(h264File);
		if (STREAM_OK != eRet) {
			printf("The H264/H265 StreamData Formated Failed: %s\n", GetErrorMsg(eRet).c_str());
		}
#endif

		//校验视频文件的统计信息
		FLV_STAT_INFO statInfo;
		if (STREAM_OK == eRet) {
			eRet |= CheckFlvDataInfo(statInfo);
			eRet |= CheckStreamDataInfo(statInfo.frameAbnormal);
			GetVideoStreamInfo(statInfo);

			PrintStaticInfo(&statInfo);
			if (STREAM_OK != eRet) {
				printf("\n");
				printf("The Statistic Information of Stream Got an Exception: %s\n", GetErrorMsg(eRet).c_str());
			}
		}

		FLV_TAG_INFO flvTagInfo = GetFlvTagInfo();
		vector<STREAM_SLICE_INFO> streamInfo = GetStreamInfo();
		MEDIA_INFO mediaInfo = GetMediaInfo();
		mediaEvalInfo = GenMediaInfo(eRet, flvTagInfo, streamInfo, statInfo, mediaInfo, flvFileName);
	}while(false);

	//清理资源
	if (flvFile) {
		fclose(flvFile);
		flvFile = NULL;
	}
	if (h264File) {
		fclose(h264File);
		h264File = NULL;
	}
	if (outFile) {
		fflush(outFile);
		fclose(outFile);
		outFile = NULL;
	}

	return mediaEvalInfo;
}
