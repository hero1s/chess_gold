

#include "game_server_mgr.h"
#include "msg_define.pb.h"
#include "game_define.h"
#include "utility/profile_manager.h"
#include "player.h"
#include "player_mgr.h"
#include "common_logic.h"
#include "servers_msg.pb.h"
#include "error_code.pb.h"
#include "client_logic_msg.pb.h"

using namespace svrlib;
using namespace Network;

namespace {

}

//--------------------------------------------------------------------------------------------
CGameServerMgr::CGameServerMgr() {
    bind_handler(this, net::svr::GS2L_MSG_REPORT, &CGameServerMgr::handle_msg_report);
    bind_handler(this, net::svr::GS2L_MSG_LEAVE_SVR, &CGameServerMgr::handle_msg_leave_svr);
    bind_handler(this, net::svr::GS2L_MSG_NOTIFY_CHANGE_ACCOUNT_DATA, &CGameServerMgr::handle_msg_notify_change_account_data);
    bind_handler(this, net::svr::GS2L_MSG_REPORT_GAME_RESULT, &CGameServerMgr::handle_msg_report_game_result);
}

CGameServerMgr::~CGameServerMgr() {
}

bool CGameServerMgr::Init() {
    CServerClientMgr::Init();
    return true;
}

void CGameServerMgr::ShutDown() {
    CServerClientMgr::ShutDown();
}

// 更新服务器列表给玩家
void CGameServerMgr::UpdateServerList2Client(std::shared_ptr<CPlayer> pPlayer) {
    if (pPlayer == nullptr)
        return;
    net::cli::msg_svrs_info_rep info;
    info.set_cur_svrid(pPlayer->GetGameSvrID());
    info.set_cur_game_type(0);
    if (pPlayer->GetGameSvrID() > 0)
    {
        auto pCurSvr = GetServerBySvrID(pPlayer->GetGameSvrID());
        if (pCurSvr)info.set_cur_game_type(pCurSvr->GetGameType());
    }
    vector<shared_ptr<CServerClient>> svrlist;
    GetAllServerInfo(svrlist);
    for (auto &it : svrlist)
    {
        net::svr_info *pSvr = info.add_svrs();
        pSvr->set_svrid(it->GetSvrID());
        pSvr->set_svr_type(it->GetSvrType());
        pSvr->set_game_type(it->GetGameType());
        auto plays = it->GetAllPlayType();
        for (uint16_t i = 0; i < plays.size(); ++i)
        {
            pSvr->add_play_types(plays[i]);
        }
        pSvr->set_status(it->GetStatus());
    }
    pPlayer->SendMsgToClient(&info, net::S2C_MSG_SVRS_INFO);
    LOG_DEBUG("发送服务器列表:{}--{},cursvr:{}", pPlayer->GetUID(), info.svrs_size(), info.cur_svrid());
}

// 给玩家选择相应游戏服务器
shared_ptr<CServerClient> CGameServerMgr::SelectGameTypeServer(std::shared_ptr<CPlayer> pPlayer, uint16_t gameType) {
    for (auto &it : m_mpServers)
    {
        auto pSvr = it.second;
        if (pSvr->GetGameType() == gameType)
        {
            if (!pSvr->IsRepair())
            {//新玩家进入正常服务器
                if (pPlayer->GetGameSvrID() == 0 || pPlayer->GetGameSvrID() == pSvr->GetSvrID()) return pSvr;
            }
            else
            {//维护中服务器只接受断线重连
                if (pPlayer->GetGameSvrID() == pSvr->GetSvrID()) return pSvr;
            }
        }
    }
    LOG_DEBUG("select game server is null,uid:{},gameType {}", pPlayer->GetUID(), gameType);
    return nullptr;
}

int CGameServerMgr::OnRecvClientMsg() {
    if (CProtobufHandleBase::OnRecvClientMsg() == 1)
    {
        return route_to_client();
    }
    return 0;
}

