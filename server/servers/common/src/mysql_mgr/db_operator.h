
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
    //������Ϸ��������
    bool LoadRoomCfg(uint16_t gameType,vector<stRoomCfg>& vecRooms);
    // ��������������Ϣ
    bool LoadMissionCfg(unordered_map<uint32_t, stMissionCfg>& mapMissions);


    // ԭ���޸Ľ��
    bool AtomChangeAccountValue(uint32_t uid, int64_t coin, int64_t safecoin, int64_t& curCoin, int64_t& curSafeCoin);


};


