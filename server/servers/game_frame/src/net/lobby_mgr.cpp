#include <data_cfg_mgr.h>
#include "lobby_mgr.h"
#include "game_server_config.h"
#include "game_define.h"
#include "net/msg_lobby_handle.h"
#include "player_mgr.h"

using namespace svrlib;
using namespace std;
using namespace Network;
using namespace net;

//大厅连接
CLobbyClient::CLobbyClient() {

}

CLobbyClient::~CLobbyClient() {

}


CLobbyMgr::CLobbyMgr()
        : m_timer(this) {

}

void CLobbyMgr::OnTimer() {
    ReportInfo2Lobby();
    CApplication::Instance().schedule(&m_timer, 3000);
}

bool CLobbyMgr::Init() {
    CApplication::Instance().schedule(&m_timer, 3000);

    SOL_CALL_LUA(CApplication::Instance().GetSolLuaState()["init_lobby_mgr"]());

    return true;
}

// 连接大厅服
bool CLobbyMgr::ConnectLobby(string ip, int32_t port, uint16_t sid) {

    net::svr::server_info info;
    info.set_svrid(CApplication::Instance().GetServerID());
    info.set_game_type(CDataCfgMgr::Instance().GetGameType());
    auto plays = CDataCfgMgr::Instance().GetGamePlayType();
    for (uint16_t i = 0; i < plays.size(); ++i)
    {
        info.add_play_types(plays[i]);
    }
    info.set_svr_type(emSERVER_TYPE_GAME);
    info.set_uuid(CApplication::Instance().GetUUID());

    //连接大厅服
    auto lobbyClient = std::make_shared<CLobbyClient>();
    if (lobbyClient->Init(info, ip, port, "lobby_connector", sid) == false)
    {
        LOG_ERROR("init lobby client mgr fail");
        return false;
    }
    //添加派发消息handle
    lobbyClient->RegisterSubHandle(std::make_shared<CHandleLobbyMsg>());
    m_lobbySvrs.insert(make_pair(lobbyClient->GetSvrID(), lobbyClient));
    return true;
}

bool CLobbyMgr::SendMsg2Client(const google::protobuf::Message *msg, uint16_t msg_type, uint32_t uin, uint16_t svrid) {
    auto pSvr = GetLobbySvrBySvrID(svrid);
    if (pSvr != nullptr)
    {
        pSvr->SendMsg2Svr(msg, msg_type, uin);
        return true;
    }
    return false;
}

bool CLobbyMgr::SendMsg2Lobby(const google::protobuf::Message *msg, uint16_t msg_type, uint16_t svrid) {
    auto pSvr = GetLobbySvrBySvrID(svrid);
    if (pSvr == nullptr)
        return false;
    pSvr->SendMsg2Svr(msg, msg_type);
    return true;
}

bool CLobbyMgr::SendMsg2AllLobby(const google::protobuf::Message *msg, uint16_t msg_type) {
    for (auto &it : m_lobbySvrs)
    {
        auto pSvr = it.second;
        pSvr->SendMsg2Svr(msg, msg_type);
    }
    return true;
}

// 请求大厅修改数值
void CLobbyMgr::NotifyLobbyChangeAccValue(uint32_t uid, int32_t operType, int32_t subType, int64_t coin, int64_t safeCoin,
                                          const string& chessid)
{
    net::svr::msg_notify_change_account_data msg;
    msg.set_uid(uid);
    msg.set_coin(coin);
    msg.set_safe_coin(safeCoin);
    msg.set_oper_type(operType);
    msg.set_sub_type(subType);
    msg.set_chessid(chessid);

    SendMsg2Client(&msg, net::svr::GS2L_MSG_NOTIFY_CHANGE_ACCOUNT_DATA, uid, 0);
}

std::shared_ptr<CLobbyClient> CLobbyMgr::GetLobbySvrBySvrID(uint16_t svrid) {
    auto it = m_lobbySvrs.find(svrid);
    if (it == m_lobbySvrs.end())return nullptr;
    return it->second;
}

void CLobbyMgr::ReportInfo2Lobby() {
    net::svr::msg_report_svr_info info;

    uint32_t players = CPlayerMgr::Instance().GetOnlines();
    info.set_onlines(players);

    SendMsg2AllLobby(&info, net::svr::GS2L_MSG_REPORT);
}













