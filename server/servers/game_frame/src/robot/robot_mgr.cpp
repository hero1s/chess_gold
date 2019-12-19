//
// Created by yinguohua on 2019/9/27.
//

#include "robot_mgr.h"
#include "game_table/game_room.h"
#include "common_logic.h"
#include "player_mgr.h"

namespace
{
    static const uint32_t s_FreeRobotNum = 10;
    static const int32_t  s_RoomMaxRobot = 100;
};

CRobotMgr::CRobotMgr()
{
    m_isOpenRobot = false;
}
CRobotMgr::~CRobotMgr()
{
}
bool CRobotMgr::Init()
{
    m_isOpenRobot = false;
    return true;
}
void CRobotMgr::ShutDown()
{

}
void CRobotMgr::OnTimeTick()
{

}
shared_ptr<CGameRobot> CRobotMgr::GetRobot(uint32_t uid)
{
    auto it = m_mpRobots.find(uid);
    if(it != m_mpRobots.end())
        return it->second;
    return nullptr;
}
// 请求分配一个机器人
bool CRobotMgr::RequestOneRobot(shared_ptr<CGameTable> pTable)
{
    if(m_mpRobots.empty() || !m_isOpenRobot)
        return false;
    if(CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
        return false;

    auto it = m_mpRobots.begin();
    uint32_t startPos = svrlib::rand()%m_mpRobots.size();
    for(;it != m_mpRobots.end();++it)
    {
        if(startPos > 0){
            startPos--;
            continue;
        }
        auto pRobot = it->second;
        if(pRobot->GetRoom() != nullptr || pRobot->GetTable() != nullptr){
            continue;
        }
        if(!pTable->GetHostRoom()->CanEnterRoom(pRobot) || !pTable->CanEnterTable(pRobot)){
            continue;
        }
        LOG_DEBUG("分配到一个机器人:%d",pRobot->GetUID());
        pTable->GetHostRoom()->EnterRoom(pRobot);
        if(pTable->EnterTable(pRobot))
            return true;
    }
    return false;
}
bool    CRobotMgr::AddRobot(shared_ptr<CGameRobot> pRobot)
{
    if(GetRobot(pRobot->GetUID()) != nullptr)
        return false;

    m_mpRobots.insert(make_pair(pRobot->GetUID(),pRobot));
    LOG_DEBUG("增加一个机器人：{},size:{}",pRobot->GetUID(),m_mpRobots.size());
    CPlayerMgr::Instance().AddPlayer(pRobot);

    return true;
}
bool    CRobotMgr::RemoveRobot(shared_ptr<CGameRobot> pRobot)
{
    m_mpRobots.erase(pRobot->GetUID());
    CPlayerMgr::Instance().RecoverPlayer(pRobot);

    LOG_DEBUG("移除一个机器人:{},size:{}",pRobot->GetUID(),m_mpRobots.size());
    return true;
}
// 空闲机器人数量
uint32_t CRobotMgr::GetFreeRobotNum()
{
    uint32_t num = 0;
    auto it = m_mpRobots.begin();
    for(;it != m_mpRobots.end();++it)
    {
        auto pRobot = it->second;
        if(pRobot->GetRoom() != nullptr || pRobot->GetTable() != nullptr){
            continue;
        }
        num++;
    }
    return num;
}



