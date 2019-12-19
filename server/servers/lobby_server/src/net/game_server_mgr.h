
#pragma once

#include <vector>
#include "svrlib.h"
#include <unordered_map>
#include <memory>
#include "packet/inner_protobuf_pkg.h"
#include "servers_msg.pb.h"
#include "server_connect/server_client.h"
#include "player.h"

using namespace std;
using namespace svrlib;
using namespace Network;

// 游戏服务管理器
class CGameServerMgr : public CServerClientMgr, public AutoDeleteSingleton<CGameServerMgr> {
public:
    CGameServerMgr();

    virtual ~CGameServerMgr();

    virtual int OnRecvClientMsg();

    virtual bool Init();

    virtual void ShutDown();

    // 更新服务器列表给玩家
    void UpdateServerList2Client(std::shared_ptr<CPlayer> pPlayer);

    // 给玩家选择相应游戏服务器
    shared_ptr<CServerClient> SelectGameTypeServer(std::shared_ptr<CPlayer> pPlayer, uint16_t gameType);

private:
    // 转发给客户端
    int route_to_client();

    // 服务器上报信息
    int handle_msg_report();

    // 返回大厅
    int handle_msg_leave_svr();

    // 修改玩家数值
    int handle_msg_notify_change_account_data();

    // 上报游戏结果
    int handle_msg_report_game_result();

    std::shared_ptr<CPlayer> GetPlayer();
};
/*
 * 游戏服务器连接大厅服务器的管理及消息处理
*/


































