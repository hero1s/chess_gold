
#include "game_player.h"
#include "helper/bufferStream.h"
#include "time/time.hpp"
#include "net/lobby_mgr.h"
#include "game_table/game_room.h"
#include "game_table/game_table.h"
#include "error_code.pb.h"

using namespace svrlib;
using namespace std;
using namespace Network;

namespace {

}

CGamePlayer::CGamePlayer(PLAYER_TYPE type)
        : CPlayerBase(type) {
    m_msgHeartTime = time::getSysTime();
    m_playDisconnect = false;
    m_pGameTable = nullptr;
    m_pGameRoom = nullptr;
    m_autoReady = false;

}

CGamePlayer::~CGamePlayer() {

}

bool CGamePlayer::SendMsgToClient(const google::protobuf::Message *msg, uint16_t msg_type) {
    if (IsPlayDisconnect() && msg_type > 3000)
    {
        LOG_DEBUG("reconnect not enter table and can't send game message {}--{}", GetUID(), msg_type);
        return false;
    }

    CLobbyMgr::Instance().SendMsg2Client(msg, msg_type, GetUID(), GetLoginLobbySvrID());
    return true;
}

void CGamePlayer::OnLoginOut() {
    LOG_DEBUG("GamePlayer OnLoginOut:{}", GetUID());
    SetPlayerState(PLAYER_STATE_LOGINOUT);
    if (m_pGameTable != nullptr)
    {
        m_pGameTable->LeaveTable(shared_from_this());
    }
    if (m_pGameRoom != nullptr)
    {
        m_pGameRoom->LeaveRoom(shared_from_this());
    }
    SetGameSvrID(0, true);
    NotifyLeaveGameSvr();
}

void CGamePlayer::OnLogin() {
    LOG_DEBUG("GamePlayer OnLogin:{}", GetUID());
    SetNetState(1);
    SetGameSvrID(CApplication::Instance().GetServerID(), true);
}

void CGamePlayer::ReLogin() {
    LOG_DEBUG("GamePlayer ReLogin:{}", GetUID());
    SetNetState(1);
    SetGameSvrID(CApplication::Instance().GetServerID(), true);
}

// ֪ͨ���ش���(�˳���Ϸ)
void CGamePlayer::NotifyLeaveGameSvr() {
    LOG_DEBUG("notify leave game:{}", GetUID());
    net::svr::msg_leave_svr msg;
    msg.set_uid(GetUID());

    SendMsgToClient(&msg, net::svr::GS2L_MSG_LEAVE_SVR);
}

// �޸�����˺���ֵ�������޸ģ�
void CGamePlayer::SyncChangeAccountValue(uint16_t operType, uint16_t subType, int64_t coin, int64_t safecoin,
                                         const string &chessid) {
    coin = ChangeAccountValue(emACC_VALUE_COIN, coin);
    safecoin = ChangeAccountValue(emACC_VALUE_SAFECOIN, safecoin);
    CLobbyMgr::Instance().NotifyLobbyChangeAccValue(GetUID(), operType, subType, coin, safecoin, chessid);
}

// �ܷ��˳�
bool CGamePlayer::CanBackLobby() {
    if (m_pGameTable != nullptr && !m_pGameTable->CanLeaveTable(shared_from_this()))
    {
        return false;
    }

    return true;
}

// ��������ʱ��
void CGamePlayer::ResetHeart() {
    m_msgHeartTime = time::getSysTime();
}

// ����ʱ��
uint32_t CGamePlayer::GetDisconnectTime() {
    auto difftime = time::getSysTime() - m_msgHeartTime;
    if (difftime > TimeConstants::MINUTE * 5)return difftime;//5����û����Ϣ���������ߴ���

    if (m_netState != 0)return 0;

    return difftime;
}

