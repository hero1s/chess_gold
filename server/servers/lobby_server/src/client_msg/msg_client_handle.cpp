

#include "msg_client_handle.h"
#include "data_cfg_mgr.h"
#include "game_server_config.h"
#include "player.h"
#include "player_mgr.h"
#include "common_logic.h"
#include "server_connect/server_client.h"
#include "net/game_server_mgr.h"
#include "msg_define.pb.h"
#include "client_logic_msg.pb.h"

using namespace Network;
using namespace svrlib;
using namespace std;

namespace {

};

CHandleClientMsg::CHandleClientMsg() {
    bind_handler(this, net::C2S_MSG_HEART, &CHandleClientMsg::handle_msg_heart);
    bind_handler(this, net::C2S_MSG_LOGIN, &CHandleClientMsg::handle_msg_login);
    bind_handler(this, net::C2S_MSG_REQ_SVRS_INFO, &CHandleClientMsg::handle_msg_req_svrs_info);
    bind_handler(this, net::C2S_MSG_ENTER_SVR, &CHandleClientMsg::handle_msg_enter_gamesvr);

    bind_handler(this, net::C2S_MSG_REPORT_GPS, &CHandleClientMsg::handle_msg_report_gps);
    bind_handler(this, net::C2S_MSG_REPORT_NET_DELAY, &CHandleClientMsg::handle_msg_report_net_delay);
    bind_handler(this, net::C2S_MSG_REQ_ROOMS_INFO, &CHandleClientMsg::handle_msg_req_rooms_info);
}

CHandleClientMsg::~CHandleClientMsg() {

}

int CHandleClientMsg::OnRecvClientMsg() {
    AutoProfile("CHandleClientMsg::OnRecvClientMsg");
    auto ret = CProtobufHandleBase::OnRecvClientMsg();
    if (ret == 1)
    {
        return route_to_game_svr();
    }
    else if (ret < 0)
    {
        LOG_ERROR("handle client msg < 0,close");
        m_connPtr->Close();
    }
    return 0;
}

#ifndef CHECK_PLAYER_PLAY
#define CHECK_PLAYER_PLAY   \
    auto pPlayer = GetPlayer(m_connPtr);\
    if(pPlayer == nullptr || !pPlayer->IsPlaying())\
        return -1;
#endif

// 转发给游戏服
int CHandleClientMsg::route_to_game_svr() {
    CHECK_PLAYER_PLAY

    if (pPlayer->GetGameSvrID() > 0)
    {
        pPlayer->SendMsgToGameSvr(m_pkt_buf, m_buf_len, m_head->msgID);
    }
    else
    {
        LOG_DEBUG("玩家不在游戏服uid:{}--cursid {}--cmd:{}", pPlayer->GetUID(), pPlayer->GetGameSvrID(), m_head->msgID);
    }
    return 0;
}

// 心跳包
int CHandleClientMsg::handle_msg_heart() {
    LOG_DEBUG("heart msg:{}", m_connPtr->GetUID());
    net::cli::msg_heart_test msg;
    msg.set_svr_time(time::getSysTime());
    pkg_client::SendProtobufMsg(m_connPtr, &msg, net::C2S_MSG_HEART);
    LOG_DEBUG("send heart msg:{}", msg.DebugString());
    return 0;
}

//登录
int CHandleClientMsg::handle_msg_login() {
    net::cli::msg_login_req msg;
    PARSE_MSG(msg);

    LOG_ERROR("recv player login msg:{}", msg.DebugString());
    uint32_t uid = msg.uid();
    net::cli::msg_login_rep repmsg;
    repmsg.set_server_time(time::getSysTime());

    string strDecyPHP = msg.key();
    auto pPlayerObj = CPlayerMgr::Instance().GetPlayer<CPlayer>(m_connPtr->GetUID());
    auto pPlayerUid = CPlayerMgr::Instance().GetPlayer<CPlayer>(uid);

    // 校验密码
    if (pPlayerUid == nullptr || pPlayerUid->GetLoginKey() != strDecyPHP)
    {
        auto bRet = CCommonLogic::VerifyPasswd(strDecyPHP, uid, msg.check_time());
        if (!bRet || std::abs(int64_t(time::getSysTime()) - int64_t(msg.check_time())) > TimeConstants::DAY)
        {
            LOG_ERROR("the ip is:{},svrtime:{},sendtime:{}", m_connPtr->GetRemoteAddress(), time::getSysTime(), msg.check_time());
            repmsg.set_result(-1);
            pkg_client::SendProtobufMsg(m_connPtr, &repmsg, net::S2C_MSG_LOGIN_REP);
            return -1;
        }
    }

    //维护状态，只有在玩玩家可以进
    if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
    {
        if (pPlayerUid == nullptr)
        {
            repmsg.set_result(0);
            pkg_client::SendProtobufMsg(m_connPtr, &repmsg, net::S2C_MSG_LOGIN_REP);
            LOG_ERROR("服务器维护状态，只有在玩玩家能进入");
            m_connPtr->Close();
            return 0;
        }
    }

    if (pPlayerObj != nullptr)
    {
        LOG_ERROR("more send login msg");
        if (pPlayerObj->GetPlayerState() == PLAYER_STATE_LOAD_DATA)
        {
            pPlayerObj->OnLogin();
        }
        else
        {
            pPlayerObj->NotifyEnterGame();
            pPlayerObj->SendAllPlayerData2Client();
        }
        pPlayerObj->SetLoginKey(strDecyPHP);
        return 0;
    }
    else
    {
        if (pPlayerUid != nullptr)// 在玩
        {
            auto pOldSock = pPlayerUid->GetSession();
            uint32_t diffTime = time::getSysTime() - pPlayerUid->GetReloginTime();
            LOG_ERROR("remove old player :{} oldsock {},difftime {}", uid, pOldSock->GetRemoteAddress(), diffTime);
            if (diffTime < 1)
            {
                LOG_ERROR("登录挤号过于频繁:{}", pPlayerUid->GetUID());
                return -1;
            }
            if (pOldSock != nullptr)
            {
                pPlayerUid->NotifyLoginOut(0, msg.deviceid());
                pOldSock->SetUID(0);
                pOldSock->Close();
            }

            pPlayerUid->SetSession(m_connPtr);
            pPlayerUid->SetLoginKey(strDecyPHP);

            if (pPlayerUid->GetPlayerState() == PLAYER_STATE_PLAYING)
            {
                pPlayerUid->ReLogin();
            }
            else
            {
                pPlayerUid->OnLogin();
            }

            return 0;
        }
    }
    auto pPlayer = std::make_shared<CPlayer>(PLAYER_TYPE_ONLINE);
    pPlayer->SetUID(uid);
    pPlayer->SetSession(m_connPtr);

    CPlayerMgr::Instance().AddPlayer(pPlayer);
    pPlayer->OnLogin();
    pPlayer->SetLoginKey(strDecyPHP);

    return 0;
}

