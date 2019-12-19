//
// Created by yinguohua on 2019/9/27.
//
#pragma once
#include "game_player.h"

class CGameRobot : public CGamePlayer
{
public:
    CGameRobot();
    virtual ~CGameRobot();
    virtual void  	OnLogin();
    virtual void 	OnGameEnd();

    virtual void 	OnTimeTick(uint64_t uTime,bool bNewDay);
    // 是否需要回收
    virtual bool 	NeedRecover();
public:
    // 是否破产
    bool    IsBankrupt();


protected:
    int64_t   m_loginInCoin;

};

