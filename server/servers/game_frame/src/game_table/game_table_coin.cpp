//
// Created by Administrator on 2019/9/13.
//

#include "game_table_coin.h"
#include "game_room.h"
#include "mysql_mgr/dbmysql_mgr.h"
#include "net/lobby_mgr.h"
#include "data_cfg_mgr.h"
#include "player_mgr.h"

using namespace svrlib;
using namespace std;
using namespace net;

CGameCoinTable::CGameCoinTable(std::shared_ptr<CGameRoom> pRoom, int64_t tableID)
        : CGameTable(pRoom, tableID, emTABLE_TYPE_SYSTEM), m_timerClear(this) {

}

CGameCoinTable::~CGameCoinTable() {

}

bool CGameCoinTable::Init() {

    return true;
}

void CGameCoinTable::ShutDown() {

}

//��λ����
void CGameCoinTable::ResetTable() {
    m_round = 0;
}

//��Ϸ��Ϣ
int CGameCoinTable::OnMessage(std::shared_ptr<CGamePlayer> pPlayer, uint16_t cmdID, const uint8_t *pkt_buf, uint16_t buf_len) {
    //������Ϣ����
    switch (cmdID)
    {
//		case net::C2S_MSG_PRIVATE_GAMEOVER_REQ:
//		{
//			net::msg_private_gameover_req msg;
//			PARSE_MSG_FROM_ARRAY(msg);
//			LOG_TAB_DEBUG("˽�˷�ͶƱ:{}--{}--{}", pPlayer->GetUID(), msg.is_agree(), msg.req_id());
//			VoteGameOver(msg.reason(), pPlayer->GetUID(), msg.req_id(), msg.is_agree());
//			return 0;
//		}

    }


    return OnGameMessage(pPlayer, cmdID, pkt_buf, buf_len);
}

bool CGameCoinTable::EnterTable(std::shared_ptr<CGamePlayer> pPlayer) {
    if (CanEnterTable(pPlayer) != net::RESULT_CODE_SUCCESS)
        return false;
    if (NeedSitDown())
    {//��Ҫ�ֶ�����վ��
        pPlayer->SetTable(shared_from_this());
        LOG_TAB_DEBUG("enter gametable��room:{}--tb:{},uid:{}", m_pHostRoom->GetRoomID(), GetTableID(), pPlayer->GetUID());
        OnPlayerJoin(true, 0, pPlayer);
        return true;
    }
    else
    {//�Զ�����
        for (uint8_t i = 0; i < m_vecPlayers.size(); ++i)
        {
            auto &seat = m_vecPlayers[i];
            if (seat.pPlayer == nullptr)
            {
                SitDown(seat, pPlayer);
                pPlayer->SetTable(shared_from_this());
                LOG_TAB_DEBUG("enter gametable��room:{}--tb:{},uid:{}", m_pHostRoom->GetRoomID(), GetTableID(), pPlayer->GetUID());
                OnPlayerJoin(true, i, pPlayer);
                if (pPlayer->IsAutoReady() || m_pHostRoom->IsNeedMarry())
                {
                    PlayerReady(pPlayer, true);
                }
                return true;
            }
        }
    }
    LOG_TAB_ERROR("enter gametable fail:{}", pPlayer->GetUID());
    return false;
}

bool CGameCoinTable::LeaveTable(std::shared_ptr<CGamePlayer> pPlayer, bool bNotify, uint8_t leaveType) {
    if (pPlayer->GetTable() != shared_from_this())
    {
        LOG_TAB_ERROR("error table:{}--{}", pPlayer->GetTableID(), GetTableID());
        return false;
    }
    if (NeedSitDown())
    {   //��Ҫ�ֶ�����վ��
        for (uint8_t i = 0; i < m_vecPlayers.size(); ++i)
        {
            if (m_vecPlayers[i].pPlayer == pPlayer)
            {
                m_vecPlayers[i].Reset();

                OnActionUserStandUp(i, pPlayer);
                OnPlayerJoin(false, i, pPlayer);

                pPlayer->SetTable(nullptr);
                LOG_TAB_DEBUG("leave table:room:{}--tb:{},uid:{}", m_pHostRoom->GetRoomID(), GetTableID(), pPlayer->GetUID());
                if (bNotify)
                {
                    NotifyPlayerLeave(pPlayer, leaveType);
                }
                return true;
            }
        }
        LOG_TAB_DEBUG("looker leave table:{}", pPlayer->GetUID());
        OnPlayerJoin(false, 0, pPlayer);
        pPlayer->SetTable(nullptr);
        if (bNotify)
        {
            NotifyPlayerLeave(pPlayer, leaveType);
        }
        return true;
    }
    else
    {
        for (uint8_t i = 0; i < m_vecPlayers.size(); ++i)
        {
            if (m_vecPlayers[i].pPlayer == pPlayer)
            {
                m_vecPlayers[i].Reset();
                pPlayer->SetTable(nullptr);
                LOG_TAB_DEBUG("leave table:room:{}--tb:{},uid:{}", m_pHostRoom->GetRoomID(), GetTableID(), pPlayer->GetUID());

                OnPlayerJoin(false, i, pPlayer);
                if (bNotify)
                {
                    NotifyPlayerLeave(pPlayer, leaveType);
                }
                return true;
            }
        }
    }
    LOG_TAB_ERROR("leave table:{}", pPlayer->GetUID());
    return false;
}

