#ifndef __PAUTA_H__
#define __PAUTA_H__
//拉依达准则
#include <stdint.h>
#include <vector>
using namespace std;

int PautaCheck(vector<uint32_t> srcData, vector<uint32_t>& vAbnormal, int ratio = 3);

#endif


