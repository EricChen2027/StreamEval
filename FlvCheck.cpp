//
//  FlvCheck.cpp
//  streamEval
//
//  Created by chenyu on 17/4/18.
//  Copyright © 2017年 chenyu. All rights reserved.
//
#include <string.h>
#include <math.h>

#include "FlvCheck.hpp"

unsigned int ParseFlvFile(FILE* flvFile, FILE** H264file, FLV_HEADER& header, vector<FLV_TAG>& vFlvTag) {
    if (NULL == flvFile || NULL == *H264file) {
        return FILE_NOT_EXSIT;
    }
    //empty file
    fseek(flvFile, 0, SEEK_END);
    if (ftell(flvFile) <= 0) {
        return STREAM_OK;
    }
    
    //parse file
    fseek(flvFile, 0, SEEK_SET);
    unsigned int iRet = ParseHead(flvFile, header);
    if (STREAM_OK == iRet) {
        //不包含header,seek back
        if ((header.Signature[2] != (uint32_t)'f' && header.Signature[2] != (uint32_t)'F') ||
            (header.Signature[1] != (uint32_t)'l' && header.Signature[1] != (uint32_t)'L') ||
            (header.Signature[0] != (uint32_t)'v' && header.Signature[0] != (uint32_t)'V')) {
            
            //seek back
            fseek(flvFile, 0, SEEK_SET);
        }
        
        vFlvTag.clear();
        iRet = ParseBody(flvFile, H264file, vFlvTag);
    }
    
    return iRet;
}


unsigned int ParseFlvFile(void* buffer, long length, FILE** H264file, FLV_HEADER& header, vector<FLV_TAG>& vFlvTag, long& bufferRead) {
	if (NULL == *H264file) {
		return FILE_NOT_EXSIT;
	}
	//empty buffer
	if (NULL == buffer || 0 == length) {
		return STREAM_OK;
	}
	
	//parse file(maybe contains no header)
	unsigned int iRet = ParseHead(buffer, length, header, bufferRead);
	if (STREAM_OK == iRet) {
		vFlvTag.clear();
		iRet = ParseBody(buffer, length, H264file, vFlvTag, bufferRead);
	}else if (FLV_HEADER_ERROR == iRet) {
		memset(&header, 0, sizeof(header));
		vFlvTag.clear();
		iRet = ParseBody(buffer, length, H264file, vFlvTag, bufferRead);
	}
	
    return iRet;
}

unsigned int ParseHead(FILE* flvFile, FLV_HEADER& header) {
    if (NULL == flvFile) {
        return FILE_NOT_EXSIT;
    }
   
    //READ BUFFER
	//“FLV”
	uint32_t fileType;
	if (!ReadU24(&fileType, flvFile) || ((fileType != (uint32_t)'flv') && fileType != (uint32_t)'FLV')) {
		return FLV_HEADER_ERROR;
	}
	//FLV version
	uint32_t flvVer;
	if (!ReadU8(&flvVer, flvFile)) {
		return FLV_HEADER_ERROR;
	}
	//dose body have video or audio 
	uint32_t bodyType = 0;
	if (!ReadU8(&bodyType, flvFile)) {
		return FLV_HEADER_ERROR;
	}
	//check head length
	uint32_t headLength;
	if (!ReadU32(&headLength, flvFile) || headLength != 9) {
		return FLV_HEADER_ERROR;
	}
	memcpy(header.Signature, &fileType, 3);
	header.Version = flvVer;
	header.Flags = bodyType;
	header.DataOffset = headLength;
    
	return STREAM_OK;
}

unsigned int ParseHead(void* buffer, long length, FLV_HEADER& header, long& bufferRead) {
	if (NULL == buffer || 0 == length) {
		return DATA_BUFFER_NULL;
	}

	//READ BUFFER
	//“FLV”
	uint32_t fileType;
	memcpy(&header, buffer, sizeof(FLV_HEADER));
	if ((header.Signature[0] != 'f' && header.Signature[0] != 'F') ||
		(header.Signature[1] != 'l' && header.Signature[1] != 'L') ||
		(header.Signature[2] != 'v' && header.Signature[2] != 'V') ||
		(header.DataOffset != 9)) {
		return FLV_HEADER_ERROR;
	}
	bufferRead = 9;

	return STREAM_OK;
}

