//
// Created by toney on 2018/3/1.
//
#pragma once

#include "svrlib.h"
#include "db_struct_define.h"

class CGameTable;
class CGameRoom;

class CGameLogicMgr : public AutoDeleteSingleton<CGameLogicMgr>
{
public:
	CGameLogicMgr();

	~CGameLogicMgr();

	bool Init();

	void ShutDown();

protected:
    //初始化斗地主房间
    void InitLandRoom(uint16_t gameType,stRoomCfg& cfg);
    //初始化跑得快房间
    void InitKuaiPaoRoom(uint16_t gameType,stRoomCfg& cfg);

};



