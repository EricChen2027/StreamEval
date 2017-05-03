//
//  BitStreamCheck.cpp
//  streamEval
//
//  Created by chenyu on 17/4/18.
//  Copyright © 2017年 chenyu. All rights reserved.
//

#include "BitStreamCheck.hpp"
#include "Common.hpp"

/**
 * @ 将解读出来的nalu数据转换为方便阅读的信息
 *
 */
void CopyNalu2Info(vector<NALU_t> vNalu, vector<STREAM_NALU_INFO>& streamInfo) {
    int m_nSliceIndex = 1;
    for (vector<NALU_t>::iterator it = vNalu.begin(); it != vNalu.end(); ++it) {
        char nalTypeInfo[37] = {0};
        char nalInfo[15] = {0};
        if ((*it).type == 0)         //H264
        {
            switch ((*it).nalType)
            {
                case 0: {
                    sprintf(nalTypeInfo, "Unspecified");
                    break;
                }
                case 1: {
                    sprintf(nalTypeInfo, "Coded slice of a non-IDR picture");       //non IDR
                    switch ((*it).sliceType)
                    {
                        case 0:
                        case 5:
                            sprintf(nalInfo, "P Slice #%d", m_nSliceIndex);
                            break;
                        case 1:
                        case 6:
                            sprintf(nalInfo, "B Slice #%d", m_nSliceIndex);
                            break;
                        case 2:
                        case 7:
                            sprintf(nalInfo, "I Slice #%d", m_nSliceIndex);
                            break;
                        case 3:
                        case 8:
                            sprintf(nalInfo, "SP Slice #%d", m_nSliceIndex);
                            break;
                        case 4:
                        case 9:
                            sprintf(nalInfo, "SI Slice #%d", m_nSliceIndex);
                            break;
                    }
                    m_nSliceIndex++;
                    break;
                }
                case 2: {
                    sprintf(nalTypeInfo, "DPA");       
                    break;
                }
                case 3: {
                    sprintf(nalTypeInfo, "DPB");        
                    break;
                }
                case 4: {
                    sprintf(nalTypeInfo, "DPC");        
                    break;
                }
                case 5: {
                    sprintf(nalTypeInfo, "Coded slice of an IDR picture");
                    sprintf(nalInfo, "IIDR #%d", m_nSliceIndex);
                    m_nSliceIndex++;
                    break;
                }
                case 6: {
                    sprintf(nalTypeInfo, "Supplemental enhancement information");
					sprintf(nalInfo, "SEI");
                    break;
                }
                case 7: {
                    sprintf(nalTypeInfo, "Sequence parameter set");
					sprintf(nalInfo, "SPS");
                    break;
                }
                case 8: {
                    sprintf(nalTypeInfo, "Picture parameter set");
					sprintf(nalInfo, "PPS");
                    break;
                }
                case 9: {
                    sprintf(nalTypeInfo, "Access UD");
                    sprintf(nalInfo, "AUD");
                    break;
                }
                case 10: {
                    sprintf(nalTypeInfo, "END_SEQUENCE");
					break;
                }
                case 11: {
                    sprintf(nalTypeInfo, "END_STREAM");					
					break;
                }
                case 12: {
                    sprintf(nalTypeInfo, "FILLER_DATA");
					break;
                }
                case 13: {
                    sprintf(nalTypeInfo, "SPS_EXT");
					break;
                }
                case 19: {
                    sprintf(nalTypeInfo, "AUXILIARY_SLICE");
					break;
                }
                default: {
                    sprintf(nalTypeInfo, "Other");
					break;
                }
            }
        }else{
            //H265
        }
        
        STREAM_NALU_INFO sliceInfoItem;
        sliceInfoItem.offset = (*it).offset;
        sliceInfoItem.length = (*it).len;
        sliceInfoItem.startcode = (*it).startcodeBuffer;
        sliceInfoItem.nalTypeInfo = nalTypeInfo;
        sliceInfoItem.nalInfo = nalInfo;
        streamInfo.push_back(sliceInfoItem);
    }
    
    return;
}

