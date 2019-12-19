
#include <data_cfg_mgr.h>
#include "msg_lobby_handle.h"
#include "msg_define.pb.h"
#include "servers_msg.pb.h"
#include "game_define.h"
#include "game_player.h"
#include "player_mgr.h"
#include "game_table/game_room_mgr.h"
#include "net/center_client.h"

using namespace Network;
using namespace svrlib;
using namespace net;

CHandleLobbyMsg::CHandleLobbyMsg() {
    bind_handler(this, net::svr::L2GS_MSG_NOTIFY_NET_STATE, &CHandleLobbyMsg::handle_msg_notify_net_state);
    bind_handler(this, net::svr::L2GS_MSG_ENTER_INTO_SVR, &CHandleLobbyMsg::handle_msg_enter_svr);
    bind_handler(this, net::C2S_MSG_BACK_LOBBY, &CHandleLobbyMsg::handle_msg_back_lobby);

    bind_handler(this, net::C2S_MSG_REQ_ROOMS_INFO, &CHandleLobbyMsg::handle_msg_req_rooms_info);
    bind_handler(this, net::C2S_MSG_ENTER_ROOM, &CHandleLobbyMsg::handle_msg_enter_room);
    bind_handler(this, net::C2S_MSG_REQ_TABLE_LIST, &CHandleLobbyMsg::handle_msg_req_table_list);
    bind_handler(this, net::C2S_MSG_LEAVE_TABLE_REQ, &CHandleLobbyMsg::handle_msg_leave_table_req);
    bind_handler(this, net::C2S_MSG_ENTER_TABLE_REQ, &CHandleLobbyMsg::handle_msg_enter_table);
    bind_handler(this, net::C2S_MSG_TABLE_READY, &CHandleLobbyMsg::handle_msg_table_ready);
    bind_handler(this, net::C2S_MSG_TABLE_CHAT, &CHandleLobbyMsg::handle_msg_table_chat);
    bind_handler(this, net::C2S_MSG_TABLE_SET_AUTO, &CHandleLobbyMsg::handle_msg_table_set_auto);
    bind_handler(this, net::C2S_MSG_FAST_JOIN_ROOM, &CHandleLobbyMsg::handle_msg_fast_join_room);
    bind_handler(this, net::C2S_MSG_FAST_JOIN_TABLE, &CHandleLobbyMsg::handle_msg_fast_join_table);
    bind_handler(this, net::C2S_MSG_QUERY_TABLE_LIST_REQ, &CHandleLobbyMsg::handle_msg_query_table_list);
    bind_handler(this, net::C2S_MSG_SITDOWN_STANDUP, &CHandleLobbyMsg::handle_msg_sitdown_standup);
    bind_handler(this, net::svr::L2GS_MSG_FLUSH_CHANGE_ACC_DATA, &CHandleLobbyMsg::handle_msg_flush_change_acc_data);
}

CHandleLobbyMsg::~CHandleLobbyMsg() {

}

int CHandleLobbyMsg::OnRecvClientMsg() {
    LOG_DEBUG("收到大厅服务器 {} 消息:uin:{}--cmd:{}",m_connPtr->GetUID(), m_head->uin, m_head->msgID);
    if (CProtobufHandleBase::OnRecvClientMsg() == 1)
    {
        return handle_msg_gameing_oper();
    }
    return 0;
}

// 游戏内消息
int CHandleLobbyMsg::handle_msg_gameing_oper() {
    LOG_DEBUG("游戏内部消息:uid:{}--msg:{}", m_head->uin, m_head->msgID);
    auto pGamePlayer = GetGamePlayer();
    if (pGamePlayer == nullptr)
    {
        return 0;
    }
    pGamePlayer->OnMessage(m_head->msgID,m_pkt_buf, m_buf_len);

    return 0;
}

