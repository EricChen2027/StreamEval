#include "FlvCheck.h"
#include "pauta.h"

/**
  *	@ 校验Flv包头
  *	@ in	FILE*	flvFile
  * @ inout File**	h264file
  * 
  * @ return check result
  */
unsigned int CheckFlvFile(FILE* flvFile, FILE** h264File){
	//clear the statinfo;
	flvTagInfo.clear();
		
	fseek(flvFile, 0, SEEK_END);
	if (ftell(flvFile) <= 0) {
		return STREAM_OK;
	}
	fseek(flvFile, 0, SEEK_SET);

	videoNum = 0;
	audioNum = 0;

	uint32_t bodyType = 0;
	unsigned int eRet = STREAM_OK;
	eRet = CheckHead(flvFile, bodyType);

	if (STREAM_OK == eRet) {
		eRet = CheckBody(flvFile, h264File, bodyType);
	}

	return eRet;
}
/**
  *	@ 校验Flv包头
  * @ in	FILE*		flvFile
  * @ inout uint32_t	bodyType
  * 
  * @ return check result
  */
unsigned int CheckHead(FILE* flvFile, uint32_t& bodyType) {
	//校验“FLV”字段
	uint32_t fileType;
	if (!ReadU24(&fileType, flvFile) || ((fileType != (uint32_t)'flv') && fileType != (uint32_t)'FLV')) {
		return FLV_HEADER_ERROR;
	}
	//获取FLV版本号
	uint32_t flvVer;
	if (!ReadU8(&flvVer, flvFile)) {
		return FLV_HEADER_ERROR;
	}
	//获取body内容是否包含音、视频
	if (!ReadU8(&bodyType, flvFile)) {
		return FLV_HEADER_ERROR;
	}
	//校验head长度
	uint32_t headLength;
	if (!ReadU32(&headLength, flvFile) || headLength != 9) {
		return FLV_HEADER_ERROR;
	}

	return STREAM_OK;
}

/**
  *	@ 校验Flv Body
  * @ in	FILE*		flvFile
  * @ in	uint32_t	bodyType(0000 0101:音、视频
  								 0000 0100:音频
  								 0000 0001:视频）
  *
  * @ return check result
  */
