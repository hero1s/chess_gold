
#pragma once

#include <vector>
#include "svrlib.h"
#include <unordered_map>
#include <memory>
#include "packet/inner_protobuf_pkg.h"
#include "servers_msg.pb.h"
#include "server_connect/server_client.h"

using namespace std;
using namespace svrlib;
using namespace Network;

// 中心服管理器
class CCenterMgr : public CServerClientMgr, public AutoDeleteSingleton<CCenterMgr> {
public:
    CCenterMgr();

    virtual ~CCenterMgr();

    virtual bool Init();

    virtual void ShutDown();

private:


};

/*
 * 游戏服务器连接中心服务器的管理及消息处理
*/


































