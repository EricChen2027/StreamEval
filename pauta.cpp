#include <math.h>
#include <stdlib.h>
#include "pauta.h"

using namespace std;

//拉依达准则适用于样本比较大的场合
int PautaCheck(vector<uint32_t> srcData, vector<uint32_t>& vAbnormal, int ratio) {
	int length = srcData.size();
	if (length <= 10) {
		return -1;
	}
	if (0 != vAbnormal.size()) {
		vAbnormal.clear();
	}

	//期望
	long sum = 0;
	for(vector<uint32_t>::iterator it = srcData.begin(); it != srcData.end(); it++) {
		sum += *it;
	}
	float expectation = sum / length;
	//方差
	float varSum = 0.0;
	for(vector<uint32_t>::iterator it = srcData.begin(); it != srcData.end(); it++) {
		varSum += pow((*it - expectation), 2);
	}
	float variance = sqrt(varSum / length);

	//拉依达
	for(vector<uint32_t>::iterator it = srcData.begin(); it != srcData.end(); it++) {
		if (abs(*it - expectation) > ratio * variance) {
			vAbnormal.push_back(*it);
		}
	}

	return 0;
}