unsigned int ParseBody(FILE* flvFile, FILE** H264file, vector<FLV_TAG>& vFlvTag) {
    if (NULL == flvFile || NULL == *H264file) {
        return FILE_NOT_EXSIT;
    }
    
    unsigned int iRet = STREAM_OK;
    FLV_TAG tagItem;
	uint32_t preTagSize = 0;
	uint32_t dataSize = 0;

    //jump over previousTagSize0
    fseek(flvFile, 4, SEEK_CUR);
	int count = 0;
    while(true) {
        //end
        if (feof(flvFile)) {
            break;
        }
        
        //read a single tag 
		memset(&tagItem, 0, sizeof(tagItem));
        if (!ReadU8((uint32_t*)&tagItem.TagType, flvFile)) {
			if (feof(flvFile)) {
				break;
			}
            iRet = FILE_READ_ERROR;
            break;
        }
        //tagtype == 8||9||18
        if (TAG_TYPE_AUDIO != tagItem.TagType &&
            TAG_TYPE_VIDEO != tagItem.TagType &&
            TAG_TYPE_SCRIPT != tagItem.TagType) {
            iRet = FLV_VIDEOTAG_UNKNOWN;
            break;
        }
		count++;
		//data size
        if (!ReadU24((uint32_t*)&tagItem.DataSize, flvFile)) {
            iRet = FILE_READ_ERROR;
            break;
        }
        //timestamp
        if (!ReadU24((uint32_t*)&tagItem.Timestamp, flvFile)) {
            iRet = FILE_READ_ERROR;
            break;
        }
        if (!ReadU8((uint32_t*)&tagItem.TimestampExt, flvFile)) {
            iRet = FILE_READ_ERROR;
            break;
        }
        //streamid
        if (!ReadU24((uint32_t*)&tagItem.StreamID, flvFile)) {
            iRet = FILE_READ_ERROR;
            break;
        }
        
        //push back
        vFlvTag.push_back(tagItem);
        
        //jump over non_audio and non_video frame
        //jump over next previousTagSizen at the same time
		//small end
		dataSize = (tagItem.DataSize[2] << 16 & 0xff0000) | (tagItem.DataSize[1] << 8 & 0xff00) | (tagItem.DataSize[0] & 0xff);
        if (tagItem.TagType != TAG_TYPE_AUDIO &&
            tagItem.TagType != TAG_TYPE_VIDEO) {
            fseek(flvFile, dataSize, SEEK_CUR);
            fseek(flvFile, 4, SEEK_CUR);
            continue;
        }
        //decode
        iRet = ParseBodyTag(flvFile, H264file, tagItem.TagType, dataSize);
        if (STREAM_OK != iRet) {
            break;
        }
        
        //check previousTagSizeN        
        if (!ReadU32(&preTagSize, flvFile)) {
            if (!feof(flvFile)) {
                iRet = FILE_READ_ERROR;
            }
            break;
        }else if(preTagSize != (dataSize + 11)){
            iRet = FLV_TAGSIZE_NOTMATCH;
            break;
        }
	 }

    return iRet;
}
unsigned int ParseBody(void* buffer, long length, FILE** H264file, vector<FLV_TAG>& vFlvTag, long& bufferRead) {
	if (NULL == buffer || 0 == length) {
		return DATA_BUFFER_NULL;
	}
	if (NULL == *H264file) {
		return FILE_NOT_EXSIT;
	}

	unsigned int iRet = STREAM_OK;
	FLV_TAG tagItem;
	uint32_t preTagSize = 0;
	uint32_t dataSize = 0;

	//jump over previousTagSize0
	bufferRead += 4;
	int count = 0;
	int sizeOfTag = 11;			//basic tag info size
	byte* pointer = (byte*)buffer + bufferRead;
	while(bufferRead + sizeOfTag < length) {
		//read a single tag 
		memset(&tagItem, 0, sizeof(tagItem));
		memcpy(&tagItem.TagType, pointer, 1);
		bufferRead += 1;
		pointer += 1;

		memcpy(&tagItem.DataSize, pointer, 3);
		byte temp = tagItem.DataSize[0];
		tagItem.DataSize[0] = tagItem.DataSize[2];
		tagItem.DataSize[2] = temp;
		bufferRead += 3;
		pointer += 3;

		memcpy(&tagItem.Timestamp, pointer, 3);
		temp = tagItem.Timestamp[0];
		tagItem.Timestamp[0] = tagItem.Timestamp[2];
		tagItem.Timestamp[2] = temp;
		bufferRead += 3;
		pointer += 3;

		memcpy(&tagItem.TimestampExt, pointer, 1);
		bufferRead += 1;
		pointer += 1;
		memcpy(&tagItem.StreamID, pointer, 3);
		temp = tagItem.StreamID[0];
		tagItem.StreamID[0] = tagItem.StreamID[2];
		tagItem.StreamID[2] = temp;
		bufferRead += 3;
		pointer += 3;

		//tagtype == 8||9||18
		uint32_t tagType = tagItem.TagType & 0x1f;
		if (TAG_TYPE_AUDIO != tagType &&
			TAG_TYPE_VIDEO != tagType &&
			TAG_TYPE_SCRIPT != tagType) {
			iRet = FLV_VIDEOTAG_UNKNOWN;
			break;
		}
		count++;
		//push back
		vFlvTag.push_back(tagItem);

		//jump over non_audio and non_video frame
		//jump over next previousTagSizen at the same time
		//big end
		dataSize = (tagItem.DataSize[2] << 16 & 0xff0000) | (tagItem.DataSize[1] << 8 & 0xff00) | (tagItem.DataSize[0] & 0xff);
		if (tagItem.TagType != TAG_TYPE_AUDIO &&
			tagItem.TagType != TAG_TYPE_VIDEO) {
				bufferRead += dataSize;
				pointer += dataSize;

				bufferRead += 4;
				pointer += 4;
				continue;
		}
		//decode
		iRet = ParseBodyTag(pointer, length - bufferRead, H264file, tagItem.TagType, dataSize);
		if (STREAM_OK != iRet) {
			break;
		}
		bufferRead += dataSize;
		pointer += dataSize;

		//check previousTagSizeN
		uint32_t preTagSize = 0;
		memcpy(&preTagSize, pointer, 4);		
		preTagSize = HTON32(preTagSize);		//swap BIG END to Small End
		if (preTagSize != (dataSize + 11)) {
			iRet = FLV_TAGSIZE_NOTMATCH;
			break;
		}
		bufferRead += 4;
		pointer += 4;
	}
	
	return iRet;
}