// 通知网络状态
int CHandleLobbyMsg::handle_msg_notify_net_state() {
    net::svr::msg_notify_net_state msg;
    PARSE_MSG(msg);
    uint32_t uid = msg.uid();
    uint8_t state = msg.state();
    uint32_t loginIp = msg.newip();
    uint8_t noPlayer = msg.no_player();
    LOG_DEBUG("通知网络状态:{}-->{}-->noplayer:{}", uid, state, noPlayer);
    auto pGamePlayer = GetGamePlayer();
    if (pGamePlayer != nullptr)
    {
        if (noPlayer == 1)
        {
            //pGamePlayer->SetNetStateNoPlayer();
        }
        else
        {
            pGamePlayer->SetIP(loginIp);
            pGamePlayer->SetNetState(state);
        }
    }
    return 0;
}

// 进入游戏服务器
int CHandleLobbyMsg::handle_msg_enter_svr() {
    net::svr::msg_enter_into_game_svr msg;
    PARSE_MSG(msg);
    uint32_t uid = m_head->uin;
    LOG_DEBUG("进入游戏服务器:{}", uid);

    auto pGamePlayer = GetGamePlayer();
    if (pGamePlayer != nullptr)
    {
        pGamePlayer->ReLogin();
    }
    else
    {
        if (msg.player_type() == PLAYER_TYPE_ONLINE)
        {
            pGamePlayer = std::make_shared<CGamePlayer>(PLAYER_TYPE_ONLINE);
            pGamePlayer->SetPlayerGameData(msg);
            pGamePlayer->OnLogin();
            CPlayerMgr::Instance().AddPlayer(pGamePlayer);
        }
        else
        {
            LOG_ERROR("error player type :{}", msg.player_type());
            return 0;
        }
    }
    pGamePlayer->SetLoginLobbySvrID(m_connPtr->GetUID());

    net::cli::msg_enter_gamesvr_rep msgrep;
    msgrep.set_result(RESULT_CODE_SUCCESS);
    msgrep.set_svrid(CApplication::Instance().GetServerID());
    msgrep.set_game_type(CDataCfgMgr::Instance().GetGameType());

    pGamePlayer->SendMsgToClient(&msgrep, net::S2C_MSG_ENTER_SVR_REP);

    return 0;
}

// 返回大厅
int CHandleLobbyMsg::handle_msg_back_lobby() {
    net::cli::msg_back_lobby_req msg;
    PARSE_MSG(msg);
    uint32_t uid = m_head->uin;
    LOG_DEBUG("请求返回大厅:{} -- {}", uid, msg.is_action());
    auto pGamePlayer = GetGamePlayer();
    if (pGamePlayer != nullptr)
    {
        if (!pGamePlayer->CanBackLobby())
        {
            net::cli::msg_back_lobby_rep rep;
            rep.set_result(RESULT_CODE_FAIL);
            rep.set_reason(RESULT_CODE_GAMEING);
            pGamePlayer->SendMsgToClient(&rep, net::S2C_MSG_BACK_LOBBY_REP);
            LOG_DEBUG("游戏状态中不能返回大厅");
            return 0;
        }
        else
        {
            CPlayerMgr::Instance().RecoverPlayer(pGamePlayer);
        }
        return 0;
    }
    else
    {
        if (msg.is_action() > 0)
        {
            LOG_ERROR("服务器主动请求返回大厅:{}");
            net::svr::msg_leave_svr msg;
            msg.set_uid(uid);
            ReplyMsg(&msg, net::svr::GS2L_MSG_LEAVE_SVR, uid);
        }
    }

    LOG_DEBUG("发送返回大厅消息:{}", uid);
    net::cli::msg_back_lobby_rep rep;
    rep.set_result(RESULT_CODE_SUCCESS);
    rep.set_reason(RESULT_CODE_SUCCESS);
    ReplyMsg(&rep, net::S2C_MSG_BACK_LOBBY_REP, uid);
    return 0;
}

// 请求房间列表信息
int CHandleLobbyMsg::handle_msg_req_rooms_info() {
    net::cli::msg_rooms_info_req msg;
    PARSE_MSG(msg);
    uint32_t uid = m_head->uin;
    LOG_DEBUG("请求房间列表信息:{}", uid);
    net::cli::msg_rooms_info_rep roomList;
    CGameRoomMgr::Instance().GetRoomList2Client(roomList,uid);
    ReplyMsg(&roomList, net::S2C_MSG_ROOMS_INFO,uid);
    LOG_DEBUG("Send Room List:{}", roomList.rooms_size());
    return 0;
}

