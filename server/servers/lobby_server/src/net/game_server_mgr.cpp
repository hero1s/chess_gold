

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

// ���·������б�����
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
    LOG_DEBUG("���ͷ������б�:{}--{},cursvr:{}", pPlayer->GetUID(), info.svrs_size(), info.cur_svrid());
}

// �����ѡ����Ӧ��Ϸ������
shared_ptr<CServerClient> CGameServerMgr::SelectGameTypeServer(std::shared_ptr<CPlayer> pPlayer, uint16_t gameType) {
    for (auto &it : m_mpServers)
    {
        auto pSvr = it.second;
        if (pSvr->GetGameType() == gameType)
        {
            if (!pSvr->IsRepair())
            {//����ҽ�������������
                if (pPlayer->GetGameSvrID() == 0 || pPlayer->GetGameSvrID() == pSvr->GetSvrID()) return pSvr;
            }
            else
            {//ά���з�����ֻ���ܶ�������
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

// ת�����ͻ���
int CGameServerMgr::route_to_client() {
    LOG_DEBUG("ת�����ͻ�����Ϣ:uid:{}--cmd:{}", m_head->uin, m_head->msgID);
    auto pPlayer = GetPlayer();
    if (pPlayer != nullptr)
    {
        pPlayer->SendMsgToClient(m_pkt_buf, m_buf_len, m_head->msgID);
    }
    else
    {
        LOG_DEBUG("ת����Ϣ�ͻ��˲����ڣ�֪ͨ��Ϸ������:{}", m_head->uin);
        net::svr::msg_notify_net_state msg;
        msg.set_uid(m_head->uin);
        msg.set_state(0);
        msg.set_newip(0);
        msg.set_no_player(1);
        pkg_inner::SendProtobufMsg(m_connPtr, &msg, net::svr::L2GS_MSG_NOTIFY_NET_STATE, m_head->uin, 0, 0);
    }
    return 0;
}

// �������ϱ���Ϣ
int CGameServerMgr::handle_msg_report() {
    net::svr::msg_report_svr_info msg;
    PARSE_MSG(msg);

    uint32_t players = msg.onlines();

    //LOG_DEBUG("��Ϸ���ϱ���Ϣ:sid {}--{}",m_connPtr->GetUID(),players);

    return 0;
}

// ���ش���
int CGameServerMgr::handle_msg_leave_svr() {
    net::svr::msg_leave_svr msg;
    PARSE_MSG(msg);

    uint32_t uid = msg.uid();
    LOG_DEBUG("֪ͨ���ش���:{}", uid);
    auto pPlayer = GetPlayer();
    if (pPlayer != nullptr)
    {
        pPlayer->BackLobby();
        pPlayer->NotifyClientBackLobby(RESULT_CODE_SUCCESS, RESULT_CODE_SUCCESS);
    }
    else
    {
        LOG_DEBUG("���ش�����Ҳ�����:{}", uid);
    }
    return 0;
}

// �޸������ֵ
int CGameServerMgr::handle_msg_notify_change_account_data() {
    net::svr::msg_notify_change_account_data msg;
    PARSE_MSG(msg);
    uint32_t uid = msg.uid();
    int64_t coin = msg.coin();
    int64_t safeCoin = msg.safe_coin();
    int32_t operType = msg.oper_type();
    int32_t subType = msg.sub_type();
    string chessid = msg.chessid();
    LOG_DEBUG("change value {} ��ֵ:coin:{},safecoin:{}", uid, coin, safeCoin);
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

// �ϱ���Ϸ���
int CGameServerMgr::handle_msg_report_game_result() {
    net::svr::msg_report_game_result msg;
    PARSE_MSG(msg);
    uint32_t gameType = msg.game_type();
    uint32_t playType = msg.play_type();
    uint32_t uid = msg.uid();

    int32_t win = msg.win_score() > 0 ? 1 : 0;
    int32_t lose = (win == 1) ? 0 : 1;
    uint32_t cate1 = gameType;        //��Ϸ����
    uint32_t cate2 = playType;        //�淨����
    uint32_t cate3 = 0;               //�������
    uint32_t cate4 = 0;               //���伶��
    LOG_DEBUG("�ϱ���Ϸ���:{},gameType {},playType {}", uid,gameType,playType);

    auto pPlayer = GetPlayer();
    if (pPlayer == nullptr)
    {
        return 0;
    }
    if (!pPlayer->CanModifyData())
        return 0;

    // ��������
    pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_PLAY, 1, cate1, cate2, cate3, cate4);
    pPlayer->GetMissionMgr().ActMission(net::MISSION_TYPE_WIN, win, cate1, cate2, cate3, cate4);

    pPlayer->GetMissionMgr().SaveMiss();
    return 0;
}


std::shared_ptr<CPlayer> CGameServerMgr::GetPlayer() {
    auto pPlayer = CPlayerMgr::Instance().GetPlayer<CPlayer>(m_head->uin);
    return pPlayer;
}




