unsigned int CheckBody(FILE* flvFile, FILE** h264File, uint32_t bodyType) {
	unsigned int eRet = STREAM_OK;
	if (!flvFile) {
		return FILE_NOT_EXSIT;
	}

	//jump over previousTagSize0
	fseek(flvFile, 4, SEEK_CUR);
	//
	uint32_t type		= 0;
	uint32_t dataLength = 0;
	uint32_t timeStamp	= 0;
	uint32_t timeStampExt = 0;
	uint32_t vTimeStampReal = 0;
	uint32_t aTimeStampReal = 0;
	uint32_t lastVideoTimeStamp = 0;
	uint32_t lastAudioTimeStamp = 0;
	//use to cal video
	uint32_t vValidTimeNum = 0;
	uint32_t vValidTimeSum = 0;
	uint32_t vValidTimeStart = 0;
	vValidTime.clear();
	//used to cal audio
	uint32_t aValidTimeNum = 0;
	uint32_t aValidTimeSum = 0;
	uint32_t aValidTimeStart = 0;
	aValidTime.clear();
	syncInfo.clear();
	//used to cal size(audio + video + script)
	uint32_t mediaSize = 0;

	uint32_t streamid	= 0;
	uint32_t tagNum		= 0;
	void* pbuf = NULL;
	char* realTime = (char*)malloc(20);
	memset(realTime, 0, 20);
	printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
	printf("|                               Flv Tag Info                                         |\n");
	printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
	printf("|TagNum    |TagType   |TagSize    |TimeStamp   |timeStampExt |TimeStampSub |StreamID |\n");
	while(true) {
		if (!ReadU8(&type, flvFile)) {
			if (!feof(flvFile)) {
				eRet = FILE_READ_ERROR;
			}else {
				eRet = STREAM_OK;
			}
			break;
		}
		if (TAG_TYPE_AUDIO != type && TAG_TYPE_VIDEO != type && TAG_TYPE_SCRIPT != type) {
			break;
		}
		if (!ReadU24(&dataLength, flvFile)) {
			if (!feof(flvFile)) {
				eRet = FILE_READ_ERROR;
			}
			break;
		}
		mediaSize += dataLength;
		/*
		if (!ReadTime(&timeStamp, inFile)) {
			eRet = false;
			break;
		}*/
		if (!ReadU24(&timeStamp, flvFile)) {
			if (!feof(flvFile)) {
				eRet = FILE_READ_ERROR;
			}
			break;
		}
		if (!ReadU8(&timeStampExt, flvFile)) {
			if (!feof(flvFile)) {
				eRet = FILE_READ_ERROR;
			}
			break;
		}
		if (!ReadU24(&streamid, flvFile)) {
			if (!feof(flvFile)) {
				eRet = FILE_READ_ERROR;
			}
			break;
		}else if (streamid != 0) {
			eRet = FLV_STREAMID_NOTZERO;
			break;
		}
		tagNum++;
		
		//caculate fps
		if (TAG_TYPE_VIDEO == type) {
			lastVideoTimeStamp = vTimeStampReal;
			vTimeStampReal = (timeStampExt << 24) | timeStamp;
			if (0 == vValidTimeStart){
				vValidTimeStart = vTimeStampReal;
			}else {
				if (0 != vTimeStampReal - lastVideoTimeStamp) {
					vValidTime.push_back(vTimeStampReal - lastVideoTimeStamp);
				}
			}
			if (0 != vTimeStampReal) {
				vValidTimeNum++;
				vValidTimeSum = vTimeStampReal - vValidTimeStart;
				averageTime = vValidTimeSum / vValidTimeNum;
			}

			//计算音、视频同步信息
			if (0 != vTimeStampReal && 0 != lastAudioTimeStamp) {
				if ((int)vTimeStampReal < ((int)lastAudioTimeStamp) - STUTTER_TIME) {
					syncInfo.audioLeadNum++;
					syncInfo.audioLeadTimeAv += abs(int(vTimeStampReal - lastAudioTimeStamp));
				}
			}
		}else if (TAG_TYPE_AUDIO == type) {
			lastAudioTimeStamp = aTimeStampReal;
			aTimeStampReal = (timeStampExt << 24) | timeStamp;
			if (0 == aValidTimeStart){
				aValidTimeStart = aTimeStampReal;
			}else {
				if (0 != aTimeStampReal - lastAudioTimeStamp) {
					aValidTime.push_back(aTimeStampReal - lastAudioTimeStamp);
				}
			}
			if (0 != aTimeStampReal) {
				aValidTimeNum++;
				aValidTimeSum = aTimeStampReal - aValidTimeStart;
			}

			//计算音、视频同步信息
			if (0 != lastVideoTimeStamp && 0 != lastAudioTimeStamp) {
				if ((int)aTimeStampReal < ((int)lastVideoTimeStamp - STUTTER_TIME)) {
					syncInfo.videoLeadNum++;
					syncInfo.videoLeadTimeAv = abs(int(aTimeStampReal - lastVideoTimeStamp));
				}
			}
		}

		TAG_INFO tagInfo;
		tagInfo.tagtype = type;
		tagInfo.tagsize = dataLength;
		if (type == TAG_TYPE_AUDIO) {
			tagInfo.timestamp = aTimeStampReal;
			tagInfo.timestamp_sub = aTimeStampReal - lastAudioTimeStamp;
		}else if (type == TAG_TYPE_VIDEO) {
			tagInfo.timestamp = vTimeStampReal;
			tagInfo.timestamp_sub = vTimeStampReal - lastVideoTimeStamp;
		}else {

		}
		flvTagInfo.tagInfo.push_back(tagInfo);

#ifdef VIDEO_SHOW_ONLY
		if (type == TAG_TYPE_AUDIO) {
#ifdef TIMESTAMP_TO_REALTIME
			//转换timeStamp
			TimeStamp2RealTime(timeStamp, &realTime);
			printf("|%10d|%10d|%11d|%s|%13d|%9d|\n", tagNum, type, dataLength, realTime, timeStampExt, streamid);
#else
			//printf("|%10d|%10d|%11d|%12d|%13d|%13d|%9d|\n", tagNum, type, dataLength, timeStamp, timeStampExt, vTimeStampReal - lastVideoTimeStamp, streamid);
			printf("|%10d|%10d|%11d|%12d|%13d|%13d|%9d|\n", tagNum, type, dataLength, timeStamp, timeStampExt, aTimeStampReal - lastAudioTimeStamp, streamid);
#endif
		}
#else
#ifdef TIMESTAMP_TO_REALTIME
		printf("|%10d|%10d|%11d|%s|%13d|%9d|\n", tagNum, type, dataLength, realTime, timeStampExt, streamid);
#else
		if (type == TAG_TYPE_VIDEO) {
			printf("|%10d|%10d|%11d|%12d|%13d|%13d|%9d|\n", tagNum, type, dataLength, timeStamp, timeStampExt, vTimeStampReal - lastVideoTimeStamp, streamid);
		}else {
			printf("|%10d|%10d|%11d|%12d|%13d|%13d|%9d|\n", tagNum, type, dataLength, timeStamp, timeStampExt, aTimeStampReal - lastAudioTimeStamp, streamid);
		}
		
#endif
#endif
		
		//jump over non_audio and non_video frame，
		//jump over next previousTagSizen at the same time
		if (type != TAG_TYPE_AUDIO && type != TAG_TYPE_VIDEO) {
			fseek(flvFile, dataLength, SEEK_CUR);
			fseek(flvFile, 4, SEEK_CUR);
			continue;
		}

		//校验文件类型
		//暂时不实现

		//校验音、视频数据
		eRet = CheckBodyTag(flvFile, h264File, type, dataLength, videoNum, audioNum);
		if (STREAM_OK != eRet) {
			break;
		}
		
		//check previousTagSizeN
		uint32_t preTagSize = 0;
		if (!ReadU32(&preTagSize, flvFile)) {
			if (!feof(flvFile)) {
				eRet = FILE_READ_ERROR;
			}
			break;
		}else if(preTagSize != (dataLength + 11)){
			eRet = FLV_TAGSIZE_NOTMATCH;
			break;
		}
	}
	flvTagInfo.audioNum = audioNum;
	flvTagInfo.videoNum = videoNum;
	flvTagInfo.length = vValidTimeSum;
	flvTagInfo.size = mediaSize;

	printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");

	if (realTime) {
		free(realTime);
		realTime = NULL;
	}

	return eRet;
}