unsigned int ParseBitStream(FILE* H264File, MEDIA_INFO& media_info, vector<NALU_t>& vNalu) {
    if (NULL == H264File) {
        return FILE_NOT_EXSIT;
    }

    /******************************/
    /****      Parse NALU     *****/
    /******************************/
    h264_stream_t* m_hH264 = h264_new();
    unsigned int iRet = probeNALU(H264File, m_hH264, vNalu, -1);
    videoinfo_t videoInfo;
    memset(&videoInfo, '\0', sizeof(videoinfo_t));
    memcpy(&videoInfo, m_hH264->info, sizeof(videoinfo_t));
    
    if (NULL != m_hH264) {
        h264_free(m_hH264);
        m_hH264 = NULL;
    }
    
	string sProfileInfo = "";        //profile
	string sVideoFormat = "";        //format
	char sLevelInfo[10] = {'\0'};
	string sTierInfo = "";
	char sBitDepth[35] = {'\0'};

	if (videoInfo.type)
	{
		//h265
	}
	else // h264
	{
		// profile
		switch (videoInfo.profile_idc)
		{
		case 66:
			sProfileInfo = "Baseline";
			break;
		case 77:
			sProfileInfo = "Main";
			break;
		case 88:
			sProfileInfo = "Extended";
			break;
		case 100:
			sProfileInfo = "High";
			break;
		case 110:
			sProfileInfo = "High 10";        
			break;
		case 122:
			sProfileInfo = "High 422";        
			break;
		case 144:
			sProfileInfo = "High 444";        
			break;
		default:
			sProfileInfo = "Unknown";
			break;
		}
		sTierInfo = "";
		sprintf(sLevelInfo, "%d", videoInfo.level_idc);
	}
	// common
	// bit depth
	sprintf(sBitDepth, "Luma bit: %d Chroma bit: %d", videoInfo.bit_depth_luma, videoInfo.bit_depth_chroma);

	// chroma format
	switch (videoInfo.chroma_format_idc)
	{
	case 1:
		sVideoFormat = "YUV420";
		break;
	case 2:
		sVideoFormat = "YUV422";
		break;
	case 3:
		sVideoFormat = "YUV444";
		break;
	case 0:
		sVideoFormat = "monochrome";
		break;
	default:
		sVideoFormat = "Unknown";
		break;
	}

	//video code type
	media_info.file_info = videoInfo.type ? "H.265/HEVC" : "H.264/AVC";
	//video size
	char temp_buffer[100] = {'\0'};
	sprintf(temp_buffer, "%dx%d", videoInfo.width, videoInfo.height);
	media_info.pic_size = string(temp_buffer);
	//video format
	memset(temp_buffer, 100, '\0');
	sprintf(temp_buffer, "%s %s", sVideoFormat.c_str(), sBitDepth);
	media_info.video_format = string(temp_buffer);
	//video profile info
	memset(temp_buffer, 100, '\0');
	sprintf(temp_buffer, "%s Profile @ Level %s %s", sProfileInfo.c_str(), sLevelInfo, sTierInfo.c_str());
	media_info.profile_info = string(temp_buffer);
	//video encoding type
	memset(temp_buffer, 100, '\0');
	sprintf(temp_buffer, "%s", videoInfo.encoding_type ? "CABAC" : "CAVLC");
	media_info.encoding_type = string(temp_buffer);
	
    return iRet;
}

/**
 *	@ ËÑË÷ÊÓÆµÎÄ¼þµÄNALµ¥Ôª£¬¼ÇÂ¼Æ«ÒÆ¼°³¤¶È
 * @ inout vector<NALU_t>& vNal
 * @ in int num
 *
 * @ return result
 */
unsigned int probeNALU(FILE* h264File, h264_stream_t* m_hH264, vector<NALU_t>& vNal, int num) {
    int nal_num=0;
    int offset=0;
    int nalLen = 0;
    
    NALU_t n;
    memset(&n, '\0', sizeof(NALU_t));
    n.type = FILE_H264;
    
    offset = findFirstNALU(h264File, &(n.startcodeLen));
    if (offset < 0)
    {
        return FLV_VIDEOTAG_NO_NALU;
    }
    fseek(h264File, offset, SEEK_SET);
    while (!feof(h264File))
    {
        if (num > 0 && nal_num == num)
        {
            break;
        }
        nalLen = getAnnexbNALU(h264File, m_hH264, &n);//Ã¿Ö´ÐÐÒ»´Î£¬ÎÄ¼þµÄÖ¸ÕëÖ¸Ïò±¾´ÎÕÒµ½µÄNALUµÄÄ©Î²£¬ÏÂÒ»¸öÎ»ÖÃ¼´ÎªÏÂ¸öNALUµÄÆðÊ¼Âë0x000001
        n.offset = offset;
        n.num = nal_num;
        offset = offset + nalLen;
        
        vNal.push_back(n);
        
        nal_num++;
    }
    return STREAM_OK;
}

