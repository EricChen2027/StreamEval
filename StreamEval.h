#ifndef __STREAMEVAL_H__
#define __STREAMEVAL_H__

#include "libjsoncpp/json/json.h"

/**
 * @ char*     : stream url
 * @ char*     : 文件路径
 * @ char*     : 文件名称
 * @ int       : 校验模式(0: 按照大小静态校验, param对应文件大小;1：按照时间静态校验，param对应时长)
 *
 */
JSONCPP_STRING StreamEval(char* RTMP_URL, const char* filePath, char* fileSubName, int model = 0, uint32_t param = 2 * 1024);



#endif