/**
  *	@ 校验Flv tag
  * @ inout uint32_t bodyType
  * 
  * @ return check result
  */
static int h264space = 0x01000000;//H264内容间隔标识00000001
unsigned int CheckBodyTag(FILE* flvFile, FILE** h264File, uint32_t tagType, uint32_t tagSize, int& videoNum, int& audioNum) {
	unsigned int eRet = STREAM_OK;
	uint32_t frameAndCode = 0;
	uint32_t frameType = 0;
	uint32_t codecID = 0;
	uint32_t AVCPacketType = 0;
	uint32_t compositionTime = 0;
	uint32_t videoInfo = 0;
	void* pBuf = NULL;
	switch(tagType) {
		case TAG_TYPE_AUDIO:{
			audioNum++;
			pBuf = (void*)malloc(tagSize + 1);
			if (!pBuf) {
				eRet = BUFFER_MALLOC_FAILED;
				break;
			}
			memset(pBuf, 0, tagSize + 1);
			if (!fread(pBuf, 1, tagSize, flvFile)) {
				eRet = BUFFER_READ_ERROR;
				break;
			}

			break;
		}
		case TAG_TYPE_VIDEO:{
			//Video Tag Header
			videoNum++;
			if (!ReadU8(&frameAndCode, flvFile)){
				eRet = BUFFER_READ_ERROR;
				break;
			}
			frameType = frameAndCode & 0xF0;
			frameType = frameType >> 4;
			codecID = frameAndCode & 0x0F;
			if (!ReadU8(&AVCPacketType, flvFile)) {
				eRet = BUFFER_READ_ERROR;
				break;
			}
			if (!ReadU24(&compositionTime, flvFile)) {
				eRet = BUFFER_READ_ERROR;
				break;
			}

			//Video Tag Body
			if (frameType == 5) {
				if (!ReadU8(&videoInfo, flvFile)) {
					eRet = BUFFER_READ_ERROR;
					break;
				}
				if (videoInfo == 0) {
					printf("Start of client-side seeking video frame sequence\n");
				}else {
					printf("End of client-side seeking video frame sequence\n");
				}
				if (tagSize != (1+1+3+1)) {
					eRet = FLV_VIDEOTAGSIZE_NOTMATCH;
				}
			}else {
				switch(codecID) {
					//AVC 解码
					case 7: {
						//printf("	AVC VIDEO PACKET\n");
						uint32_t templength = 0;
						char* tempbuff = NULL; 
						if (AVCPacketType == 0) {
							//printf("	AVCDecoderConfigurationRecord\n");
							// AVC sequence header
							fseek(flvFile, 6,SEEK_CUR);
 

							ReadU16(&templength, flvFile);  
							//printf("	sssize:%d\n", templength);  

							tempbuff = (char*)malloc(templength);  
							fread(tempbuff, 1, templength, flvFile);  
							fwrite(&h264space, 1, 4, *h264File);  
							fwrite(tempbuff, 1, templength, *h264File); 
							free(tempbuff);  

							ReadU8(&templength, flvFile);//ppsnum  

							ReadU16(&templength, flvFile);//ppssize  
							//printf("	ppsize:%d\n", templength);  

							tempbuff = (char*)malloc(templength);  
							fread(tempbuff, 1, templength, flvFile);  
							fwrite(&h264space, 1, 4, *h264File);  
							fwrite(tempbuff, 1, templength, *h264File);  
							free(tempbuff);
						}else {
							//printf("	One or more NALUs\n");

							//AVC NALU || AVC end of sequnce
							int countsize = 2 + 3;  
							while (countsize < tagSize)  
							{  
								ReadU32(&templength, flvFile);  
								tempbuff = (char*)malloc(templength);  
								fread(tempbuff, 1, templength, flvFile); 
								fwrite(&h264space, 1, 4, *h264File);  
								fwrite(tempbuff, 1, templength, *h264File);  
								free(tempbuff);  
								countsize += (templength + 4);  
							}  
						}
						
						break;
					}
					default:{
						break;
					}
				}
			}

			break;
		}
		case TAG_TYPE_SCRIPT:
			break;
		default:
			break;
	}

	if (pBuf) {
		free(pBuf);
		pBuf = NULL;
	}
	return eRet;
}

