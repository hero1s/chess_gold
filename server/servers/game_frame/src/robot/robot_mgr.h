//
// Created by yinguohua on 2019/9/27.
//

#pragma once

#include "svrlib.h"
#include "robot.h"

class CGamePlayer;

class CGameTable;

class CRobotMgr : public AutoDeleteSingleton<CRobotMgr> {
public:
    CRobotMgr();

    ~CRobotMgr();

    bool Init();

    void ShutDown();

    void OnTimeTick();

    shared_ptr<CGameRobot> GetRobot(uint32_t uid);

    // �������һ��������
    bool RequestOneRobot(shared_ptr<CGameTable> pTable);

    bool AddRobot(shared_ptr<CGameRobot> pRobot);

    bool RemoveRobot(shared_ptr<CGameRobot> pRobot);

    // ���л���������
    uint32_t GetFreeRobotNum();

protected:


protected:
    unordered_map<uint32_t, shared_ptr<CGameRobot>> m_mpRobots;
    bool m_isOpenRobot;

};

