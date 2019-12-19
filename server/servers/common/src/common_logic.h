
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
    // 检测时间是否在区间
    static bool IsJoinTime(string startTime, string endTime);
    // 打印出牌字符串
    static void LogCardString(uint8_t cardData[], uint8_t cardCount);
    // 校验登陆码
    static bool VerifyPasswd(string& loginKey, uint32_t uid, uint32_t checkTime);

    // 判断是否重置信息
    static bool IsNeedReset(time_t lastTime, time_t curTime);

    // 原子修改离线玩家数据
    static bool AtomChangeOfflineAccData(uint32_t uid, uint16_t operType, uint16_t subType, int64_t coin, int64_t safecoin, const string &chessid = "");



};

