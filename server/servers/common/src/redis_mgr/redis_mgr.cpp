
#include <game_define.h>
#include "servers_msg.pb.h"
#include "redis_mgr.h"
#include "utility/puid.hpp"
#include "snappy/snappy.h"

using namespace std;
using namespace svrlib;

namespace {
    const static char *s_key_onlineinfo = "ONLINEINFO";                // 玩家信息
    const static char *s_key_serverstatus = "SERVERSTATUS";            // 服务器状态
};

CRedisMgr::CRedisMgr() {
}

CRedisMgr::~CRedisMgr() {
}

void CRedisMgr::OnTimer() {
    AUTOPROFILE("CRedisMgr::OnTimer");
    CRedisWrap::OnTimer();
    AutoUpdateServerStatus();

    //test toney
    json jvalue;
    jvalue["time"] = time::getSysTime();
    jvalue["uid"] = 110;
    //PushDataToBackQueue("testqueue",jvalue);

}
/*----------------------------------------逻辑接口begin----------------------------------------------------------------*/
// 设置玩家在线信息
void CRedisMgr::SetPlayerOnlineInfo(uint32_t uid, string field, int64_t value) {
    string key = CStringUtility::FormatToString("%s-%d", s_key_onlineinfo, uid);
    SafeAsyncCommond("hset", {key, field, value});
    SafeAsyncCommond("expire", {key, TimeConstants::DAY});
}

int64_t CRedisMgr::GetPlayerOnlineInfo(uint32_t uid, string field) {
    string key = CStringUtility::FormatToString("%s-%d", s_key_onlineinfo, uid);
    int64_t valID = 0;
    auto result = SafeSyncCommond("hget", {key, field});
    if (result.isOk())
    {
        valID = result.toInt();
    }
    else
    {
        LOG_ERROR("Get PlayerInfo fail:uid {}--filed:{},{}", uid, field, result.toString());
    }

    LOG_DEBUG("Get PlayerInfo {} field:{}--:{}", uid, field, valID);
    return valID;
}

int64_t CRedisMgr::IncrbyPlayerOnlineInfo(uint32_t uid, string field, int64_t increment, bool needRet) {
    string key = CStringUtility::FormatToString("%s-%d", s_key_onlineinfo, uid);
    SafeAsyncCommond("expire", {key, TimeConstants::DAY});
    int64_t retval = 0;
    if (needRet)
    {
        auto result = SafeSyncCommond("hincrby", {key, field, increment});
        if (result.isOk())
        {
            retval = result.toInt();
        }
    }
    else
    {
        SafeAsyncCommond("hincrby", {key, field, increment});
    }
    return retval;
}

/*----------------------------------------逻辑接口end------------------------------------------------------------------*/

//  推送数据到后端队列
void CRedisMgr::PushDataToBackQueue(string queueName, json &jsonValue) {
    SafeAsyncCommond("LPUSH", {queueName, jsonValue.dump()});
}

// 自动更新服务器状态
void CRedisMgr::AutoUpdateServerStatus() {
    string key = CStringUtility::FormatToString("%s", s_key_serverstatus);
    uint16_t sid = CApplication::Instance().GetServerID();
    SafeAsyncCommond("hget", {key, sid}, [this](const redisclient::RedisValue &v)
    {
        uint8_t status = 0;
        if (v.isOk())
        {
            status = v.toInt();
        }
        else
        {
            LOG_ERROR("获取服务器状态信息失败:sid {},{}", CApplication::Instance().GetServerID(), v.toString());
            return;
        }
        if (status != CApplication::Instance().GetStatus())
        {
            CApplication::Instance().SetStatus(status);
            LOG_DEBUG("server status change:{}", status);
        }
    });
}