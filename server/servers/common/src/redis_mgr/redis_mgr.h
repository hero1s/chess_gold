
#pragma once

#include "svrlib.h"
#include "config/config.h"
#include <string>
#include "redisclient/redis_client_wrap.h"
#include "asio.hpp"
#include "nlohmann/json_wrap.h"

using namespace std;
using namespace svrlib;

class CRedisMgr : public CRedisWrap, public AutoDeleteSingleton<CRedisMgr> {
public:
    CRedisMgr();

    virtual ~CRedisMgr();

    virtual void OnTimer();

public:


public:
/*----------------------------------------逻辑接口begin--------------------------------------------------------------------*/
    // 设置玩家在线信息
    void SetPlayerOnlineInfo(uint32_t uid, string field, int64_t value);

    int64_t GetPlayerOnlineInfo(uint32_t uid, string field);

    int64_t IncrbyPlayerOnlineInfo(uint32_t uid, string field, int64_t increment, bool needRet);

/*----------------------------------------逻辑接口end----------------------------------------------------------------------*/
    //  推送数据到后端队列
    void PushDataToBackQueue(string queueName, json &jsonValue);


protected:
    // 自动更新服务器状态
    void AutoUpdateServerStatus();


};



























































































































































