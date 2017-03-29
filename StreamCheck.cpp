#include "common.h"
#include "StreamCheck.h"
#include "pauta.h"

unsigned int CheckBitStream(FILE* h264File) {
	iFrame = 0;
	pFrame = 0;
	bFrame = 0;
	memset(&gopInfo, 0, sizeof(GOP_INFO));
	iFrameSize.clear();
	bFrameSize.clear();
	pFrameSize.clear();
	streamInfo.clear();
	g_media_info.clear();;

	if (!h264File) {
		return FILE_NOT_EXSIT;
	}
	h264_stream_t* m_hH264 = h264_new();
	vector<NALU_t> vNal;
	unsigned int eRet = probeNALU(h264File, m_hH264, vNal, -1);
	if (STREAM_OK == eRet) {
		PrintNALInfo(vNal);
		PrintStreamInfo(m_hH264);
	}

	if (m_hH264) {
		h264_free(m_hH264);
		m_hH264 = NULL;
	}

	return eRet;
}

/**
  *	@ ������Ƶ�ļ���NAL��Ԫ����¼ƫ�Ƽ�����
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
		nalLen = getAnnexbNALU(h264File, m_hH264, &n);//ÿִ��һ�Σ��ļ���ָ��ָ�򱾴��ҵ���NALU��ĩβ����һ��λ�ü�Ϊ�¸�NALU����ʼ��0x000001
		n.offset = offset;
		n.num = nal_num;
		offset = offset + nalLen;

		vNal.push_back(n);

		nal_num++;
	}
	return STREAM_OK;
}

/**
  *	@ ����NAL������������ʼ�ַ�֮�������ֽ�����������startcode��NALU�ĳ���
  * @ in	FILE* fp����Ƶ�ļ�ָ��
  * @ inout NALU_t* nalu��
  *	
  * @ return result
  * @ note��һ����Ƶ�ļ��в�ͬ��NAL��startcode���ܲ�һ��������SPSΪ4�ֽڣ���SEI����Ϊ3�ֽ�
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

	if (3 != fread (buffer, 1, 3, fp))//�������ж�3���ֽ�
	{
		free(buffer);
		return 0;
	}
	info2 = findStartcode3(buffer);//�ж��Ƿ�Ϊ0x000001
	if(info2 != 1)
	{
		//������ǣ��ٶ�һ���ֽ�
		if(1 != fread(buffer+3, 1, 1, fp))//��һ���ֽ�
		{
			free(buffer);
			return 0;
		}
		info3 = findStartcode4(buffer);//�ж��Ƿ�Ϊ0x00000001
		if (info3 != 1)//������ǣ�����-1
		{
			free(buffer);
			return -1;
		}
		else
		{
			//�����0x00000001,�õ���ʼǰ׺Ϊ4���ֽ�
			nalu->startcodeLen = 4;
		}
	}
	else
	{
		//�����0x000001,�õ���ʼǰ׺Ϊ3���ֽ�
		nalu->startcodeLen = 3;
	}

	pos = nalu->startcodeLen;
	//������һ����ʼ�ַ��ı�־λ
	found = 0;
	info2 = 0;
	info3 = 0;

	while (!found)
	{
		if (feof(fp))//�ж��Ƿ����ļ�β
		{
			eof = 1;
			goto got_nal;
		}
		buffer[pos++] = fgetc(fp);//��һ���ֽڵ�BUF��

		info3 = findStartcode4(&buffer[pos-4]);//�ж��Ƿ�Ϊ0x00000001
		if(info3 != 1)
			info2 = findStartcode3(&buffer[pos-3]);//�ж��Ƿ�Ϊ0x000001

		found = (info2 == 1 || info3 == 1);
	}

	// startcode����Ϊ3��Ҳ����Ϊ4����Ҫ����ж�
	rewind = (info3 == 1)? -4 : -3;

	if (0 != fseek (fp, rewind, SEEK_CUR))//���ļ�ָ��ָ��ǰһ��NALU��ĩβ
	{
		free(buffer);
		printf("Cannot fseek in the bit stream inFile");
	}

got_nal:
	// ���ﵽ�ļ�ĩβʱ������1��λ��
	if (eof)
	{
		rewind = -1;
	}

	// ������ʼ�����ڵ�5���ֽ�
	if (nalu->startcodeLen == 3)
		sprintf(nalu->startcodeBuffer, "%02x%02x%02x%02x", buffer[0], buffer[1], buffer[2], buffer[3]);
	else
		sprintf(nalu->startcodeBuffer, "%02x%02x%02x%02x%02x", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
	nalu->len = pos+rewind;

	uint8_t nal_header = 0;
	if (nalu->type)
	{
		//H265��ʱע�͵�
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

		// ��ȡslice���ͣ�I֡��P֡��B֡
		// ע����nal����Ϊ1~5ʱ��ȡ
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

	return (pos+rewind);//����������ʼ�ַ�֮�������ֽ�������������ǰ׺��NALU�ĳ���
}

/**
  *	@ ������Ƶ�ļ��ĵ�һ��NALU
  * @ in	FILE* fp����Ƶ�ļ�ָ��
  * @ inout int* startcodeLenght����ʼ�볤��
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
		//��һ���ֽڵ�BUF��
		buffer[pos++] = fgetc(fp);

		//�ж��Ƿ�Ϊ0x00000001
		info3 = findStartcode4(&buffer[pos-4]);
		if(info3 != 1)
		{
			//�ж��Ƿ�Ϊ0x000001
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

	// �ļ�ָ��Ҫ�ָ�
	fseek(fp, -startcode_len, SEEK_CUR);

	free(buffer);
	if (startcodeLenght != NULL)
		*startcodeLenght = startcode_len;

	return pos - startcode_len;
}

void PrintNALInfo(vector<NALU_t> vNal)
{
	//GOP��Ϣ
	memset(&gopInfo, 0, sizeof(GOP_INFO));
	
	//��ӡh264/h265 ��nalu��Ϣ
	int nNal = vNal.size();
	int m_nSliceIndex = 1;
	while(true) {
		printf("\n\n");
		if (nNal <= 0) {
			printf("The Video CodecID is not 7\n");
			break;
		}else {
			//������һ����Ƶ�ļ�ֻ��һ�ֱ����ʽ
			printf("The Video CodecID is 7 and the code type is %5d\n", vNal[0].type);
		}
		printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
		printf("|                            Video  Tag  Information                                       |\n");
		printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
		printf("|No.   |Offset    |Length |Start Code  |NAL Type                             |Info         |\n");

		for (int i = 0; i < nNal; i++)
		{
			STREAM_SLICE_INFO sliceInfo;

			char nalTypeInfo[37] = {0};
			char nalInfo[15] = {0};
			//H264
			if (vNal[i].type == 0)
			{
				// NAL��Ԫ����
				switch (vNal[i].nalType)
				{
				case 0:
					sprintf(nalTypeInfo, "Unspecified");
					break;
				case 1:
					sprintf(nalTypeInfo, "Coded slice of a non-IDR picture");
					switch (vNal[i].sliceType)
					{
					case 0:
					case 5:
						sprintf(nalInfo, "P Slice #%d", m_nSliceIndex);
						pFrame++;
						break;
					case 1:
					case 6:
						sprintf(nalInfo, "B Slice #%d", m_nSliceIndex);
						bFrame++;
						break;
					case 2:
					case 7:
						sprintf(nalInfo, "I Slice #%d", m_nSliceIndex);
						//ͳ��GOP��Ϣ
						iFrame++;
						validPFrame = pFrame;
						validBFrame = bFrame;
						if (2 == iFrame) {
							gopInfo.bFrameNum = bFrame;
							gopInfo.pFrameNum = pFrame;
							gopInfo.gopSize = bFrame + pFrame;
						}
						break;
					}
					m_nSliceIndex++;
					break;
				case 2:
					sprintf(nalTypeInfo, "DPA");
					break;
				case 3:
					sprintf(nalTypeInfo, "DPB");
					break;
				case 4:
					sprintf(nalTypeInfo, "DPC");
					break;
				case 5:
					sprintf(nalTypeInfo, "Coded slice of an IDR picture");
					sprintf(nalInfo, "IIDR #%d", m_nSliceIndex);
					//ͳ��GOP��Ϣ
					iFrame++;
					validPFrame = pFrame;
					validBFrame = bFrame;
					if (2 == iFrame) {
						gopInfo.bFrameNum = bFrame;
						gopInfo.pFrameNum = pFrame;
						gopInfo.gopSize = bFrame + pFrame;
					}

					m_nSliceIndex++;
					break;
				case 6:
					sprintf(nalTypeInfo, "Supplemental enhancement information");
					sprintf(nalInfo, "SEI");
					break;
				case 7:
					sprintf(nalTypeInfo, "Sequence parameter set");
					sprintf(nalInfo, "SPS");
					break;
				case 8:
					sprintf(nalTypeInfo, "Picture parameter set");
					sprintf(nalInfo, "PPS");
					break;
				case 9:
					sprintf(nalTypeInfo, "Access UD");
					sprintf(nalInfo, "AUD");
					break;
				case 10:
					sprintf(nalTypeInfo, "END_SEQUENCE");
					break;
				case 11:
					sprintf(nalTypeInfo, "END_STREAM");
					break;
				case 12:
					sprintf(nalTypeInfo, "FILLER_DATA");
					break;
				case 13:
					sprintf(nalTypeInfo, "SPS_EXT");
					break;
				case 19:
					sprintf(nalTypeInfo, "AUXILIARY_SLICE");
					break;
				default:
					sprintf(nalTypeInfo, "Other");
					break;
				}
			}else{
				//Ԥ��H265
			}

			sliceInfo.offset = vNal[i].offset;
			sliceInfo.length = vNal[i].len;
			sliceInfo.startcode = vNal[i].startcodeBuffer;
			sliceInfo.nalTypeInfo = nalTypeInfo;
			sliceInfo.nalInfo = nalInfo;
			streamInfo.push_back(sliceInfo);
			printf("|%-6d|%08X  |%-7d|%-12s|%-37s|%-13s|\n", vNal[i].num + 1, vNal[i].offset, vNal[i].len, vNal[i].startcodeBuffer, nalTypeInfo, nalInfo);
		}
		break;
	}
	printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");

	return;
}
void PrintStreamInfo(h264_stream_t* m_hH264) {
	if (!m_hH264) {
		return;
	}
	g_media_info.clear();
	videoinfo_t videoInfo;
	memset(&videoInfo, '\0', sizeof(videoinfo_t));
	memcpy(&videoInfo, m_hH264->info, sizeof(videoinfo_t));

	printf("\n\n");
	printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
	printf("|                            Video               Info                                      |\n");
	printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");

	string sProfileInfo = "";		//profile
	string sVideoFormat = "";		//format
	char sLevelInfo[10] = {'\0'};
	string sTierInfo = "";
	char sBitDepth[35] = {'\0'};

	if (videoInfo.type)
	{
		//h265
	}
	else // h264
	{
		// profile����
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
			sProfileInfo = "Unkown";
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
		sVideoFormat = "Unkown";
		break;
	}
	printf("%s File Information\r\n", videoInfo.type ? "H.265/HEVC" : "H.264/AVC");
	printf("Picture Size	\t: %dx%d\n", videoInfo.width, videoInfo.height);
	printf("- Cropping Left   \t: %d\n", videoInfo.crop_left);
	printf("- Cropping Right  \t: %d\n", videoInfo.crop_right);
	printf("- Cropping Top    \t: %d\n", videoInfo.crop_top);
	printf("- Cropping Bottom \t: %d\n", videoInfo.crop_bottom);
	printf("Video Format \t: %s %s\n", sVideoFormat.c_str(), sBitDepth);
	printf("Stream Type \t: %s Profile @ Level %s %s\n", sProfileInfo.c_str(), sLevelInfo, sTierInfo.c_str());
	printf("Encoding Type \t: %s\n", videoInfo.encoding_type ? "CABAC" : "CAVLC");
	printf("Max fps \t\t: %.03f\n", videoInfo.max_framerate);

	char temp_buffer[100] = {'\0'};
	sprintf(temp_buffer, "%dx%d", videoInfo.width, videoInfo.height);
	g_media_info.pic_size = string(temp_buffer);
	memset(temp_buffer, 100, '\0');
	sprintf(temp_buffer, "%s %s", sVideoFormat.c_str(), sBitDepth);
	g_media_info.video_format = string(temp_buffer); 
	memset(temp_buffer, 100, '\0');
	sprintf(temp_buffer, "%s Profile @ Level %s %s", sProfileInfo.c_str(), sLevelInfo, sTierInfo.c_str());
	g_media_info.stream_type = string(temp_buffer); 
	memset(temp_buffer, 100, '\0');
	sprintf(temp_buffer, "%s", videoInfo.encoding_type ? "CABAC" : "CAVLC");
	g_media_info.encoding_type = string(temp_buffer); 
	
	return;
}

unsigned int CheckStreamDataInfo(int& vAbnormal) {
	unsigned int eRet = STREAM_OK;
	//У��GOP�ķֲ��Ƿ����һ������,����й����������ģ�����û�й���Ҳ���Բ��ţ�ע�͵�
/*
	if (iFrame > 2) {
		if (
			(validBFrame / (iFrame - 1) != gopInfo.bFrameNum) || 
			(validPFrame / (iFrame - 1) != gopInfo.pFrameNum) || 
			(validBFrame + validPFrame) / (iFrame - 1) != gopInfo.gopSize) {
			eRet = FLV_VIDEOGOP_EXCEPTION; 
		}
	}
*/

	//У������֡�Ĵ�С�Ƿ����һ������
	vector<unsigned int> iAbnormal;
	if(iFrameSize.size() > 10) {
		int iRet = PautaCheck(iFrameSize, iAbnormal);
		if (!iRet && iAbnormal.size() > 0) {
			eRet |= FLV_FRAMESIZE_EXCEPTION;
		}
	}
	vector<unsigned int> bAbnormal;
	if (bFrameSize.size() > 10) {
		int iRet = PautaCheck(bFrameSize, bAbnormal);
		if (!iRet && bAbnormal.size() > 0) {
			eRet |= FLV_FRAMESIZE_EXCEPTION;
		}
	}
	vector<unsigned int> pAbnormal;
	if (pFrameSize.size() > 10) {
		int iRet = PautaCheck(pFrameSize, pAbnormal);
		if (!iRet && pAbnormal.size() > 0) {
			eRet |= FLV_FRAMESIZE_EXCEPTION;
		}
	}
	vAbnormal = iAbnormal.size() + bAbnormal.size() + pAbnormal.size();

	return eRet;
}

int GetVideoStreamInfo(FLV_STAT_INFO& statInfo) {
	statInfo.iFrame = iFrame;
	statInfo.pFrame = pFrame;
	statInfo.bFrame = bFrame;
	statInfo.frame = iFrame + pFrame + bFrame;

	return 0;
}

vector<STREAM_SLICE_INFO> GetStreamInfo() {
	return streamInfo;
}		
MEDIA_INFO GetMediaInfo() {
	return g_media_info;
}

