//
// Created by toney on 19/10/19.
//
#pragma once

#include <map>
#include "svrlib.h"
#include "db_struct_define.h"

using namespace std;
using namespace svrlib;

class CPlayer;

class CMissionMgr {
public:
    CMissionMgr();

    ~CMissionMgr();

    void AttachPlayer(std::shared_ptr<CPlayer> pPlayer);

    std::shared_ptr<CPlayer> GetAttachPlayer();

    void SetMission(map<uint32_t, stUserMission>& mission);

    void SaveMiss();

    //获得用户的任务
    void GetMission();

    //操作用户的任务
    void ActMission(uint16_t type, uint32_t value = 1, uint32_t cate1 = 0, uint32_t cate2 = 0, uint32_t cate3 = 0, uint32_t cate4 = 0);

    //获得任务奖励
    void GetMissionPrize(uint32_t msid);

    // 添加任务
    bool AddMission(uint32_t msid);

    bool RemoveMission(uint32_t msid);

    // 是否有这个任务
    bool IsExistMission(uint32_t msid);

    stUserMission *GetMission(uint32_t msid);

    // 重置任务
    void ResetMission(uint8_t cycle);

    // 能否接受任务
    bool CanAcceptMiss(stMissionCfg *pCfg);

    // 任务是否完成
    bool IsFinished(stUserMission *pMission);

    // 奖励任务
    bool RewardMission(stUserMission *pMission);

    bool SendMissionData2Client();

    bool SendMission2Client(stUserMission &mission);

protected:
    using MAP_MISSION = map<uint32_t, stUserMission>;
    MAP_MISSION m_mission;
    std::shared_ptr<CPlayer> m_pHost = nullptr;

};
