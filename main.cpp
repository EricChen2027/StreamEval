#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <fstream>
#include <time.h>
#include <map>

#define NO_FCGI_DEFINES
#include "libfcgi/fcgi_stdio.h"
#include "StreamEval.h"
#include "libpcre/pcre.h"

using namespace std;


#define SHOW_RESULT_INJSON 1 
#define OVECCOUNT          30          //3的倍数

//const char* HTDOCS_PATH = "/home/chenyu/nginx/htdocs/StreamEval/";
const char* HTDOCS_PATH = "/data/StreamEval/";

string getParam(string uri, string key) {
	string value = "";
	
	const char* error;
	int erroroffset;
	int ovector[OVECCOUNT];
	string pattern = "(&|^)" + key + "=[^&]+";
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

	FILE* outFile = NULL;
    int count = 0;

	while(FCGI_Accept() >= 0) {
		char* queryString = getenv("QUERY_STRING");
		//char queryString[300] = "url=http://pull99.a8.com/live/1490588630267836.flv&form=json&model=1&modelInfo=18";

		string url = getParam(string(queryString), "url");
		string form = getParam(string(queryString), "form");
		string model = getParam(string(queryString), "model");
		string modelInfo = getParam(string(queryString), "modelInfo");

		// EVAL
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
		
		char* queryurl = (char*) malloc(url.length() + 1);
		for (int i = 0; i < url.length() + 1; i++) {
			queryurl[i] = '\0';
		}
		url.copy(queryurl, url.length() + 1, 0);
		string resultInfo = "";
		if (model != "") {
			resultInfo = StreamEval(queryurl, HTDOCS_PATH, fileSubName, atoi(model.c_str()), atoi(modelInfo.c_str()));
		}else {
			resultInfo = StreamEval(queryurl, HTDOCS_PATH, fileSubName);
		}
		free(queryurl);
		
		if (form == "json") {
			FCGI_printf("Content-type:text/plain\r\n"
						"\r\n");
			FCGI_printf(resultInfo.c_str());
		}else {
			FCGI_printf("Content-type:text/plain\r\n"
						"\r\n");
			char outFileName[100] = {'0'};
			sprintf(outFileName, "%s%s.result", HTDOCS_PATH, fileSubName);		
			string line;
			fstream fs;
			fs.open(outFileName, fstream::in);
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
	}

	return 0;
}