/**
 *	@ ½âÎöNAL£¬·µ»ØÁ½¸ö¿ªÊ¼×Ö·ûÖ®¼ä¼ä¸ôµÄ×Ö½ÚÊý£¬¼´°üº¬startcodeµÄNALUµÄ³¤¶È
 * @ in	FILE* fp£ºÊÓÆµÎÄ¼þÖ¸Õë
 * @ inout NALU_t* nalu£º
 *
 * @ return result
 * @ note£ºÒ»¸öÊÓÆµÎÄ¼þÖÐ²»Í¬µÄNAL£¬startcode¿ÉÄÜ²»Ò»Ñù¡£±ÈÈçSPSÎª4×Ö½Ú£¬µ«SEI¿ÉÄÜÎª3×Ö½Ú
 */
int getAnnexbNALU(FILE* fp, h264_stream_t* m_hH264, NALU_t* nalu)
{
    int pos = 0;
    int found, rewind;
    unsigned char *buffer;
    int info2=0, info3=0;
    int eof = 0;
    
    if ((buffer = (unsigned char*)calloc (MAX_NAL_SIZE, sizeof(char))) == NULL)
        printf("Could not allocate buffer memory\n");
    
    if (3 != fread (buffer, 1, 3, fp))//´ÓÂëÁ÷ÖÐ¶Á3¸ö×Ö½Ú
    {
        free(buffer);
        return 0;
    }
    info2 = findStartcode3(buffer);//ÅÐ¶ÏÊÇ·ñÎª0x000001
    if(info2 != 1)
    {
        //Èç¹û²»ÊÇ£¬ÔÙ¶ÁÒ»¸ö×Ö½Ú
        if(1 != fread(buffer+3, 1, 1, fp))//¶ÁÒ»¸ö×Ö½Ú
        {
            free(buffer);
            return 0;
        }
        info3 = findStartcode4(buffer);//ÅÐ¶ÏÊÇ·ñÎª0x00000001
        if (info3 != 1)//Èç¹û²»ÊÇ£¬·µ»Ø-1
        {
            free(buffer);
            return -1;
        }
        else
        {
            //Èç¹ûÊÇ0x00000001,µÃµ½¿ªÊ¼Ç°×ºÎª4¸ö×Ö½Ú
            nalu->startcodeLen = 4;
        }
    }
    else
    {
        //Èç¹ûÊÇ0x000001,µÃµ½¿ªÊ¼Ç°×ºÎª3¸ö×Ö½Ú
        nalu->startcodeLen = 3;
    }
    
    pos = nalu->startcodeLen;
    //²éÕÒÏÂÒ»¸ö¿ªÊ¼×Ö·ûµÄ±êÖ¾Î»
    found = 0;
    info2 = 0;
    info3 = 0;
    
    while (!found)
    {
        if (feof(fp))//ÅÐ¶ÏÊÇ·ñµ½ÁËÎÄ¼þÎ²
        {
            eof = 1;
            goto got_nal;
        }
        buffer[pos++] = fgetc(fp);//¶ÁÒ»¸ö×Ö½Úµ½BUFÖÐ
        
        info3 = findStartcode4(&buffer[pos-4]);//ÅÐ¶ÏÊÇ·ñÎª0x00000001
        if(info3 != 1)
            info2 = findStartcode3(&buffer[pos-3]);//ÅÐ¶ÏÊÇ·ñÎª0x000001
        
        found = (info2 == 1 || info3 == 1);
    }
    
    // startcode¿ÉÄÜÎª3£¬Ò²¿ÉÄÜÎª4£¬¹ÊÒªÈç´ËÅÐ¶Ï
    rewind = (info3 == 1)? -4 : -3;
    
    if (0 != fseek (fp, rewind, SEEK_CUR))//°ÑÎÄ¼þÖ¸ÕëÖ¸ÏòÇ°Ò»¸öNALUµÄÄ©Î²
    {
        free(buffer);
        printf("Cannot fseek in the bit stream inFile");
    }
    
got_nal:
    // µ±´ïµ½ÎÄ¼þÄ©Î²Ê±£¬»ØÍË1¸öÎ»ÖÃ
    if (eof)
    {
        rewind = -1;
    }
    
    // °üÀ¨ÆðÊ¼ÂëÔÚÄÚµÄ5¸ö×Ö½Ú
    if (nalu->startcodeLen == 3)
        sprintf(nalu->startcodeBuffer, "%02x%02x%02x%02x", buffer[0], buffer[1], buffer[2], buffer[3]);
    else
        sprintf(nalu->startcodeBuffer, "%02x%02x%02x%02x%02x", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
    nalu->len = pos+rewind;
    
    uint8_t nal_header = 0;
    if (nalu->type)
    {
        //H265ÔÝÊ±×¢ÊÍµô
        /*
         m_hH265->sh->read_slice_type = 1;
         h265_read_nal_unit(m_hH265, &buffer[nalu->startcodeLen], nalu->len - nalu->startcodeLen);
         nalu->nalType = m_hH265->nal->nal_unit_type;
         nalu->sliceType = m_hH265->sh->slice_type;
         m_hH265->sh->read_slice_type = 0;*/
    }
    else
    {
        // simple version
#if 0
        nal_header = buffer[nalu->startcodeLen];
        nalu->nalType = nal_header & 0x1f;// 5 bit
        
        // »ñÈ¡sliceÀàÐÍ£ºIÖ¡¡¢PÖ¡¡¢BÖ¡
        // ×¢£ºÔÚnalÀàÐÍÎª1~5Ê±»ñÈ¡
        if (nalu->nalType <= 5 && nalu->nalType >= 1)
        {
            int start_bit = 0;
            int first_mb_in_slice = ue((char*)buffer+nalu->startcodeLen+1, 8, start_bit);
            nalu->sliceType = ue((char*)buffer+nalu->startcodeLen+1, 8, start_bit);
        }
        if (nalu->nalType == 7 || nalu->nalType == 8) // sps pps
        {
            read_nal_unit(m_hH264, &buffer[nalu->startcodeLen], nalu->len - nalu->startcodeLen);
        }
#else
        m_hH264->sh->read_slice_type = 1;
        read_nal_unit(m_hH264, &buffer[nalu->startcodeLen], nalu->len - nalu->startcodeLen);
        nalu->nalType = m_hH264->nal->nal_unit_type;
        nalu->sliceType = m_hH264->sh->slice_type;
        m_hH264->sh->read_slice_type = 0;
#endif
    }
    
    free(buffer);
    
    return (pos+rewind);//·µ»ØÁ½¸ö¿ªÊ¼×Ö·ûÖ®¼ä¼ä¸ôµÄ×Ö½ÚÊý£¬¼´°üº¬ÓÐÇ°×ºµÄNALUµÄ³¤¶È
}

/**
 *	@ ËÑË÷ÊÓÆµÎÄ¼þµÄµÚÒ»¸öNALU
 * @ in	FILE* fp£ºÊÓÆµÎÄ¼þÖ¸Õë
 * @ inout int* startcodeLenght£ºÆðÊ¼Âë³¤¶È
 *
 * @ return result
 */
int findFirstNALU(FILE* fp, int* startcodeLenght)
{
    int found = 0;
    int info2 = 0;
    int info3 = 0;
    int eof = 0;
    int pos = 0;
    int startcode_len = 0;
    unsigned char *buffer = NULL;
    
    if ((buffer = (unsigned char*)calloc(MAX_NAL_SIZE, sizeof(char))) == NULL)
        printf ("Could not allocate buffer memory\n");
    
    while (!found && !feof(fp))
    {
        //¶ÁÒ»¸ö×Ö½Úµ½BUFÖÐ
        buffer[pos++] = fgetc(fp);
        
        //ÅÐ¶ÏÊÇ·ñÎª0x00000001
        info3 = findStartcode4(&buffer[pos-4]);
        if(info3 != 1)
        {
            //ÅÐ¶ÏÊÇ·ñÎª0x000001
            info2 = findStartcode3(&buffer[pos-3]);
            if (info2)
            {
                startcode_len = 3;
            }
        }
        else
        {
            startcode_len = 4;
        }
        
        found = (info2 == 1 || info3 == 1);
        if (pos >= MAX_NAL_SIZE)
        {
            free(buffer);
            return -1;
        }
    }
    
    // ÎÄ¼þÖ¸ÕëÒª»Ö¸´
    fseek(fp, -startcode_len, SEEK_CUR);
    
    free(buffer);
    if (startcodeLenght != NULL)
        *startcodeLenght = startcode_len;
    
    return pos - startcode_len;
}


unsigned int CheckNalu(vector<NALU_t> vNalu, STATIC_INFO& statInfo) {
    for (vector<NALU_t>::iterator it = vNalu.begin(); it != vNalu.end(); ++it) {
        if ((*it).type == 0)         //H264
        {
            statInfo.avctype = FILE_H264;
            switch ((*it).nalType)
            {
                case 1: {
                    switch ((*it).sliceType)
                    {
                        case 0:
                        case 5:
                            statInfo.slice_p++;
                            break;
                        case 1:
                        case 6:
                            statInfo.slice_b++;
                            break;
                        case 2:
                        case 7:
                            statInfo.slice_i++;
                            break;
                        case 3:
                        case 8:
                            statInfo.slice_sp++;
                            break;
                        case 4:
                        case 9:
                            statInfo.slice_si++;
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case 5: {
                    statInfo.idr++;
                    break;
                }
                default: {
                    break;
                }
            }
        }else{
            //H265
            statInfo.avctype = FILE_H265;
        }
    }
    return STREAM_OK;
}


