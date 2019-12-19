
#include "mission_mgr.h"
#include "db_struct_define.h"
#include "player.h"
#include "mysql_mgr/dbmysql_mgr.h"
#include "data_cfg_mgr.h"
#include "msg_define.pb.h"
#include "client_logic_msg.pb.h"

using namespace std;
using namespace svrlib;

CMissionMgr::CMissionMgr() {
    m_pHost = nullptr;
}

CMissionMgr::~CMissionMgr() {
    m_pHost = nullptr;
}

void CMissionMgr::AttachPlayer(std::shared_ptr<CPlayer> pPlayer) {
    m_pHost = pPlayer;
}

std::shared_ptr<CPlayer> CMissionMgr::GetAttachPlayer() {
    return m_pHost;
}

void CMissionMgr::SetMission(map<uint32_t, stUserMission> &mission) {
    m_mission = mission;
    m_pHost->SetLoadState(emACCDATA_TYPE_MISS);
    GetMission();
}

void CMissionMgr::SaveMiss() {
    CDBMysqlMgr::Instance().SaveUserMission(m_pHost->GetUID(), m_mission);
    vector<uint32_t> dels;
    for (auto &it : m_mission)
    {
        stUserMission &refMiss = it.second;
        switch (refMiss.update)
        {
            case emDB_ACTION_UPDATE:
            case emDB_ACTION_INSERT:
            {
                refMiss.update = emDB_ACTION_NONE;
                break;
            }
            case emDB_ACTION_DELETE:
            {
                dels.push_back(refMiss.msid);
                break;
            }
            default:
                break;
        }
    }
    for (uint32_t i = 0; i < dels.size(); ++i)
    {
        RemoveMission(dels[i]);
    }
}

//获得用户的任务
void CMissionMgr::GetMission() {
    auto missioncfg = CDataCfgMgr::Instance().GetAllMissionCfg();
    for (auto &it : missioncfg)
    {
        if (it.second.status == 1)
        {
            continue;
        }
        if (IsExistMission(it.first))
            continue;
        AddMission(it.first);
    }
    // 删除不存在的任务
    for (auto &it : m_mission)
    {
        stUserMission &refMiss = it.second;
        const stMissionCfg *pCfg = CDataCfgMgr::Instance().GetMissionCfg(refMiss.msid);
        if (pCfg == NULL || pCfg->status == 1)
        {
            refMiss.update = emDB_ACTION_DELETE;
        }
    }
    SaveMiss();
}

//操作用户任务
void CMissionMgr::ActMission(uint16_t type, uint32_t value, uint32_t cate1, uint32_t cate2, uint32_t cate3, uint32_t cate4) {
    for (auto &it : m_mission)
    {
        //已完成的任务不再累加
        stUserMission &refUserMiss = it.second;
        const stMissionCfg *pMissCfg = CDataCfgMgr::Instance().GetMissionCfg(refUserMiss.msid);
        if (pMissCfg == NULL)
            continue;

        if (pMissCfg->mtimes <= refUserMiss.rtimes)
        {
            continue;
        }
        if (pMissCfg->cycletimes <= refUserMiss.ctimes)
        {
            continue;
        }

        //判断动作类型
        if (pMissCfg->type != type)
        {
            continue;
        }
        if (pMissCfg->cate1 > 0 && pMissCfg->cate1 != cate1)
        {
            continue;
        }
        if (pMissCfg->cate2 > 0 && pMissCfg->cate2 != cate2)
        {
            continue;
        }
        if (pMissCfg->cate3 > 0 && pMissCfg->cate3 != cate3)
        {
            continue;
        }
        if (pMissCfg->cate4 > 0 && pMissCfg->cate4 != cate4)
        {
            continue;
        }

        if (pMissCfg->straight == 1 && value == 0)
        {// 连续任务
            refUserMiss.rtimes = 0;
        }
        else
        {
            if (value == 0)
                continue;
            refUserMiss.rtimes += value;
        }
        refUserMiss.ptime = time::getSysTime();// 操作时间
        if (refUserMiss.update == emDB_ACTION_NONE)
        {
            refUserMiss.update = emDB_ACTION_UPDATE;
        }
        //判断是否自动领取
        if (pMissCfg->autoprize == 1 && IsFinished(&refUserMiss))
        {
            RewardMission(&refUserMiss);
        }
        else
        {
            SendMission2Client(refUserMiss);
        }
    }
}

//获得任务奖励
void CMissionMgr::GetMissionPrize(uint32_t msid) {
    stUserMission *pMission = GetMission(msid);
    if (pMission == NULL)
    {
        LOG_ERROR("CPlayer::getMissionPrize the uid = {},the msid = {},has no mission", m_pHost->GetUID(), msid);
        return;
    }
    //判断不能领
    if (IsFinished(pMission) == false)
        return;
    RewardMission(pMission);
}