/**
  *	timeStamp转时间
  *
  */
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
unsigned int CheckFlvDataInfo(FLV_STAT_INFO& statInfo) { 
	unsigned int eRet = STREAM_OK;

	//根据videoTag 的stamptime筛选异常值
	int iRet = PautaCheck(vValidTime, statInfo.vAbnormal, 4);
	if (!iRet) {
		for(vector<uint32_t>::iterator it = statInfo.vAbnormal.begin(); it != statInfo.vAbnormal.end(); it++) {
			if (*it > STUTTER_NUM) {
				eRet = FLV_VIDEOSTAMP_EXCEPTION;
				break;
			}
		}
	}
	//根据audioTag 的stamptime筛选异常值
	iRet = PautaCheck(aValidTime, statInfo.aAbnormal);
	if (!iRet) {
		for(vector<uint32_t>::iterator it = statInfo.aAbnormal.begin(); it != statInfo.aAbnormal.end(); it++) {
			if (*it > STUTTER_NUM) {
				eRet |= FLV_AUDIOSTAMP_EXCEPTION;
				break;
			}
		}
	}

	//统计音、视频信息
	statInfo.videoNum = videoNum;
	statInfo.audioNum = audioNum;
	if (0 != averageTime) {
		statInfo.avFps = 1000 / averageTime;
	}else {
		statInfo.avFps = 0;
	}
	if (statInfo.vAbnormal.size() > 0) {
		for(vector<uint32_t>::iterator it = statInfo.vAbnormal.begin(); it != statInfo.vAbnormal.end(); it++) {
			for(vector<uint32_t>::iterator it2 = vValidTime.begin(); it2 != vValidTime.end(); it2++) {
				if (*it == *it2) {
					vValidTime.erase(it2);
					break;
				}
			}
		}
	}
	long videoTimeSum = 0;
	for(vector<uint32_t>::iterator it = vValidTime.begin(); it != vValidTime.end(); it++) {
		videoTimeSum += *it;
	}
	if (videoTimeSum > 0) {
		statInfo.exFps = 1000 * vValidTime.size() / videoTimeSum;
		if (statInfo.exFps < 10) {		//硬写死成10fps
			eRet |= FLV_FPS_TOOLOW;
		}
	}
	//统计音视频同步信息
	statInfo.syncInfo = syncInfo;
	if (0 != syncInfo.audioLeadNum) {
		eRet |= FLV_AUDIO_ONLY;
	}
	if (0 != syncInfo.videoLeadNum) {
		eRet |= FLV_VIDEO_ONLY;
	}

	return eRet;
}

FLV_TAG_INFO GetFlvTagInfo() {
	return flvTagInfo;
}