int32_t CGameCoinTable::CanEnterTable(std::shared_ptr<CGamePlayer> pPlayer) {
    if (pPlayer->GetTable() != nullptr && pPlayer->GetTable() != shared_from_this())
    {
        LOG_TAB_ERROR("����Ѵ������ӻ��������ѹر�");
        return net::RESULT_CODE_FAIL;
    }
    if (IsFullTable() || GetGameState() != TABLE_STATE_FREE || WantNeedRecover())
    {
        LOG_TAB_ERROR("�����������ڿ���״̬");
        return net::RESULT_CODE_TABLE_FULL;
    }
    // �޶����
    if (GetPlayerCurScore(pPlayer) < GetEnterMin())
    {
        LOG_TAB_ERROR("���ֲ�������:{}--{}", GetPlayerCurScore(pPlayer), GetEnterMin());
        return net::RESULT_CODE_CION_ERROR;
    }

    return net::RESULT_CODE_SUCCESS;
}

bool CGameCoinTable::CanLeaveTable(std::shared_ptr<CGamePlayer> pPlayer) {
    //�Թ��߿����뿪(���ϰ��˳�������ע���toney)
    if (IsExistLooker(pPlayer->GetUID()) && IsExistPlayer(pPlayer->GetUID()) == false)return true;

    uint16_t chairID = GetChairID(pPlayer);
    //��ҳ�û�в��뼴���뿪
    if (GetPlayStatus(chairID) == 0)
        return true;

    //�ǹر�״̬����ֻ�ܿ���״̬�����뿪
    if (GetGameState() != TABLE_STATE_FREE)
    {
        LOG_TAB_DEBUG("�ǹر�״̬����ֻ�ܿ��в����뿪:{}", pPlayer->GetUID());
        return false;
    }

    return false;
}

// �Ƿ������Ϸ
bool CGameCoinTable::IsJoinPlay(uint16_t chairID) {
    if (chairID >= m_vecPlayers.size())return false;
    auto &seat = m_vecPlayers[chairID];
    if (seat.pPlayer == nullptr)return false;
    if (seat.playStatus == 0)
    {
        return false;
    }

    return true;
}

bool CGameCoinTable::CanSitDown(std::shared_ptr<CGamePlayer> pPlayer, uint16_t chairID) {
    if (GetPlayerCurScore(pPlayer) < m_pHostRoom->GetSitDown())
    {
        LOG_TAB_DEBUG("the sitdown condition not have :{}", m_pHostRoom->GetSitDown());
        return false;
    }
    if (!IsExistLooker(pPlayer->GetUID()))
    {
        LOG_TAB_DEBUG("not in looklist:{}", pPlayer->GetUID());
        return false;
    }
    if (chairID >= m_vecPlayers.size())
    {
        LOG_TAB_DEBUG("the seat is more big:{}", chairID);
        return false;
    }
    if (m_vecPlayers[chairID].pPlayer != nullptr)
    {
        LOG_TAB_DEBUG("the seat is other player");
        return false;
    }

    return true;
}

bool CGameCoinTable::CanStandUp(std::shared_ptr<CGamePlayer> pPlayer) {
    if (CanLeaveTable(pPlayer))
        return true;

    return false;
}

bool CGameCoinTable::NeedSitDown() {
    return false;
}