// 请求服务器信息
int CHandleClientMsg::handle_msg_req_svrs_info() {
    CHECK_PLAYER_PLAY
    LOG_DEBUG("请求服务器信息:{}", pPlayer->GetUID());

    CGameServerMgr::Instance().UpdateServerList2Client(pPlayer);
    return 0;
}

// 请求进入游戏服务器
int CHandleClientMsg::handle_msg_enter_gamesvr() {
    net::cli::msg_enter_gamesvr_req msg;
    PARSE_MSG(msg);
    uint16_t gameType = msg.game_type();
    LOG_DEBUG("请求进入游戏服务器：gameType:{}", gameType);
    CHECK_PLAYER_PLAY

    uint16_t bRet = pPlayer->EnterGameSvr(gameType);
    if (bRet != RESULT_CODE_SUCCESS)
    {
        LOG_DEBUG("无法进入游戏服务器:{}--{}", pPlayer->GetUID(), gameType);
        net::cli::msg_enter_gamesvr_rep msgrep;
        msgrep.set_result(bRet);
        msgrep.set_svrid(pPlayer->GetGameSvrID());
        pPlayer->SendMsgToClient(&msgrep, net::S2C_MSG_ENTER_SVR_REP);
        pPlayer->NotifyClientBackLobby(RESULT_CODE_SUCCESS, RESULT_CODE_ENTER_SVR_FAIL);
    }

    return 0;
}

// 上报GPS
int CHandleClientMsg::handle_msg_report_gps() {
    net::cli::msg_report_gps msg;
    PARSE_MSG(msg);
    CHECK_PLAYER_PLAY

    LOG_DEBUG("上报GPS定位uid:{} lon {}--lat {}", pPlayer->GetUID(), msg.lon(), msg.lat());
    pPlayer->FlushGPS(msg.lon(), msg.lat());
    return 0;
}

// 上报网络延迟
int CHandleClientMsg::handle_msg_report_net_delay() {
    net::cli::msg_report_net_delay msg;
    PARSE_MSG(msg);
    CHECK_PLAYER_PLAY

    if (pPlayer->SetNetDelay(msg.delay()))
    {
        //CDBMysqlMgr::Instance().UpdateNetDelay(pPlayer->GetUID(), pPlayer->GetNetDelay(), msg.net_name());
    }
    LOG_DEBUG("上报网络延迟:{}--{}--{}", pPlayer->GetUID(), msg.delay(), msg.net_name().c_str());
    return 0;
}

// 请求房间列表信息
int CHandleClientMsg::handle_msg_req_rooms_info() {
    net::cli::msg_rooms_info_req msg;
    PARSE_MSG(msg);
    CHECK_PLAYER_PLAY

    if (pPlayer->IsInLobby())
    {
        auto pServer = CGameServerMgr::Instance().SelectGameTypeServer(pPlayer, msg.game_type());
        if (pServer == nullptr)
        {
            LOG_DEBUG("不能获取游戏房间列表:uid {}--cursvrID:{}--gameType:{}", pPlayer->GetUID(), pPlayer->GetGameSvrID(), msg.game_type());
            return 0;
        }
        pServer->SendMsg(&msg, net::C2S_MSG_REQ_ROOMS_INFO, pPlayer->GetUID());
    }
    else
    {
        route_to_game_svr();
    }
    return 0;
}

std::shared_ptr<CPlayer> CHandleClientMsg::GetPlayer(const TCPConnPtr &connPtr) {
    auto pPlayer = CPlayerMgr::Instance().GetPlayer<CPlayer>(connPtr->GetUID());
    if (pPlayer == nullptr || !pPlayer->IsPlaying())
    {
        LOG_DEBUG("玩家不存在，或者玩家不在在线状态:{}", connPtr->GetUID());
        return nullptr;
    }
    return pPlayer;
}



