// 进入房间
int CHandleLobbyMsg::handle_msg_enter_room() {
    net::cli::msg_enter_room_req msg;
    PARSE_MSG(msg);
    uint32_t uid = m_head->uin;
    uint16_t roomID = msg.room_id();
    LOG_DEBUG("请求进入房间:{}-{}", uid, roomID);
    auto pGamePlayer = GetGamePlayer();
    if (pGamePlayer == nullptr)
    {
        LOG_DEBUG("请求进入房间的玩家不存在");
        return 0;
    }
    net::cli::msg_enter_room_rep rep;
    auto pOldRoom = pGamePlayer->GetRoom();
    if (pOldRoom != nullptr && pOldRoom->GetRoomID() == roomID)
    {
        LOG_DEBUG("已经在房间中了:{}", roomID);
        pOldRoom->EnterRoom(pGamePlayer);
        return 0;
    }
    if (!pGamePlayer->CanBackLobby())
    {
        LOG_DEBUG("已经在座位上了不能换房:{}", uid);
        rep.set_result(0);
        pGamePlayer->SendMsgToClient(&rep, net::S2C_MSG_ENTER_ROOM_REP);
        return 0;
    }

    if (pOldRoom != nullptr)
    {
        pOldRoom->LeaveRoom(pGamePlayer);
    }
    auto pNewRoom = CGameRoomMgr::Instance().GetRoom(roomID);
    if (pNewRoom == nullptr || !pNewRoom->CanEnterRoom(pGamePlayer))
    {
        LOG_DEBUG("房间不存在或者不能进入:uid:{}--room:{}", uid, roomID);
        rep.set_result(0);
        pGamePlayer->SendMsgToClient(&rep, net::S2C_MSG_ENTER_ROOM_REP);
        return 0;
    }
    pNewRoom->EnterRoom(pGamePlayer);

    return 0;
}

// 请求桌子列表
int CHandleLobbyMsg::handle_msg_req_table_list() {
    net::cli::msg_table_list_req msg;
    PARSE_MSG(msg);
    auto pGamePlayer = GetGamePlayer();
    if (pGamePlayer == nullptr)
        return 0;
    auto pRoom = pGamePlayer->GetRoom();
    if (pRoom == nullptr)
    {
        LOG_DEBUG("玩家没有进入房间:{}", pGamePlayer->GetUID());
        return 0;
    }
    LOG_DEBUG("请求桌子列表:{},{}", pGamePlayer->GetUID(), msg.DebugString());
    pRoom->SendTableListToPlayer(pGamePlayer,0);
    return 0;
}

// 离开桌子
int CHandleLobbyMsg::handle_msg_leave_table_req() {
    net::cli::msg_leave_table_req msg;
    PARSE_MSG(msg);
    uint32_t uid = m_head->uin;
    LOG_DEBUG("请求离开桌子:{}", uid);
    auto pGamePlayer = GetGamePlayer();
    if (pGamePlayer == nullptr)
        return 0;
    auto pTable = pGamePlayer->GetTable();
    if (pTable == nullptr)
    {
        LOG_DEBUG("不在桌子上:{}", uid);
        return 0;
    }
    auto pRoom = pGamePlayer->GetRoom();
    net::cli::msg_leave_table_rep rep;
    if (pTable->CanLeaveTable(pGamePlayer))
    {
        pTable->LeaveTable(pGamePlayer);
        rep.set_result(1);
        if (pRoom != nullptr)
        {
            pRoom->LeaveRoom(pGamePlayer);
        }
    }
    else
    {
        LOG_DEBUG("不能离开桌子:{}", uid);
        rep.set_result(0);
    }
    pGamePlayer->SendMsgToClient(&rep, net::S2C_MSG_LEAVE_TABLE_REP);

    return 0;
}

