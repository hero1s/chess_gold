//
// Created by yinguohua on 2019/9/27.
//

#include "robot.h"
#include "game_table/game_room.h"
#include "data_cfg_mgr.h"

using namespace svrlib;
using namespace std;

CGameRobot::CGameRobot()
        : CGamePlayer(PLAYER_TYPE_ROBOT) {
    m_loginInCoin = 0;
}

CGameRobot::~CGameRobot() {

}

void CGameRobot::OnTimeTick(uint64_t uTime, bool bNewDay) {
    //ά��״̬
    if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
    {
        TryLeaveCurTable();
    }
}

void CGameRobot::OnLogin() {
    CGamePlayer::OnLogin();
    m_loginInCoin = GetAccountValue(emACC_VALUE_COIN);
    LOG_ERROR("�����˽�����Ϸ������:uid:{},coin:{}", GetUID(), GetAccountValue(emACC_VALUE_COIN));
}

void CGameRobot::OnGameEnd() {

}

// �Ƿ���Ҫ����
bool CGameRobot::NeedRecover() {
    //ά��״̬
    if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
    {
        if (!IsInGamePlaying())return true;
    }
    return false;
}

// �Ƿ��Ʋ�
bool CGameRobot::IsBankrupt() {
    if (GetAccountValue(emACC_VALUE_COIN) < (m_loginInCoin / 4))
    {
        LOG_DEBUG("����Ʋ�uid:{} :{}-->{}", GetUID(), m_loginInCoin, GetAccountValue(emACC_VALUE_COIN));
        return true;
    }

    return false;
}