void CGamePlayer::OnTimeTick(uint64_t uTime, bool bNewDay) {
    //ά��״̬
    if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
    {
        TryLeaveCurTable();
    }
    //����״̬
    if (GetNetState() == 0 || GetDisconnectTime() > TimeConstants::MINUTE)
    {
        if (m_pGameTable != nullptr)
        {
            uint8_t tableType = m_pGameTable->GetTableType();
            switch (tableType)
            {
                case emTABLE_TYPE_SYSTEM:// ϵͳ����
                {
                    TryLeaveCurTable();
                    break;
                }
                case emTABLE_TYPE_PLAYER:// ��������
                {
                    if (m_pGameTable->WantNeedRecover())
                    {
                        TryLeaveCurTable();
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

}

// �Ƿ���Ҫ����
bool CGamePlayer::NeedRecover() {
    //ά��״̬�����˳�
    if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
    {
        if (CanBackLobby())
        {
            LOG_ERROR("ά��״̬�����ش���");
            return true;
        }
    }
    if (GetDisconnectTime() > TimeConstants::MINUTE)
    {//����1�����߳�
        if (m_pGameTable != nullptr)
        {
            return false;
        }
        return true;
    }
    return false;
}

// �Ƿ�������Ϸ��
bool CGamePlayer::IsInGamePlaying() {
    if (m_pGameTable == nullptr)
        return false;
    if (m_pGameTable->GetGameState() != TABLE_STATE_FREE || m_pGameTable->IsReady(shared_from_this()))
        return true;

    return false;
}

// ��Ϣ����
int CGamePlayer::OnMessage(uint16_t cmdID, const uint8_t *pkt_buf, uint16_t buf_len) {
    if (m_pGameTable)
    {
        m_pGameTable->OnMessage(shared_from_this(), cmdID, pkt_buf, buf_len);
    }
    ResetHeart();
    return 0;
}

uint8_t CGamePlayer::GetNetState() {
    return m_netState;
}

void CGamePlayer::SetNetState(uint8_t state) {
    m_netState = state;
    if (m_pGameTable)
    {
        m_pGameTable->OnActionUserNetState(shared_from_this(), m_netState, false);
        if (state == 0)
        {
            if (m_pGameTable->CanLeaveTable(shared_from_this()))
            {
                if (m_pGameTable->LeaveTable(shared_from_this()))
                {
                    m_pGameRoom->LeaveRoom(shared_from_this());
                }
            }
        }
    }
    else
    {
        SetPlayDisconnect(false);
    }

}

void CGamePlayer::SetPlayDisconnect(bool bFlag) {
    m_playDisconnect = bFlag;
}

bool CGamePlayer::IsPlayDisconnect() {
    return m_playDisconnect;
}

std::shared_ptr<CGameRoom> CGamePlayer::GetRoom() {
    return m_pGameRoom;
}

void CGamePlayer::SetRoom(std::shared_ptr<CGameRoom> pRoom) {
    m_pGameRoom = pRoom;
}

uint16_t CGamePlayer::GetRoomID() {
    if (m_pGameRoom)return m_pGameRoom->GetRoomID();
    return 0;
}

std::shared_ptr<CGameTable> CGamePlayer::GetTable() {
    return m_pGameTable;
}

void CGamePlayer::SetTable(std::shared_ptr<CGameTable> pTable) {
    m_pGameTable = pTable;
}

int64_t CGamePlayer::GetTableID() {
    if (m_pGameTable)return m_pGameTable->GetTableID();
    return 0;
}

void CGamePlayer::SetAutoReady(bool bAuto) {
    m_autoReady = bAuto;
}

bool CGamePlayer::IsAutoReady() {
    return m_autoReady;
}

// �뿪��ǰ����
std::shared_ptr<CGameTable> CGamePlayer::TryLeaveCurTable() {
    if (m_pGameTable == nullptr)
        return nullptr;
    if (m_pGameTable->CanLeaveTable(shared_from_this()))
    {
        m_pGameTable->LeaveTable(shared_from_this(), true, 1);
    }
    return m_pGameTable;
}

















