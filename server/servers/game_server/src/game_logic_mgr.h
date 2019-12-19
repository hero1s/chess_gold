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
    //��ʼ������������
    void InitLandRoom(uint16_t gameType,stRoomCfg& cfg);
    //��ʼ���ܵÿ췿��
    void InitKuaiPaoRoom(uint16_t gameType,stRoomCfg& cfg);

};