// 进入桌子
int CHandleLobbyMsg::handle_msg_enter_table() {
    net::cli::msg_enter_table_req msg;
    PARSE_MSG(msg);
    uint32_t uid = m_head->uin;
    LOG_DEBUG("req enter table:uid:{}-->tblid:{}-->passwd:{}", uid, msg.table_id());
    auto pGamePlayer = GetGamePlayer();
    if (pGamePlayer == nullptr)
        return 0;
    auto pRoom = pGamePlayer->GetRoom();
    if (pRoom == nullptr)
    {
        LOG_DEBUG("change table not in room:{}", uid);
        return 0;
    }
    uint8_t bRet = pRoom->EnterTable(pGamePlayer, msg.table_id());
    if (msg.table_id() != 0 && bRet == RESULT_CODE_SUCCESS)
        return 0;

    net::cli::msg_enter_table_rep rep;
    rep.set_result(bRet);
    rep.set_table_id(msg.table_id());
    pGamePlayer->SendMsgToClient(&rep, net::S2C_MSG_ENTER_TABLE);
    return 0;
}

// 桌子准备
int CHandleLobbyMsg::handle_msg_table_ready() {
    net::cli::msg_table_ready_req msg;
    PARSE_MSG(msg);
    auto pGamePlayer = GetGamePlayer();
    if (pGamePlayer == nullptr)
    {
        LOG_DEBUG("玩家对象为NULL");
        return 0;
    }
    LOG_DEBUG("桌子准备:{}", pGamePlayer->GetUID());
    auto pTable = pGamePlayer->GetTable();
    if (pTable != nullptr)
    {
        if(pTable->GetHostRoom()->IsNeedMarry()){
            LOG_ERROR("need marry table is can't ready");
            return 0;
        }
        pTable->PlayerReady(pGamePlayer, msg.ready() > 0 ? true : false);
        pGamePlayer->SetAutoReady(false);
    }
    else
    {
        LOG_ERROR("table is not exist ,player ready table is null");
    }
    return 0;
}

// 桌子聊天
int CHandleLobbyMsg::handle_msg_table_chat() {
    net::cli::msg_table_chat_req msg;
    PARSE_MSG(msg);

    auto pGamePlayer = GetGamePlayer();
    if (pGamePlayer == nullptr)
        return 0;

    auto pTable = pGamePlayer->GetTable();
    if (pTable != nullptr)
    {
        net::cli::msg_table_chat_rep rep;
        rep.set_uid(pGamePlayer->GetUID());
        rep.set_chat_msg(msg.chat_msg());
        pTable->TableMsgToAll(&rep, net::S2C_MSG_TABLE_CHAT, false);
    }
    return 0;
}

// 桌子设置托管
int CHandleLobbyMsg::handle_msg_table_set_auto() {
    net::cli::msg_table_set_auto_req msg;
    PARSE_MSG(msg);
    LOG_DEBUG("桌子设置托管:{}", msg.auto_type());
    auto pGamePlayer = GetGamePlayer();
    if (pGamePlayer == nullptr)
        return 0;

    auto pTable = pGamePlayer->GetTable();
    if (pTable != nullptr)
    {
        pTable->PlayerSetAuto(pGamePlayer, msg.auto_type());
    }
    return 0;
}

// 快速开始
int CHandleLobbyMsg::handle_msg_fast_join_room() {
    net::cli::msg_fast_join_room_req msg;
    PARSE_MSG(msg);
    LOG_DEBUG("快速加入房间");
    auto pGamePlayer = GetGamePlayer();
    if (pGamePlayer == nullptr)
        return 0;
    net::cli::msg_fast_join_room_rep rep;

    uint8_t bRet = CGameRoomMgr::Instance().FastJoinRoom(pGamePlayer)
                   ? RESULT_CODE_SUCCESS
                   : RESULT_CODE_FAIL;
    rep.set_result(bRet);
    pGamePlayer->SendMsgToClient(&rep, net::S2C_MSG_FAST_JOIN_ROOM_REP);
    return 0;
}

