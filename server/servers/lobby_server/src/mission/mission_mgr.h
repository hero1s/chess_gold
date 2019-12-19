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

    //����û�������
    void GetMission();

    //�����û�������
    void ActMission(uint16_t type, uint32_t value = 1, uint32_t cate1 = 0, uint32_t cate2 = 0, uint32_t cate3 = 0, uint32_t cate4 = 0);

    //���������
    void GetMissionPrize(uint32_t msid);

    // �������
    bool AddMission(uint32_t msid);

    bool RemoveMission(uint32_t msid);

    // �Ƿ����������
    bool IsExistMission(uint32_t msid);

    stUserMission *GetMission(uint32_t msid);

    // ��������
    void ResetMission(uint8_t cycle);

    // �ܷ��������
    bool CanAcceptMiss(stMissionCfg *pCfg);

    // �����Ƿ����
    bool IsFinished(stUserMission *pMission);

    // ��������
    bool RewardMission(stUserMission *pMission);

    bool SendMissionData2Client();

    bool SendMission2Client(stUserMission &mission);

protected:
    using MAP_MISSION = map<uint32_t, stUserMission>;
    MAP_MISSION m_mission;
    std::shared_ptr<CPlayer> m_pHost = nullptr;

};
