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
    // �Ƿ���Ҫ����
    virtual bool 	NeedRecover();
public:
    // �Ƿ��Ʋ�
    bool    IsBankrupt();


protected:
    int64_t   m_loginInCoin;

};