// 快速换桌
int CHandleLobbyMsg::handle_msg_fast_join_table() {
    LOG_DEBUG("快速换桌");
    net::cli::msg_fast_join_table_req msg;
    PARSE_MSG(msg);
    auto pGamePlayer = GetGamePlayer();
    if (pGamePlayer == nullptr)
        return 0;
    net::cli::msg_fast_join_table_rep rep;
    rep.set_result(0);
    auto pRoom = pGamePlayer->GetRoom();
    if (pRoom != nullptr)
    {
        if (pRoom->FastJoinTable(pGamePlayer))
        {
            rep.set_result(1);
        }
    }
    pGamePlayer->SendMsgToClient(&rep, net::S2C_MSG_FAST_JOIN_TABLE_REP);
    return 0;
}

// 查看桌子信息
int CHandleLobbyMsg::handle_msg_query_table_list() {
    net::cli::msg_query_table_list_req msg;
    PARSE_MSG(msg);
    uint32_t uid = m_head->uin;
    LOG_DEBUG("查看桌子信息:{}", uid);
    auto pGamePlayer = GetGamePlayer();
    if (pGamePlayer == nullptr)
        return 0;
    auto pRoom = pGamePlayer->GetRoom();
    if (pRoom == nullptr)
    {
        LOG_DEBUG("玩家没有进入房间:{}", uid);
        return 0;
    }
    uint32_t startID = msg.startid();
    uint32_t endID = msg.endid();
    pRoom->QueryTableListToPlayer(pGamePlayer, startID, endID);

    return 0;
}

// 站立做起
int CHandleLobbyMsg::handle_msg_sitdown_standup() {
    if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
    {
        LOG_ERROR("维护状态,直接掉线切服");
        return 0;
    }
    net::cli::msg_sitdown_standup_req msg;
    PARSE_MSG(msg);
    LOG_DEBUG("站立坐下:{}", msg.oper_id());
    auto pGamePlayer = GetGamePlayer();
    if (pGamePlayer == nullptr)
        return 0;

    auto pTable = pGamePlayer->GetTable();
    if (pTable != nullptr)
    {
        net::cli::msg_sitdown_standup_rep rep;

        if (pTable->PlayerSitDownStandUp(pGamePlayer, msg.oper_id(), msg.chair_id()))
        {
            rep.set_result(net::RESULT_CODE_SUCCESS);
        }
        else
        {
            rep.set_result(net::RESULT_CODE_FAIL);
        }

        rep.set_oper_id(msg.oper_id());
        rep.set_chair_id(msg.chair_id());
        pGamePlayer->SendMsgToClient(&rep, net::S2C_MSG_SITDOWN_STANDUP);
    }
    return 0;
}
//刷新数据变更
int CHandleLobbyMsg::handle_msg_flush_change_acc_data()
{
    net::svr::msg_flush_change_account_data msg;
    PARSE_MSG(msg);
    auto pGamePlayer = GetGamePlayer();
    if (pGamePlayer == nullptr)
        return 0;

    pGamePlayer->ChangeAccountValue(emACC_VALUE_COIN, msg.coin());
    pGamePlayer->ChangeAccountValue(emACC_VALUE_SAFECOIN, msg.safe_coin());
    pGamePlayer->SetLat(msg.lat());
    pGamePlayer->SetLon(msg.lon());

    return 0;
}

std::shared_ptr<CGamePlayer> CHandleLobbyMsg::GetGamePlayer() {
    auto pPlayer = CPlayerMgr::Instance().GetPlayer<CGamePlayer>(m_head->uin);
    if (pPlayer == nullptr)
    {
        LOG_DEBUG("游戏玩家不存在:{}", m_head->uin);
    }
    else
    {
        pPlayer->ResetHeart();
    }
    return pPlayer;
}

void CHandleLobbyMsg::ReplyMsg(const google::protobuf::Message *msg, uint16_t msg_type, uint32_t uin) {
    pkg_inner::SendProtobufMsg(m_connPtr, msg, msg_type, uin, 0, 0);
}









