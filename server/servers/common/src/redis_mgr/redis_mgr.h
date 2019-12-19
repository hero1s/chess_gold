
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
/*----------------------------------------�߼��ӿ�begin--------------------------------------------------------------------*/
    // �������������Ϣ
    void SetPlayerOnlineInfo(uint32_t uid, string field, int64_t value);

    int64_t GetPlayerOnlineInfo(uint32_t uid, string field);

    int64_t IncrbyPlayerOnlineInfo(uint32_t uid, string field, int64_t increment, bool needRet);

/*----------------------------------------�߼��ӿ�end----------------------------------------------------------------------*/
    //  �������ݵ���˶���
    void PushDataToBackQueue(string queueName, json &jsonValue);


protected:
    // �Զ����·�����״̬
    void AutoUpdateServerStatus();


};



























































































































