bool CGameCoinTable::PlayerReady(std::shared_ptr<CGamePlayer> pPlayer, bool bReady) {
    if (GetPlayerCurScore(pPlayer) < GetEnterMin())
    {
        LOG_TAB_DEBUG("can't ready {}", pPlayer->GetUID());
        return false;
    }
    for (uint8_t i = 0; i < m_vecPlayers.size(); ++i)
    {
        auto &seat = m_vecPlayers[i];
        if (seat.pPlayer == pPlayer)
        {
            seat.readyStatus = bReady ? 1 : 0;
            seat.autoStatus = 0;
            seat.readyTime = time::getSysTime();
            seat.overTimes = 0;
            SendReadyStateToClient();
            OnActionUserOnReady(i, pPlayer);
            return true;
        }
    }
    LOG_TAB_ERROR("player ready fail :{}--{}", pPlayer->GetUID(), GetTableID());
    return false;
}

//�û����߻�����
bool CGameCoinTable::OnActionUserNetState(std::shared_ptr<CGamePlayer> pPlayer, bool bConnected, bool isJoin) {
    if (bConnected)//��������
    {
        if (isJoin)
        {
            pPlayer->SetPlayDisconnect(false);
            SendTableInfoToClient(pPlayer);
            SendSeatInfoToClient();

            if (IsExistLooker(pPlayer->GetUID()))
            {
                NotifyPlayerJoin(pPlayer, true);
            }
            SendLookerListToClient(pPlayer);
            SendGameScene(pPlayer);
            SendReadyStateToClient();
        }
    }
    else
    {
        pPlayer->SetPlayDisconnect(true);
        SendSeatInfoToClient();
        if (IsExistLooker(pPlayer->GetUID()))
        {
            pPlayer->TryLeaveCurTable();
        }
        SendReadyStateToClient();
    }

    LOG_TAB_DEBUG("OnAction Net State:{}--{}--{}", pPlayer->GetUID(), bConnected, isJoin);

    return true;
}

//�û�����
bool CGameCoinTable::OnActionUserSitDown(uint16_t wChairID, std::shared_ptr<CGamePlayer> pPlayer) {
    SendSeatInfoToClient();
    return true;
}

//�û�����
bool CGameCoinTable::OnActionUserStandUp(uint16_t wChairID, std::shared_ptr<CGamePlayer> pPlayer) {
    SendSeatInfoToClient();
    return true;
}

//�û�ͬ��
bool CGameCoinTable::OnActionUserOnReady(uint16_t wChairID, std::shared_ptr<CGamePlayer> pPlayer) {
    return true;
}

//��ҽ�����뿪
void CGameCoinTable::OnPlayerJoin(bool isJoin, uint16_t chairID, std::shared_ptr<CGamePlayer> pPlayer) {
    if (isJoin)
    {
        SendTableInfoToClient(pPlayer);
    }
    if (NeedSitDown())
    {
        if (isJoin)
        {
            AddLooker(pPlayer);
            SendLookerListToClient(pPlayer);
        }
        else
        {
            RemoveLooker(pPlayer);
        }
    }
    SendSeatInfoToClient();
    SendReadyStateToClient();
}

// �Ƿ���Ҫ����
bool CGameCoinTable::NeedRecover() {
    if (m_pHostRoom->IsNeedMarry() && IsEmptyTable())
        return true;//ƥ��ģʽ���տ�����

    return false;
}

bool CGameCoinTable::WantNeedRecover() {
    if (m_pHostRoom->IsNeedMarry() && m_round > 0)
    {
        return true;//ƥ��ģʽ�������ܽ���
    }
    return false;
}

//û���ֵ�����Զ�վ��
void CGameCoinTable::StandUpNotScore() {
    for (auto &seat : m_vecPlayers)
    {
        auto pPlayer = seat.pPlayer;
        if (pPlayer != nullptr && GetPlayerCurScore(pPlayer) < GetEnterMin())
        {
            LeaveTable(pPlayer, true, 1);
        }
    }
}

void CGameCoinTable::LeaveAllPlayer() {
    for (auto &seat : m_vecPlayers)
    {
        auto pPlayer = seat.pPlayer;
        if (pPlayer != nullptr)
        {
            LeaveTable(pPlayer, true, 1);
        }
    }
}

