//
// Created by toney

#pragma once

#include "svrlib.h"
#include "game_room.h"
#include "msg_define.pb.h"
#include <unordered_map>

class CGamePlayer;

class CGameRoomMgr : public AutoDeleteSingleton<CGameRoomMgr> {
public:
    CGameRoomMgr();

    ~CGameRoomMgr();

    bool Init();

    void ShutDown();

    bool AddRoom(std::shared_ptr<CGameRoom> pRoom);

    bool RemoveRoom(std::shared_ptr<CGameRoom> pRoom);

    std::shared_ptr<CGameRoom> GetRoom(uint32_t roomID);

    // ���ͷ����б�
    void GetRoomList2Client(net::cli::msg_rooms_info_rep& roomList,uint32_t uid);

    // ���ټ��뷿��
    bool FastJoinRoom(std::shared_ptr<CGamePlayer> pPlayer);

    // ���ָ���������ͷ���
    void GetRoomList(vector<std::shared_ptr<CGameRoom>> &rooms);

private:
    using MAP_ROOMS = unordered_map<uint32_t, std::shared_ptr<CGameRoom>>;
    MAP_ROOMS m_mpRooms;

};