// 添加任务
bool CMissionMgr::AddMission(uint32_t msid) {
    const stMissionCfg *pCfg = CDataCfgMgr::Instance().GetMissionCfg(msid);
    if (pCfg == NULL || pCfg->status == 1)
        return false;
    if (IsExistMission(msid))
        return false;

    stUserMission mission;
    mission.msid = msid;
    mission.rtimes = 0;
    mission.ptime = 0;
    mission.ctimes = 0;
    mission.cptime = 0;
    mission.update = emDB_ACTION_INSERT;
    m_mission.insert(make_pair(msid, mission));

    return true;
}

bool CMissionMgr::RemoveMission(uint32_t msid) {
    m_mission.erase(msid);
    return true;
}

// 是否有这个任务
bool CMissionMgr::IsExistMission(uint32_t msid) {
    auto it = m_mission.find(msid);
    if (it == m_mission.end())
    {
        return false;
    }
    return true;
}

stUserMission *CMissionMgr::GetMission(uint32_t msid) {
    auto it = m_mission.find(msid);
    if (it == m_mission.end())
    {
        return NULL;
    }
    return &it->second;
}

// 重置任务
void CMissionMgr::ResetMission(uint8_t cycle) {
    //先判断用户的任务
    for (auto &it : m_mission)
    {
        stUserMission &task = it.second;
        const stMissionCfg *pCfg = CDataCfgMgr::Instance().GetMissionCfg(task.msid);
        if (pCfg == NULL)
        {
            continue;
        }
        //循环任务
        if (task.ptime > 0 && pCfg->cycle == cycle)
        {
            task.rtimes = 0;
            task.ptime = time::getSysTime();
            task.ctimes = 0;
            task.update = emDB_ACTION_UPDATE;
            task.cptime = 0;
        }
    }
}

// 能否接受任务
bool CMissionMgr::CanAcceptMiss(stMissionCfg *pCfg) {
    if (IsExistMission(pCfg->msid))
    {
        return false;
    }
    // 其它条件判断


    return true;
}

// 任务是否完成
bool CMissionMgr::IsFinished(stUserMission *pMission) {
    const stMissionCfg *pMissCfg = CDataCfgMgr::Instance().GetMissionCfg(pMission->msid);
    if (pMissCfg == NULL)
        return false;

    //判断不能领
    if (pMission->rtimes < pMissCfg->mtimes)
    {
        LOG_ERROR("CPlayer::getMissionPrize the uid = {},the msid = {},the rtimes = {},the mtimes = {}",
                  m_pHost->GetUID(), pMission->msid, pMission->rtimes, pMissCfg->mtimes);
        return false;
    }
    if (pMission->ctimes >= pMissCfg->cycletimes)
    {
        LOG_ERROR("CPlayer::getMissionPrize the uid = {},the msid = {},the ctimes = {},the cycletimes = {}",
                  m_pHost->GetUID(), pMission->msid, pMission->ctimes, pMissCfg->cycletimes);
        return false;
    }

    return true;
}

// 奖励任务
bool CMissionMgr::RewardMission(stUserMission *pMission) {
    const stMissionCfg *pMissCfg = CDataCfgMgr::Instance().GetMissionCfg(pMission->msid);
    if (pMissCfg == NULL)
        return false;
    pMission->rtimes = 0;
    pMission->ctimes++;
    pMission->cptime = time::getSysTime();// 完成时间
    if (pMission->update == emDB_ACTION_NONE)
    {
        pMission->update = emDB_ACTION_UPDATE;
    }
    int64_t roomCard, coin;
    roomCard = coin = 0;
    for (uint32_t i = 0; i < pMissCfg->missionprize.size(); ++i)
    {
        uint32_t qty = pMissCfg->missionprize[i].qty;
        switch (pMissCfg->missionprize[i].poid)
        {
            case emACC_VALUE_COIN:
            {
                coin = qty;
                break;
            }
            default:
                break;
        }
    }
    m_pHost->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_TASK, pMission->msid, coin, 0);
    SendMission2Client(*pMission);
    m_pHost->UpdateAccValue2Client();
    return true;
}

bool CMissionMgr::SendMissionData2Client() {
    net::cli::msg_send_all_mission_rep msg;
    for (auto &it : m_mission)
    {
        net::mission_data *pInfo = msg.add_missions();
        pInfo->set_msid(it.second.msid);
        pInfo->set_ctimes(it.second.ctimes);
        pInfo->set_rtimes(it.second.rtimes);
        pInfo->set_cptime(it.second.cptime);
    }
    m_pHost->SendMsgToClient(&msg, net::S2C_MSG_SEND_ALL_MISSION_REP);
    return true;
}

bool CMissionMgr::SendMission2Client(stUserMission &mission) {
    net::cli::msg_send_mission_rep msg;
    net::mission_data *pInfo = msg.mutable_mission();
    pInfo->set_msid(mission.msid);
    pInfo->set_ctimes(mission.ctimes);
    pInfo->set_rtimes(mission.rtimes);
    pInfo->set_cptime(mission.cptime);
    m_pHost->SendMsgToClient(&msg, net::S2C_MSG_SEND_MISSION_REP);

    return true;
}









