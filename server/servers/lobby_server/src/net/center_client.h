
#pragma once

#include "svrlib.h"
#include <string.h>
#include "packet/inner_protobuf_pkg.h"
#include <unordered_map>
#include "servers_msg.pb.h"
#include "server_connect/server_connector.h"
#include "player.h"

using namespace std;
using namespace svrlib;
using namespace Network;

class CCenterClientMgr : public CSvrConnectorMgr, public AutoDeleteSingleton<CCenterClientMgr> {
public:
    CCenterClientMgr();

    virtual ~CCenterClientMgr();

    virtual int OnRecvClientMsg();

protected:
    // 转发给客户端
    int route_to_client();

    // 玩家登录通知
    int handle_msg_player_login_lobby();

    std::shared_ptr<CPlayer> GetPlayer();
};
/*
 * 大厅服务器-》》中心服的管理及消息处理
*/

