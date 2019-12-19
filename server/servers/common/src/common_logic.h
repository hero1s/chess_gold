
#pragma once

#include "svrlib.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <stdlib.h>

class CPlayer;

class CPlayerBase;

using namespace std;
using namespace svrlib;

class CCommonLogic {
public:
    // ���ʱ���Ƿ�������
    static bool IsJoinTime(string startTime, string endTime);
    // ��ӡ�����ַ���
    static void LogCardString(uint8_t cardData[], uint8_t cardCount);
    // У���½��
    static bool VerifyPasswd(string& loginKey, uint32_t uid, uint32_t checkTime);

    // �ж��Ƿ�������Ϣ
    static bool IsNeedReset(time_t lastTime, time_t curTime);

    // ԭ���޸������������
    static bool AtomChangeOfflineAccData(uint32_t uid, uint16_t operType, uint16_t subType, int64_t coin, int64_t safecoin, const string &chessid = "");



};