// �۳���ʼ̨��
void CGameCoinTable::DeductStartFee(bool bNeedReady) {
    LOG_TAB_DEBUG("Deduct Start Fee");
    if (m_conf.feeType == TABLE_FEE_TYPE_ALLBASE)
    {
        int64_t fee = -(m_conf.baseScore * m_conf.feeValue / PRO_DENO_10000);
        for (uint32_t i = 0; i < m_vecPlayers.size(); ++i)
        {
            auto pPlayer = GetPlayer(i);
            if (pPlayer == nullptr || (bNeedReady && m_vecPlayers[i].readyStatus == 0))
                continue;

            ChangeScoreValueByUID(pPlayer->GetUID(), fee, emACCTRAN_OPER_TYPE_FEE, GetTableID());
            ChangeUserBlingLogFee(pPlayer->GetUID(), fee);
        }
        FlushSeatValueInfoToClient();
    }
}

// �۳�����̨��
void CGameCoinTable::DeducEndFee(uint32_t uid, int64_t &winScore) {
    LOG_TAB_DEBUG("Deduc End Fee");
    if (m_conf.feeType == TABLE_FEE_TYPE_WIN)
    {
        if (winScore > 0)
        {
            int64_t fee = -(winScore * m_conf.feeValue / PRO_DENO_10000);
            winScore += fee;
            ChangeUserBlingLogFee(uid, fee);
        }
        else
        {
            int64_t fee = -(winScore * m_conf.feeValue / PRO_DENO_10000);
        }
    }
}

// �ϱ���Ϸս��
void CGameCoinTable::ReportGameResult(shared_ptr<CGamePlayer> pPlayer, int64_t winScore) {
    net::svr::msg_report_game_result msg;
    msg.set_uid(pPlayer->GetUID());
    msg.set_game_type(m_pHostRoom->GetGameType());
    msg.set_play_type(m_pHostRoom->GetPlayType());
    msg.set_win_score(winScore);
    pPlayer->SendMsgToClient(&msg, net::svr::GS2L_MSG_REPORT_GAME_RESULT);
}

// ���������Ϣ
void CGameCoinTable::CalcPlayerGameInfo(uint32_t uid, int64_t winScore) {
    LOG_TAB_DEBUG("calc player game info:{}  {}", uid, winScore);
    auto pPlayer = CPlayerMgr::Instance().GetPlayer<CGamePlayer>(uid);
    if (pPlayer != nullptr)
    {
        ReportGameResult(pPlayer, winScore);
        //��ˮ
        DeducEndFee(uid, winScore);
        //�޸Ļ���
        ChangeScoreValueByUID(uid, winScore, emACCTRAN_OPER_TYPE_GAME, m_pHostRoom->GetGameType());
        //ˢ����־�Ʒ�
        ChangeUserBlingLog(pPlayer, winScore);
    }
}

int64_t CGameCoinTable::ChangeScoreValueByUID(uint32_t uid, int64_t &score, uint16_t operType, uint16_t subType) {

    auto pGamePlayer = CPlayerMgr::Instance().GetPlayer<CGamePlayer>(uid);
    if (pGamePlayer != nullptr)
    {
        pGamePlayer->SyncChangeAccountValue(operType, subType, score, 0, m_chessid);
    }
    else
    {
        CLobbyMgr::Instance().NotifyLobbyChangeAccValue(uid, operType, subType, score, 0, m_chessid);
    }

    return score;
}

void CGameCoinTable::OnGameRoundStart() {
    InitBlingLog(true);
    DeductStartFee(true);
    LOG_TAB_DEBUG("��Ϸһ�ֿ�ʼ {}", GetTableID());
}

// һ�ֽ���
void CGameCoinTable::OnGameRoundOver() {
    m_round++;
    ResetPlayerReady();
    SendSeatInfoToClient();
    SaveBlingLog();
    LOG_TAB_DEBUG("��Ϸһ�ֽ��� {}", GetTableID());
}

void CGameCoinTable::OnGameRoundFlush() {
    FlushSeatValueInfoToClient();
    StandUpNotScore();
    LOG_TAB_DEBUG("��Ϸ����ˢ������ {}", GetTableID());
    if (m_pHostRoom->IsNeedMarry())
    {
        SetGameState(TABLE_STATE_RECYCLE);
        CApplication::Instance().schedule(&m_timerClear, 5000);
    }
}

// ��ʱ�������
void CGameCoinTable::OnTimerClearPlayer() {
    LOG_TAB_DEBUG("OnTimer Clear player");
    LeaveAllPlayer();
}























































