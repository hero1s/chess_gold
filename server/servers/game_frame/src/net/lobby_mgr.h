
#pragma once

#include "utility/noncopyable.hpp"
#include "svrlib.h"
#include <string.h>
#include "servers_msg.pb.h"
#include "server_connect/server_connector.h"
#include "packet/inner_protobuf_pkg.h"
#include <unordered_map>

using namespace std;
using namespace svrlib;
using namespace Network;

//大厅连接
class CLobbyClient : public CSvrConnectorMgr {
public:
    CLobbyClient();

    virtual ~CLobbyClient();

};

/**
 * 管理大厅服务器
 */
class CLobbyMgr : public AutoDeleteSingleton<CLobbyMgr> {
public:
    CLobbyMgr();

    void OnTimer();

    bool Init();

    // 连接大厅服
    bool ConnectLobby(string ip, int32_t port, uint16_t sid);

    // 发送消息到客户端
    bool SendMsg2Client(const google::protobuf::Message *msg, uint16_t msg_type, uint32_t uin, uint16_t svrid);

    // 发送消息到大厅
    bool SendMsg2Lobby(const google::protobuf::Message *msg, uint16_t msg_type, uint16_t svrid);

    // 发送消息到全部大厅
    bool SendMsg2AllLobby(const google::protobuf::Message *msg, uint16_t msg_type);

    // 通知大厅修改数值
    void NotifyLobbyChangeAccValue(uint32_t uid, int32_t operType, int32_t subType, int64_t coin, int64_t safeCoin, const string& chessid = "");
private:
    std::shared_ptr<CLobbyClient> GetLobbySvrBySvrID(uint16_t svrid);

    void ReportInfo2Lobby();

private:
    using MAP_LOBBY = unordered_map<uint32_t, std::shared_ptr<CLobbyClient>>;
    MAP_LOBBY m_lobbySvrs;// 大厅服务器
    MemberTimerEvent<CLobbyMgr, &CLobbyMgr::OnTimer> m_timer;

};










