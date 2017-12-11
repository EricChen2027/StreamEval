#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <fstream>
#include <time.h>
#include <map>

#define NO_FCGI_DEFINES
#include "fcgi/fcgi_stdio.h"
#include "pcre/pcre.h"
#include "curl/curl.h"
#include "Common.hpp"
#include "StreamEval.hpp"

using namespace std;

#define OVECCOUNT          30          //3的倍数

const char* HTDOCS_PATH = "/data/StreamEval/";

string getParam(string uri, string key) {
	string value = "";
	
	const char* error;
	int erroroffset;
	int ovector[OVECCOUNT];
	string pattern = "(@|^)" + key + "=[^@]+";
	const char* pOut;
	pcre* reg = pcre_compile(pattern.c_str(), 0, &error, &erroroffset, NULL);
	if (NULL != reg) {
		int ret = pcre_exec(reg, NULL, uri.c_str(), uri.length(), 0, 0, ovector, OVECCOUNT);
		if (ret == 0) {
			printf("But too many substrings were found to fit in subStrVec!\n");
		}
		for (int i = 0; i < ret; i=i+2) {	
			pcre_get_substring(uri.c_str(), ovector, ret, i, &pOut);
			if (pOut != "") {
				value = pOut;
				pcre_free_substring(pOut);
		
				break;
			}
		}
	}
	pcre_free(reg);
	
	size_t pos = value.find("=");
	if (pos >= 0) {
		value = value.substr(pos + 1);
	}

	return value;
}

int main(int argc, char* argv[]) {
	if (argc <= 0) {
		return -1;
	}

	FILE* mediaFile = NULL;
	FILE* resultFile = NULL;
	int iRet = 0;

	while(FCGI_Accept() >= 0) {
		char* queryString = getenv("QUERY_STRING");
		//char queryString[300] = "url=/data/StreamEval/2017-06-07%2016:04:43.flv&model=2&form=json";
		//char queryString[300] = "url=http://hdl1.v.momocdn.com/live/m_ddf6bb554d6bb8e41496815199425100.flv&form=json&model=1&modelInfo=10"; 

		string url = getParam(string(queryString), "url");
		string form = getParam(string(queryString), "form");
		string model = getParam(string(queryString), "model");
		string modelInfo = getParam(string(queryString), "modelInfo");
		int iModel = atoi(model.c_str());
		uint32_t uiMomelInfo = atoi(modelInfo.c_str());

		/************************************/
		/********     Stream Eval    ********/
		/************************************/
		FCGI_printf("Content-type:text/plain\r\n"
   					"\r\n");
		unsigned int uiRet = STREAM_OK;
		iRet = Init();
		if (0 != iRet) {
			FCGI_printf("StreamEval Init Failed!\r\n");
			continue;
		}
		//generate file name
		time_t currTime = time(NULL);
		struct tm* localTime = localtime(&currTime);
		char fileSubName[20] = {'0'};
		sprintf(fileSubName, "%d-%02d-%02d %02d:%02d:%02d",
		        localTime->tm_year + 1900,
				localTime->tm_mon + 1,
				localTime->tm_mday,
   				localTime->tm_hour,
   				localTime->tm_min,
  				localTime->tm_sec);   
		char mediaFileName[100] = {'0'};
		char resultFileName[100] = {'0'};
		int retryNum = 0;
		sprintf(mediaFileName, "%s%s.flv", HTDOCS_PATH, fileSubName);
		sprintf(resultFileName, "%s%s.result", HTDOCS_PATH, fileSubName);
		char* queryurl = (char*) malloc(url.length() + 1);
		for (int i = 0; i < url.length() + 1; i++) {
			queryurl[i] = '\0';
		}
		url.copy(queryurl, url.length() + 1, 0);

		//download file
		switch(iModel) {
			case 0:
			case 1: {
				while(!(mediaFile = fopen(mediaFileName, "wb+"))) {
					retryNum++;
					sprintf(mediaFileName, "%s%s.flv.%d", HTDOCS_PATH, fileSubName, retryNum);
					sprintf(resultFileName, "%s%s.result.%d", HTDOCS_PATH, fileSubName, retryNum);
				}
				uiRet = Stream2File(mediaFile, queryurl, iModel, uiMomelInfo);
				break;
			}
			case 2: {
				curl_global_init(CURL_GLOBAL_DEFAULT);
				CURL* curl = curl_easy_init();
				int length = 0;
				char* decode_url = curl_easy_unescape(curl, queryurl, string(queryurl).length(), &length);
				mediaFile = fopen(decode_url, "rb+");
				curl_free(decode_url);
				curl_easy_cleanup(curl);
				curl_global_cleanup();
				if (!mediaFile) {
					iRet = -2;
					FCGI_printf("File Not Exist!\r\n");
				}
				break;
			}
			default: {
				while(!(mediaFile = fopen(mediaFileName, "wb+"))) {
					retryNum++;
					sprintf(mediaFileName, "%s%s.flv.%d", HTDOCS_PATH, fileSubName, retryNum);
					sprintf(resultFileName, "%s%s.result.%d", HTDOCS_PATH, fileSubName, retryNum);
				}
				uiRet = Stream2File(mediaFile, queryurl);
				break;
			}
		}
		if (iRet < 0) {
			free(queryurl);
			if (mediaFile) {
				fclose(mediaFile);
   				mediaFile = NULL;
   			}
			UnInit();
			continue;
		}

		//Parse Media Package
		FLV_HEADER header;
		vector<FLV_TAG> vFlvTag;
		MEDIA_INFO mediaInfo;
		vector<NALU_t> vNalu;
		STATIC_INFO statInfo; 
		if (STREAM_OK == uiRet) {
			uiRet |= ParseMediaPackage(mediaFile, header, vFlvTag);
			uiRet |= ParseMediaSlice(mediaInfo, vNalu);
		}
		//format to json/text
		if (form == "json") {
			JSONCPP_STRING resultInfo = StreamEval(uiRet, mediaInfo, vNalu, statInfo, mediaFileName);
			FCGI_printf(resultInfo.c_str());
		}else {
			uiRet = StreamEval(uiRet, mediaInfo, vNalu, statInfo);
			resultFile = freopen(resultFileName, "w+", stdout);
			if (!resultFile) {
				iRet = -1;
				FCGI_printf("There is something wrong with the server!\r\n");
				continue;
			}
			Format2Text(uiRet, vNalu, statInfo, mediaInfo);
			fclose(resultFile);
			string line;
			fstream fs;
			fs.open(resultFileName, fstream::in);
			if (fs) {
				while(!fs.eof()) {
   					getline(fs, line);
 					FCGI_printf("%s\r\n", line.c_str());
  				}
				fs.close();
			}
			FCGI_printf("\r\n");
			FCGI_printf("Stream Evaluation End!\r\n");
		}

		//UnInit
		free(queryurl);
		if (mediaFile) {
			fclose(mediaFile);
			mediaFile = NULL;
		}
		UnInit();
	}

	FCGI_printf("There is something wrong with the server! Please inform chenyu to restart~\r\n");

	return 0;
}


