#include "error_code.pb.h"
#include "db_operator.h"
#include "db_struct_define.h"
#include "common_logic.h"
#include "nlohmann/json_wrap.h"

using namespace std;
using namespace svrlib;

namespace {

};

CDBOperator::CDBOperator() {
}

CDBOperator::~CDBOperator() {
}

//加载游戏房间配置
bool CDBOperator::LoadRoomCfg(uint16_t gameType, vector<stRoomCfg> &vecRooms) {
    auto sql = CStringUtility::FormatToString("select * from roomcfg where gametype=%d and isopen=1;", gameType);
    vector<CMysqlResultRow> vecData;
    int64_t affectRow = 0;
    int iRet = Query(sql, vecData, affectRow);
    if (iRet == -1)
        return false;
    vecRooms.clear();
    for (uint32_t i = 0; i < vecData.size(); ++i)
    {
        auto &refRows = vecData[i];
        stRoomCfg roomCfg;

        roomCfg.roomID = refRows["roomid"];
        roomCfg.enter_min = refRows["entermin"];
        roomCfg.enter_max = refRows["entermax"];
        roomCfg.baseScore = refRows["basescore"];
        roomCfg.tableNum = refRows["tablenum"];
        roomCfg.marry = refRows["marry"];
        roomCfg.limitEnter = refRows["limitenter"];
        roomCfg.showonline = refRows["showonline"];
        roomCfg.sitdown = refRows["sitdown"];
        roomCfg.feeType = refRows["feetype"];
        roomCfg.feeValue = refRows["fee"];
        roomCfg.seatNum = refRows["seatnum"];

        vecRooms.push_back(roomCfg);
    }

    return true;
}

// 加载任务配置信息
bool CDBOperator::LoadMissionCfg(unordered_map<uint32_t, stMissionCfg> &mapMissions) {
    auto sql = CStringUtility::FormatToString("select * from mission;");
    vector<CMysqlResultRow> vecData;
    int64_t affectRow = 0;
    int iRet = Query(sql, vecData, affectRow);
    if (iRet == -1)
        return false;
    mapMissions.clear();
    for (uint32_t i = 0; i < vecData.size(); ++i)
    {
        auto &refRows = vecData[i];
        stMissionCfg missioncfg;
        missioncfg.msid = refRows["msid"];
        missioncfg.type = refRows["type"];
        missioncfg.status = refRows["status"];
        missioncfg.autoprize = refRows["auto"];
        missioncfg.cate1 = refRows["cate1"];
        missioncfg.cate2 = refRows["cate2"];
        missioncfg.cate3 = refRows["cate3"];
        missioncfg.cate4 = refRows["cate4"];
        missioncfg.mtimes = refRows["mtimes"];
        missioncfg.straight = refRows["straight"];
        missioncfg.cycle = refRows["cycle"];
        missioncfg.cycletimes = refRows["cycletimes"];

        auto sql2 = CStringUtility::FormatToString("select * from missionprize where msid = %d;", missioncfg.msid);
        vector<CMysqlResultRow> vecDataPri;
        Query(sql2.c_str(), vecDataPri, affectRow);
        for (uint32_t j = 0; j < vecDataPri.size(); ++j)
        {
            stMissionPrizeCfg prizeCfg;
            prizeCfg.poid = vecDataPri[j]["poid"];
            prizeCfg.qty = vecDataPri[j]["qty"];
            missioncfg.missionprize.push_back(prizeCfg);
        }
        mapMissions.insert(make_pair(missioncfg.msid, missioncfg));
    }

    return true;
}

bool CDBOperator::AtomChangeAccountValue(uint32_t uid, int64_t coin, int64_t safecoin, int64_t &curCoin, int64_t &curSafeCoin) {
    auto sql = CStringUtility::FormatToString("CALL p_change_coin_safecoin(%d,%lld,%lld);", uid, coin, safecoin);
    vector<CMysqlResultRow> vecData;
    int64_t affectRow = 0;
    int iRet = Query(sql, vecData, affectRow);
    if (iRet == -1)
        return false;
    if (vecData.size() > 0)
    {
        auto &refRows = vecData[0];
        bool bRet = refRows["ret"];
        curCoin = refRows["coin"];
        curSafeCoin = refRows["safecoin"];
        return bRet;
    }
    LOG_ERROR("修改玩家数据失败:{}--{}--{}", uid, coin, safecoin);
    return false;
}












