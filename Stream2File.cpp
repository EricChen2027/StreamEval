#include <stdio.h>
#include <time.h>

#include "Stream2File.h"

using namespace std;

#define RTMP_READ_BUFFER_SIZE	64 * 1024

static DLL_HANDLE curlHandle = NULL;
static time_t stat_st;
static uint32_t stat_model = 0;
static uint32_t stat_param = 0;

unsigned int RTMPStreamToFlv(FILE* outFile, char* streamUrl, int model, uint32_t param) {
	unsigned int eRet = STREAM_OK;
	if (!outFile) {
		return FILE_NOT_EXSIT;
	}

	//启动socket模块
	if (!InitSockets())	{
		RTMP_Log(RTMP_LOGERROR,	"Couldn't load sockets support on your platform, exiting!");

		return LIBSOCKET_LOAD_FAILED;
	}
	//初始化rtmp
	RTMP rtmp = { 0 };
	RTMP_Init(&rtmp);
	//设置url
	if (RTMP_SetupURL(&rtmp, streamUrl) == FALSE)
	{
		RTMP_Log(RTMP_LOGERROR, "Couldn't parse URL: %s", streamUrl);
		CleanupSockets();

		return STREAM_PARSE_FAILED;
	}
	//设置流类型
	int bLiveStream = TRUE;		
	if (bLiveStream){
		rtmp.Link.lFlags |= RTMP_LF_LIVE;
	}
	//设置流时间：
	RTMP_SetBufferMS(&rtmp, 10*1000);
	//建立网络连接
	RTMP_LogPrintf("Connecting ...\n");
	if (!RTMP_Connect(&rtmp, NULL))	{
		RTMP_LogPrintf("Connect Failed\n");
		CleanupSockets();

		return STREAM_CONNECT_ERROR;
	}
	RTMP_Log(RTMP_LOGINFO, "Connected...");
	//建立流连接,没有seek时间
	if (!RTMP_ConnectStream(&rtmp, 0)) {
		RTMP_Log(RTMP_LOGERROR,"ConnectStream Err\n");
		RTMP_Close(&rtmp);
		CleanupSockets();

		return STREAM_CONNECT_ERROR;
	}
	//读数据
	int nRead = 0;
	long size = ftell(outFile);
	size_t bufferSize = RTMP_READ_BUFFER_SIZE;
	char *buffer = (char *) malloc(bufferSize);
	RTMP_LogPrintf("Downloading ...\n");
	
	time_t st;
	time(&st);
	bool bQuit = false;
	do
	{
		switch(model) {
			case 0:{
				if (size > param * 1024) {
					bQuit = true;
				}
				break;
			}
			case 1:{
				time_t ed;
				time(&ed);
				if (difftime(ed, st) > param) {
					bQuit = true;
				}	
				break;
			}
			default: {
				break;
			}
		}
		if (bQuit == true) {
			break;
		}

		nRead = RTMP_Read(&rtmp, buffer, bufferSize);
		if (nRead > 0) {
			if (fwrite(buffer, sizeof(unsigned char), nRead, outFile) != (size_t) nRead)
			{
				RTMP_Log(RTMP_LOGERROR, "%s: Failed writing, exiting!", __FUNCTION__);
				eRet = FILE_WRITE_ERROR;
				break;
			}
			size += nRead;

			RTMP_LogPrintf("Receive: %5dByte, Total: %5.2fkB\n", nRead, size*1.0/1024);
		}
		else {
#ifdef _DEBUG
			RTMP_Log(RTMP_LOGDEBUG, "zero read!");
#endif
			if (rtmp.m_read.status == RTMP_READ_EOF)
				break;
		}
	}
	while (nRead > -1 && RTMP_IsConnected(&rtmp) && !RTMP_IsTimedout(&rtmp));
	RTMP_LogPrintf("Download End\n");

	//clean
	if (buffer) {
		free(buffer);
	}

	RTMP_Close(&rtmp);
	CleanupSockets();

	return eRet;
}
unsigned int HTTPStreamToFlv(FILE* outFile, char* streamUrl, int model, uint32_t param) {
	stat_model = model;
	stat_param = param;
	if (1 == model) {
		time(&stat_st);
	}		

#if (defined (WIN32) || defined (WINDOWS) || defined(_WINDOWS_))  //windows
	curlHandle = LOAD_CURLDLL("lib/libcurl.dll");
	if (!curlHandle) {
		return LIBCURL_LOAD_FAILED;
	}
#else
	//do nothing
#endif;

	CURL *curl;
	CURLcode res;
	unsigned int eRet = STREAM_OK;
#if (defined (WIN32) || defined (WINDOWS) || defined(_WINDOWS_))  //windows
	curl_global_init_p		curl_global_init	= (curl_global_init_p)GET_CURLFUNC(curlHandle, curl_global_init);
	curl_global_cleanup_p	curl_global_cleanup = (curl_global_cleanup_p)GET_CURLFUNC(curlHandle, curl_global_cleanup);
	curl_easy_init_p		curl_easy_init		= (curl_easy_init_p)GET_CURLFUNC(curlHandle, curl_easy_init);
	curl_easy_cleanup_p		curl_easy_cleanup	= (curl_easy_cleanup_p)GET_CURLFUNC(curlHandle, curl_easy_cleanup);
	curl_easy_setopt_p		curl_easy_setopt	= (curl_easy_setopt_p)GET_CURLFUNC(curlHandle, curl_easy_setopt);
	curl_easy_perform_p		curl_easy_perform	= (curl_easy_perform_p)GET_CURLFUNC(curlHandle, curl_easy_perform);
	curl_easy_getinfo_p		curl_easy_getinfo	= (curl_easy_getinfo_p)GET_CURLFUNC(curlHandle, curl_easy_getinfo);
	curl_easy_strerror_p	curl_easy_strerror	= (curl_easy_strerror_p)GET_CURLFUNC(curlHandle,  curl_easy_strerror);
#else
	//use static lib,do nothing
#endif 
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, streamUrl);
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1);
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, -1);
//		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.0.3705; .NET CLR 1.1.4322)");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, StreamWriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, outFile);

		res = curl_easy_perform(curl);
		if (CURLE_OK != res) {
			long rsp_code = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rsp_code);
			if (200 != rsp_code) {
				printf("something is wrong and the errorcode is %d\n", rsp_code);
			}
			if (CURLE_WRITE_ERROR != res) {
				fprintf(stderr, "curl_easy_perform() failed: %s\n",	curl_easy_strerror(res));
				eRet = STREAM_CONNECT_ERROR;
			}
		}
	
		fseek(outFile, 0,  SEEK_END);
		if (ftell(outFile) <= 0) {
			eRet = STREAM_DOWNLOAD_FAILED;
		}

		//is 403 forbidden?
		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		if (http_code == 403) {
			eRet = HTTP_403_FORBIDDEN;	
		}else if (http_code == 404) {
			eRet = HTTP_404_NOTFOUND;
		}

		curl_easy_cleanup(curl);
	}
	
	curl_global_cleanup();

	return eRet;
}

size_t StreamWriteCallback(char* buffer, size_t size, size_t nmemb, void *outFile) {
	if (!outFile) {
		printf("outFile is null\n");
		return 0;
	}

	size_t wSize = fwrite(buffer, size, nmemb, (FILE*)outFile);
	fflush((FILE*)outFile);
	size_t fileSize = ftell((FILE*)outFile);
	RTMP_LogPrintf("Receive: %5dByte, Total: %5.2fkB\n", wSize, fileSize*1.0/1024);

	switch(stat_model) {
		case 0:{
			#if (defined (WIN32) || defined (WINDOWS) || defined(_WINDOWS_))  //windows
			if (fileSize >= stat_param * 1024 && NULL != curlHandle) {
				return CURLPAUSE_ALL;
			}
			#else
			if (fileSize >= stat_param * 1024) {
 				return CURLPAUSE_ALL;
			}
			#endif
			
			break;
		}
		case 1: {
			time_t ed;
			time(&ed);
			if (difftime(ed, stat_st) > stat_param) {
				return CURLPAUSE_ALL;
			}
			break;
		}
		default:{
			break;
		}
	}

	return wSize;
}

