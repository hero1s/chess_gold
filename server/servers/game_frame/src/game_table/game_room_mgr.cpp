
#include "game_room_mgr.h"
#include "data_cfg_mgr.h"
#include "player_mgr.h"

using namespace std;
using namespace svrlib;

CGameRoomMgr::CGameRoomMgr() {

}

CGameRoomMgr::~CGameRoomMgr() {

}

bool CGameRoomMgr::Init() {

    return true;
}

void CGameRoomMgr::ShutDown() {
    for (auto &it : m_mpRooms)
    {
        auto pRoom = it.second;
        pRoom->ShutDown();
    }
    m_mpRooms.clear();
}

bool CGameRoomMgr::AddRoom(std::shared_ptr<CGameRoom> pRoom){
    if(GetRoom(pRoom->GetRoomID()) != nullptr)
    {
        LOG_ERROR("cann't add room,the room is exist:{}",pRoom->GetRoomID());
        return false;
    }
    m_mpRooms.insert(make_pair(pRoom->GetRoomID(),pRoom));
    return true;
}

bool CGameRoomMgr::RemoveRoom(std::shared_ptr<CGameRoom> pRoom){
    if(pRoom->GetPlayerNum() > 0)
    {
        LOG_ERROR("the room is playing and cann't remove:{}",pRoom->GetRoomID());
        return false;
    }
    m_mpRooms.erase(pRoom->GetRoomID());
    return true;
}

std::shared_ptr<CGameRoom> CGameRoomMgr::GetRoom(uint32_t roomID) {
    auto it = m_mpRooms.find(roomID);
    if (it != m_mpRooms.end())
    {
        return it->second;
    }
    return nullptr;
}

void CGameRoomMgr::GetRoomList2Client(net::cli::msg_rooms_info_rep& roomList,uint32_t uid) {
    auto pGamePlayer = CPlayerMgr::Instance().GetPlayer<CGamePlayer>(uid);
    if(pGamePlayer != nullptr)
    {
        roomList.set_cur_roomid(pGamePlayer->GetRoomID());
    }
    for (auto &it : m_mpRooms)
    {
        net::room_info *pRoom = roomList.add_rooms();
        auto pGameRoom = it.second;
        pGameRoom->GetRoomInfo(pRoom);
    }
}

bool CGameRoomMgr::FastJoinRoom(std::shared_ptr<CGamePlayer> pPlayer) {
    auto pOldRoom = pPlayer->GetRoom();
    if (pOldRoom != nullptr)
    {
        if (!pOldRoom->CanLeaveRoom(pPlayer))
        {
            return false;
        }
        if (!pOldRoom->LeaveRoom(pPlayer))
        {
            return false;
        }
    }
    vector<std::shared_ptr<CGameRoom>> rooms;
    GetRoomList(rooms);
    for (auto pRoom : rooms)
    {
        if (pRoom->GetPlayerNum() > 0 && pRoom->CanEnterRoom(pPlayer))
        {
            pRoom->EnterRoom(pPlayer);
            pRoom->FastJoinTable(pPlayer);
            return true;
        }
    }
    for (auto pRoom : rooms)
    {
        if (pRoom->CanEnterRoom(pPlayer))
        {
            pRoom->EnterRoom(pPlayer);
            pRoom->FastJoinTable(pPlayer);
            return true;
        }
    }

    return false;
}

void CGameRoomMgr::GetRoomList(vector<std::shared_ptr<CGameRoom>> &rooms) {
    for (auto &it : m_mpRooms)
    {
        auto pGameRoom = it.second;
        rooms.push_back(pGameRoom);
    }
    std::sort(rooms.begin(), rooms.end(), [](auto pRoom1, auto pRoom2)
    {
        return pRoom1->GetEnterMin() > pRoom2->GetEnterMin();
    });
}













