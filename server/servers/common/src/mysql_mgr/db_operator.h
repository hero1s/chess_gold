
#pragma once

#include <string>
#include "svrlib.h"
#include "dbmysql/dbmysql.h"
#include "dbmysql/db_wrap.h"
#include "db_struct_define.h"
#include <vector>
#include "game_define.h"
#include <unordered_map>

using namespace std;
using namespace svrlib;

/*************************************************************/
class CDBOperator : public CDBWrap {
public:
    CDBOperator();

    virtual ~CDBOperator();

public:
    //加载游戏房间配置
    bool LoadRoomCfg(uint16_t gameType,vector<stRoomCfg>& vecRooms);
    // 加载任务配置信息
    bool LoadMissionCfg(unordered_map<uint32_t, stMissionCfg>& mapMissions);


    // 原子修改金币
    bool AtomChangeAccountValue(uint32_t uid, int64_t coin, int64_t safecoin, int64_t& curCoin, int64_t& curSafeCoin);


};


