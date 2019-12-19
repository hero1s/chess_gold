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
    //维护状态
    if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
    {
        TryLeaveCurTable();
    }
}

void CGameRobot::OnLogin() {
    CGamePlayer::OnLogin();
    m_loginInCoin = GetAccountValue(emACC_VALUE_COIN);
    LOG_ERROR("机器人进入游戏服务器:uid:{},coin:{}", GetUID(), GetAccountValue(emACC_VALUE_COIN));
}

void CGameRobot::OnGameEnd() {

}

// 是否需要回收
bool CGameRobot::NeedRecover() {
    //维护状态
    if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
    {
        if (!IsInGamePlaying())return true;
    }
    return false;
}

// 是否破产
bool CGameRobot::IsBankrupt() {
    if (GetAccountValue(emACC_VALUE_COIN) < (m_loginInCoin / 4))
    {
        LOG_DEBUG("金币破产uid:{} :{}-->{}", GetUID(), m_loginInCoin, GetAccountValue(emACC_VALUE_COIN));
        return true;
    }

    return false;
}