static int h264space = 0x01000000;
unsigned int ParseBodyTag(FILE* flvFile, FILE** H264File, byte tagType, uint32_t tagSize) {
    if (NULL == flvFile || NULL == *H264File) {
        return FILE_NOT_EXSIT;
    }
    
    unsigned int iRet = STREAM_OK;
    uint32_t frameAndCode = 0;
    uint32_t frameType = 0;
    uint32_t codecID = 0;
    uint32_t AVCPacketType = 0;
    uint32_t compositionTime = 0;
    uint32_t videoInfo = 0;
    void* pBuf = NULL;
    switch(tagType) {
        case TAG_TYPE_AUDIO: {
            pBuf = (void*)malloc(tagSize + 1);
            if (!pBuf) {
                iRet = BUFFER_MALLOC_FAILED;
                break;
            }
            memset(pBuf, 0, tagSize + 1);
            if (!fread(pBuf, 1, tagSize, flvFile)) {
                iRet = BUFFER_READ_ERROR;
                break;
            }

            break;
        }
        case TAG_TYPE_VIDEO:{
            //Video Tag Header
            if (!ReadU8(&frameAndCode, flvFile)){
                iRet = BUFFER_READ_ERROR;
                break;
            }
            frameType = frameAndCode & 0xF0;
            frameType = frameType >> 4;
            codecID = frameAndCode & 0x0F;
            if (!ReadU8(&AVCPacketType, flvFile)) {
                iRet = BUFFER_READ_ERROR;
                break;
            }
            if (!ReadU24(&compositionTime, flvFile)) {
                iRet = BUFFER_READ_ERROR;
                break;
            }
            //Video Tag Body
            if (frameType == 5) {
                if (!ReadU8(&videoInfo, flvFile)) {
                    iRet = BUFFER_READ_ERROR;
                    break;
                }
                if (tagSize != (1+1+3+1)) {
                    iRet = FLV_VIDEOTAGSIZE_NOTMATCH;
                }
            }else {
                switch(codecID) {
                    case 7: {
                        uint32_t templength = 0;
                        char* tempbuff = NULL;
                        if (AVCPacketType == 0) {
                            fseek(flvFile, 6,SEEK_CUR);
                            ReadU16(&templength, flvFile);
                            
                            tempbuff = (char*)malloc(templength + 1);
							memset(tempbuff, 0, templength + 1);
                            fread(tempbuff, 1, templength, flvFile);
                            fwrite(&h264space, 1, 4, *H264File);
                            fwrite(tempbuff, 1, templength, *H264File);
                            free(tempbuff);
							tempbuff = NULL;
                            
                            ReadU8(&templength, flvFile);//ppsnum
                            ReadU16(&templength, flvFile);//ppssize
                            //printf("	ppsize:%d\n", templength);
                            
                            tempbuff = (char*)malloc(templength + 1);
							memset(tempbuff, 0, templength + 1);
                            fread(tempbuff, 1, templength, flvFile);
                            fwrite(&h264space, 1, 4, *H264File);
                            fwrite(tempbuff, 1, templength, *H264File);
                            free(tempbuff);
							tempbuff = NULL;
                        }else {
                            //printf("	One or more NALUs\n");
                            //AVC NALU || AVC end of sequnce
                            int countsize = 2 + 3;
                            while (countsize < tagSize)
                            {
                                ReadU32(&templength, flvFile);
                                tempbuff = (char*)malloc(templength + 1);
								memset(tempbuff, 0, templength + 1);
                                fread(tempbuff, 1, templength, flvFile);
                                fwrite(&h264space, 1, 4, *H264File);
                                fwrite(tempbuff, 1, templength, *H264File);
                                free(tempbuff);
								tempbuff = NULL;
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
        case TAG_TYPE_SCRIPT: {
            break;
        }
        default: {
            break;
        }
    }
    
    //FREE BUFFER
    if (pBuf) {
        free(pBuf);
        pBuf = NULL;
    }
    
    return iRet;
}
unsigned int ParseBodyTag(byte* buffer, long length, FILE** H264File, byte tagType, uint32_t tagSize) {
	if (NULL == *H264File) {
		return FILE_NOT_EXSIT;
	}
	if (NULL == buffer || 0 == length) {
		return DATA_BUFFER_NULL;
	}
	if (length < tagSize) {
		return FLV_VIDEOTAG_UNKNOWN;
	}
	
	unsigned int iRet = STREAM_OK;
	uint32_t frameAndCode = 0;
	uint32_t frameType = 0;
	uint32_t codecID = 0;
	uint32_t AVCPacketType = 0;
	uint32_t compositionTime = 0;
	uint32_t videoInfo = 0;
	void* pBuf = NULL;
	switch(tagType) {
		case TAG_TYPE_AUDIO: {
			pBuf = (void*)malloc(tagSize + 1);
			if (!pBuf) {
				iRet = BUFFER_MALLOC_FAILED;
				break;
			}
			memset(pBuf, 0, tagSize + 1);
			memcpy(pBuf, buffer, tagSize);

			break;
		}
		case TAG_TYPE_VIDEO:{
			//Video Tag Header
			memcpy(&frameAndCode, buffer, 1);
			frameType = frameAndCode & 0xF0;
			frameType = frameType >> 4;
			codecID = frameAndCode & 0x0F;
			buffer += 1;

			memcpy(&AVCPacketType, buffer, 1);
			buffer += 1;
			
			memcpy(&compositionTime, buffer, 3);
			buffer += 3;

			//Video Tag Body
			if (frameType == 5) {
				memcpy(&videoInfo, buffer, 1);
				buffer += 1;
				if (tagSize != (1+1+3+1)) {
					iRet |= FLV_VIDEOTAGSIZE_NOTMATCH;
				}
			}else {
				switch(codecID) {
					case 7: {
						uint32_t templength = 0;
						char* tempbuff = NULL;
						if (AVCPacketType == 0) {
							buffer += 6;				//?wangle
							
							memcpy(&templength, buffer, 2);
							templength = HTON16(templength);		//swap big end to small end
							buffer += 2;
							
							tempbuff = (char*)malloc(templength + 1);
							memset(tempbuff, 0, templength + 1);
							memcpy(tempbuff, buffer, templength);					
							fwrite(&h264space, 1, 4, *H264File);
							fwrite(tempbuff, 1, templength, *H264File);
							free(tempbuff);
							buffer += templength;

							memcpy(&templength, buffer, 1);	//ppsnum
							buffer += 1;
							memcpy(&templength, buffer, 2);	//ppssize
							templength = HTON16(templength);
							buffer += 2;

							tempbuff = (char*)malloc(templength + 1);
							memset(tempbuff, 0, templength + 1);
							memcpy(tempbuff, buffer, templength);
							fwrite(&h264space, 1, 4, *H264File);
							fwrite(tempbuff, 1, templength, *H264File);
							free(tempbuff);
							buffer += templength;
						}else {
							//printf("	One or more NALUs\n");
							//AVC NALU || AVC end of sequnce
							int countsize = 2 + 3;
							while (countsize < tagSize)
							{
								memcpy(&templength, buffer, 4);
								templength = HTON32(templength);		//swap big end to small end
								buffer += 4;
								
								tempbuff = (char*)malloc(templength + 1);
								memset(tempbuff, 0, templength + 1);
								memcpy(tempbuff, buffer, templength);
								fwrite(&h264space, 1, 4, *H264File);
								fwrite(tempbuff, 1, templength, *H264File);
								free(tempbuff);
								buffer += templength;

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
		case TAG_TYPE_SCRIPT: {
			break;
		}
		default: {
			break;
		}
	}

	//FREE BUFFER
	if (pBuf) {
		free(pBuf);
		pBuf = NULL;
	}

	return iRet;
}
unsigned int CheckFlvData(vector<TAG_INFO>& flvTagInfo, STATIC_INFO& statInfo, int model) {
    unsigned int iRet = STREAM_OK;
    uint32_t audioLastTimeStamp = 0;
    uint32_t videoLastTimeStamp = 0;
	uint32_t scriptLastTimeStamp = 0;
	uint32_t audioFirstDatatime = 0;
	uint32_t videoFirstDatatime = 0;
    bool bFirstAudioTag = true;
    bool bFirstVideoTag = true;
    uint32_t videoLength = 0;
    
    if (0 == model) {
        for (vector<TAG_INFO>::iterator it = flvTagInfo.begin(); it != flvTagInfo.end(); ++it) {
            statInfo.mediaSize += (*it).tagsize;
            
			//Any action, reset the status
			if (ACTION_NORMAL != (*it).action) {
				bFirstAudioTag = true;
				bFirstVideoTag = true;
			}

            switch((*it).tagtype) {
                case TAG_TYPE_AUDIO: {
					statInfo.audioTagNum++;
					//get the first effective audio tag
					if (bFirstAudioTag) {
						if ((*it).tagsize >= MIN_FLV_TAGSIZE && (*it).timestamp >= scriptLastTimeStamp) {
							bFirstAudioTag = false;
							audioLastTimeStamp = (*it).timestamp;
						}					
						break;
					}

					//calculate the timestamp sub and stat the abnormal tag
					int32_t timeStampSub = (int32_t)((*it).timestamp - audioLastTimeStamp);
					audioLastTimeStamp = (*it).timestamp;

					(*it).timestamp_sub = timeStampSub;
					statInfo.mediaLength += timeStampSub;
					
					if(AUDIO_STUTTER_NUM <= timeStampSub) {
						statInfo.aAbnormal.push_back(timeStampSub);
					}else if (0 > timeStampSub) {
						statInfo.aTransmitAb.push_back(timeStampSub);
					}
					if (0 != (*it).timestamp && videoLastTimeStamp > STUTTER_TIME) {
						if ((*it).timestamp <= videoLastTimeStamp - STUTTER_TIME) {
							statInfo.videoOnly++;
						}
					}

					//set the tag which is very small as a invalid tag
                    if ((*it).tagsize < MIN_FLV_TAGSIZE) {
                        if (!bFirstAudioTag) {
                            statInfo.aInvalid.push_back(*it);
                        }
                    }
                    break;
                }
                case TAG_TYPE_VIDEO: {
					statInfo.videoTagNum++;
					//jump over the first tag
					if(bFirstVideoTag) {
						if ((*it).tagsize >= MIN_FLV_TAGSIZE && (*it).timestamp >= scriptLastTimeStamp) {
							bFirstVideoTag = false;
							videoLastTimeStamp = (*it).timestamp;
						}
						break;
					}

					//calculate the tag sub, and record the latest video tag timestamp
					int32_t timeStampSub = (int32_t)((*it).timestamp - videoLastTimeStamp);
					videoLastTimeStamp = (*it).timestamp;

					(*it).timestamp_sub = timeStampSub;
					//calculate the length of the video that could play 
					videoLength += timeStampSub;
					//record video timestampsub which is abnormal
					if(VIDEO_STUTTER_NUM <= timeStampSub) {
						statInfo.vAbnormal.push_back(timeStampSub);
					}else if (0 > timeStampSub) {
						statInfo.vTransmitAb.push_back(timeStampSub);
					}
					//calculate sync num(use video tag timestamp as stand)
					if (0 != (*it).timestamp && audioLastTimeStamp > STUTTER_TIME) {
						if ((*it).timestamp <= audioLastTimeStamp - STUTTER_TIME) {
							statInfo.audioOnly++;
						}
					}

                    //data tag only
                    if ((*it).tagsize < MIN_FLV_TAGSIZE) {    
						//the first tag contains some desc msg, it's normal that less than 80 byte
						if (!bFirstVideoTag) {
							statInfo.vInvalid.push_back(*it);
						}
                    }

                    break;
                }
                case TAG_TYPE_SCRIPT: {
					//reset the status
					scriptLastTimeStamp = (*it).timestamp;
					bFirstAudioTag = true;
					bFirstVideoTag = true;
                    break;
                }
                default: {
                    iRet |= FLV_VIDEOTAG_UNKNOWN;
                    break;
                }
            }
        }
        
        //calculate fps
		//use videoLength in case of no valid audio tag 
        if (0 != statInfo.mediaLength) {
            statInfo.avFPS = statInfo.videoTagNum * 1000 / statInfo.mediaLength;
        }else if (0 != videoLength) {
			statInfo.avFPS = statInfo.videoTagNum * 1000 / videoLength;
			statInfo.mediaLength = videoLength;
		}
		if (0 != videoLength) {
			statInfo.realFPS = statInfo.videoTagNum * 1000 / videoLength;
		}
    }else {
		uint32_t videoDatatimeTotal = 0;
		uint32_t audioDatatimeTotal = 0;
		for (vector<TAG_INFO>::iterator it = flvTagInfo.begin(); it != flvTagInfo.end(); ++it) {
			statInfo.mediaSize += (*it).tagsize;

			//Any action, reset the status
			if (ACTION_NORMAL != (*it).action) {
				bFirstAudioTag = true;
				bFirstVideoTag = true;
			}

			switch((*it).tagtype) {
				case TAG_TYPE_AUDIO: {
					statInfo.audioTagNum++;
					//find the first effective tag
					if (bFirstAudioTag) {
						if ((*it).tagsize >= MIN_FLV_TAGSIZE && (*it).timestamp >= scriptLastTimeStamp) {
							bFirstAudioTag = false;
							audioLastTimeStamp = (*it).timestamp;
							audioFirstDatatime = (*it).datatime;		//used for dynamic check
						}
						break;
					}

					//calculate the stat info
					int32_t timeStampSub = (int32_t)((*it).timestamp - audioLastTimeStamp);
					audioLastTimeStamp = (*it).timestamp;

					(*it).timestamp_sub = timeStampSub;
					statInfo.mediaLength += timeStampSub;
					int32_t dataTimeSub = (int32_t)((*it).datatime - audioFirstDatatime);
					audioDatatimeTotal += dataTimeSub;	//used for dynamic check

					if(AUDIO_STUTTER_NUM <= timeStampSub) {
						statInfo.aAbnormal.push_back(timeStampSub);
					}else if (0 > timeStampSub) {
						statInfo.aTransmitAb.push_back(timeStampSub);
					}
					if (0 != (*it).timestamp && videoLastTimeStamp > STUTTER_TIME) {
						if ((*it).timestamp <= videoLastTimeStamp - STUTTER_TIME) {
							statInfo.videoOnly++;
						}
					}
					if (audioDatatimeTotal > statInfo.mediaLength + AUDIO_STUTTER_NUM) {	//used for dynamic check
						statInfo.aDynamicAb.push_back(audioDatatimeTotal - statInfo.mediaLength);
					}

					//find the invalid tag(may be not)
					if ((*it).tagsize < MIN_FLV_TAGSIZE) {
						if (!bFirstAudioTag) {
							statInfo.aInvalid.push_back(*it);
						}
					}

					break;
				}
				case TAG_TYPE_VIDEO: {
					statInfo.videoTagNum++;

					//jump over the first tag
					if(bFirstVideoTag) {
						if ((*it).tagsize >= MIN_FLV_TAGSIZE && (*it).timestamp >= scriptLastTimeStamp) {
							bFirstVideoTag = false;
							videoLastTimeStamp = (*it).timestamp;
							videoFirstDatatime = (*it).datatime;		//used for dynamic check
						}
						break;
					}
					//calculate the tag sub, and record the latest video tag timestamp
					int32_t timeStampSub = (int32_t)((*it).timestamp - videoLastTimeStamp);
					videoLastTimeStamp = (*it).timestamp;

					(*it).timestamp_sub = timeStampSub;
					//calculate the length of the video that could play 
					videoLength += timeStampSub;
					int32_t dataTimeSub = (int32_t)((*it).datatime - videoFirstDatatime);
					videoDatatimeTotal += dataTimeSub;	//used for dynamic check

					//record video timestampsub which is abnormal
					if(VIDEO_STUTTER_NUM <= timeStampSub) {
						statInfo.vAbnormal.push_back(timeStampSub);
					}else if (0 > timeStampSub) {
						statInfo.vTransmitAb.push_back(timeStampSub);
					}
					//calculate sync num(use video tag timestamp as stand)
					if (0 != (*it).timestamp && audioLastTimeStamp > STUTTER_TIME) {
						if ((*it).timestamp <= audioLastTimeStamp - STUTTER_TIME) {
							statInfo.audioOnly++;
						}
					}
					if (videoDatatimeTotal > videoLength + VIDEO_STUTTER_NUM) {	//used for dynamic check
						statInfo.vDynamicAb.push_back(videoDatatimeTotal - videoLength);
					}

					//data tag only
					if ((*it).tagsize < MIN_FLV_TAGSIZE) {                    
						//the first tag contains some desc msg, it's normal that less than 80 byte
						if (!bFirstVideoTag) {
							statInfo.vInvalid.push_back(*it);
						}
					}
					break;
				}
				case TAG_TYPE_SCRIPT: {
					//reset the status
					scriptLastTimeStamp = (*it).timestamp;
					bFirstAudioTag = true;
					bFirstVideoTag = true;
					break;
				}
				default: {
					iRet |= FLV_VIDEOTAG_UNKNOWN;
					break;
				}
			}
		}

		//calculate fps
		if (0 != statInfo.mediaLength) {
			statInfo.avFPS = statInfo.videoTagNum * 1000 / statInfo.mediaLength;
		}else if (0 != videoLength) {
		    statInfo.avFPS = statInfo.videoTagNum * 1000 / videoLength;
			statInfo.mediaLength = videoLength;
		}
		if (0 != videoLength) {
			statInfo.realFPS = statInfo.videoTagNum * 1000 / videoLength;
		}
    }
    
    //video and audio not sync
	if (statInfo.videoOnly > 0) {
		iRet |= FLV_VIDEO_ONLY;
	}
	if (statInfo.audioOnly > 0) {
		iRet |= FLV_AUDIO_ONLY;
	}

    //video lag
    if (statInfo.vAbnormal.size() > 0) {
        iRet |= FLV_VIDEOSTAMP_EXCEPTION;
    }
    //audio lag
    if (statInfo.aAbnormal.size() > 0) {
        iRet |= FLV_AUDIOSTAMP_EXCEPTION;
    }
    //frame too low
    if (statInfo.avFPS < STUTTER_FPS) {
        iRet |= FLV_FPS_TOOLOW;
    }
    
    return iRet;
}

/**
 * @将从报文中解析出来的结构转化为便于阅读的结构，并记录flvTag下载当前的状态(只记录第一个tag的状态)
 *
 */
void FormatFlvTag2TagInfo(vector<FLV_TAG> vFlvTag, vector<TAG_INFO>& vTagInfo, STREAM_ACTION action, uint32_t action_time) {
    TAG_INFO tagInfoItem;
	bool bRecord = false;
	int size = vTagInfo.capacity();
	for(vector<FLV_TAG>::iterator it = vFlvTag.begin(); it != vFlvTag.end(); it++) {
		tagInfoItem.clear();
        tagInfoItem.datatime = clock();
        tagInfoItem.tagtype = (uint32_t)(*it).TagType;
        uint32_t tagSize = ((*it).DataSize[2] << 16 & 0xff0000) | ((*it).DataSize[1] << 8 & 0xff00) | ((*it).DataSize[0] & 0xff);
        tagInfoItem.tagsize = tagSize;
        uint32_t timestamp = ((*it).TimestampExt << 24 & 0xff000000) | 
							 ((*it).Timestamp[2] << 16 & 0xff0000) |
							 ((*it).Timestamp[1] << 8 & 0xff00) |
							 (*it).Timestamp[0];
        tagInfoItem.timestamp = timestamp;
		tagInfoItem.timestampSrc = ((*it).Timestamp[2] << 16 & 0xff0000) |
								   ((*it).Timestamp[1] << 8 & 0xff00) |
								   (*it).Timestamp[0];
		tagInfoItem.timestampExt = (*it).TimestampExt;
		tagInfoItem.streamID = ((*it).StreamID[2] << 16 &0xff0000) |
							   ((*it).StreamID[1] << 8 & 0xff00) |
							   (*it).StreamID[0];

		if (ACTION_NORMAL != action && !bRecord) {
			tagInfoItem.action = action;
			tagInfoItem.action_time = action_time;
			bRecord = true;
		}      
        
        vTagInfo.push_back(tagInfoItem);
		size = vTagInfo.capacity();
		int test = 0;
    }
    return;
}