// 转发给客户端
int CGameServerMgr::route_to_client() {
    LOG_DEBUG("转发给客户端消息:uid:{}--cmd:{}", m_head->uin, m_head->msgID);
    auto pPlayer = GetPlayer();
    if (pPlayer != nullptr)
    {
        pPlayer->SendMsgToClient(m_pkt_buf, m_buf_len, m_head->msgID);
    }
    else
    {
        LOG_DEBUG("转发消息客户端不存在，通知游戏服断线:{}", m_head->uin);
        net::svr::msg_notify_net_state msg;
        msg.set_uid(m_head->uin);
        msg.set_state(0);
        msg.set_newip(0);
        msg.set_no_player(1);
        pkg_inner::SendProtobufMsg(m_connPtr, &msg, net::svr::L2GS_MSG_NOTIFY_NET_STATE, m_head->uin, 0, 0);
    }
    return 0;
}

// 服务器上报信息
int CGameServerMgr::handle_msg_report() {
    net::svr::msg_report_svr_info msg;
    PARSE_MSG(msg);

    uint32_t players = msg.onlines();

    //LOG_DEBUG("游戏服上报信息:sid {}--{}",m_connPtr->GetUID(),players);

    return 0;
}

// 返回大厅
int CGameServerMgr::handle_msg_leave_svr() {
    net::svr::msg_leave_svr msg;
    PARSE_MSG(msg);

    uint32_t uid = msg.uid();
    LOG_DEBUG("通知返回大厅:{}", uid);
    auto pPlayer = GetPlayer();
    if (pPlayer != nullptr)
    {
        pPlayer->BackLobby();
        pPlayer->NotifyClientBackLobby(RESULT_CODE_SUCCESS, RESULT_CODE_SUCCESS);
    }
    else
    {
        LOG_DEBUG("返回大厅玩家不存在:{}", uid);
    }
    return 0;
}

// 修改玩家数值
int CGameServerMgr::handle_msg_notify_change_account_data() {
    net::svr::msg_notify_change_account_data msg;
    PARSE_MSG(msg);
    uint32_t uid = msg.uid();
    int64_t coin = msg.coin();
    int64_t safeCoin = msg.safe_coin();
    int32_t operType = msg.oper_type();
    int32_t subType = msg.sub_type();
    string chessid = msg.chessid();
    LOG_DEBUG("change value {} 数值:coin:{},safecoin:{}", uid, coin, safeCoin);
    auto pPlayer = GetPlayer();
    if (pPlayer != nullptr && pPlayer->CanModifyData())
    {
        pPlayer->SyncChangeAccountValue(operType, subType, coin, safeCoin, chessid);
        pPlayer->UpdateAccValue2Client();
    }
    else
    {
        CCommonLogic::AtomChangeOfflineAccData(uid, operType, subType, coin, safeCoin, chessid);
    }
    return 0;
}

// 上报游戏结果
int CGameServerMgr::handle_msg_report_game_result() {
    net::svr::msg_report_game_result msg;
    PARSE_MSG(msg);
    uint32_t gameType = msg.game_type();
    uint32_t playType = msg.play_type();
    uint32_t uid = msg.uid();

    int32_t win = msg.win_score() > 0 ? 1 : 0;
    int32_t lose = (win == 1) ? 0 : 1;
    uint32_t cate1 = gameType;        //游戏类型
    uint32_t cate2 = playType;        //玩法类型
    uint32_t cate3 = 0;               //房间类别
    uint32_t cate4 = 0;               //房间级别
    LOG_DEBUG("上报游戏结果:{},gameType {},playType {}", uid,gameType,playType);

    auto pPlayer = GetPlayer();
    if (pPlayer == nullptr)
    {
        return 0;
    }
    if (!pPlayer->CanModifyData())
        return 0;

    // 激活任务
    pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_PLAY, 1, cate1, cate2, cate3, cate4);
    pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_WIN, win, cate1, cate2, cate3, cate4);

    pPlayer->GetMissionMgr().SaveMiss();
    return 0;
}


std::shared_ptr<CPlayer> CGameServerMgr::GetPlayer() {
    auto pPlayer = CPlayerMgr::Instance().GetPlayer<CPlayer>(m_head->uin);
    return pPlayer;
}




















