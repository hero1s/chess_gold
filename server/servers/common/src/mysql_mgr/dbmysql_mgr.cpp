

#include "dbmysql_mgr.h"
#include "svrlib.h"
#include <string.h>
#include <svrlib.h>
#include "common_logic.h"
#include "player_mgr.h"
#include "data_cfg_mgr.h"
#include "game_server_config.h"
#include "dbmysql/db_wrap.h"

#include "nlohmann/json.hpp"

using json = nlohmann::json;

using namespace std;
using namespace svrlib;

namespace {
    static string s_PlayerDataFields[] = {"Base", "Task"};//玩家二进制数据字段名

};

CDBMysqlMgr::CDBMysqlMgr()
        : m_reportTimer(this) {
    m_svrID = 0;
}

CDBMysqlMgr::~CDBMysqlMgr() {

}

void CDBMysqlMgr::OnTimer() {
    AutoProfile("CDBMysqlMgr::OnTimer");
    CApplication::Instance().schedule(&m_reportTimer, 20 * 1000);
    ReportOnlines();
}

bool CDBMysqlMgr::Init(const vector<stDBConf> &DBConfs) {
    AutoProfile("CDBMysqlMgr::Init");
    m_svrID = CApplication::Instance().GetServerID();
    m_DBConfs.clear();
    for (uint32_t i = 0; i < DBConfs.size(); ++i)
    {
        m_DBConfs.push_back(DBConfs[i]);
    }
    if (!ConnectSyncDB())
    {
        return false;
    }
    StartAsyncDB();

    CApplication::Instance().schedule(&m_reportTimer, 5 * 1000);

    return true;
}

void CDBMysqlMgr::ShutDown() {
    StopAsyncDB();
    m_syncDBOper.dbClose();
    m_reportTimer.cancel();
}

// 启动异步线程
bool CDBMysqlMgr::StartAsyncDB() {
    for (uint16_t i = 0; i < m_DBConfs.size(); ++i)
    {
        auto pAsyncTask = std::make_shared<CDBTask>(CApplication::Instance().GetAsioContext());
        pAsyncTask->SetDBName(m_DBConfs[i].sDBName);
        pAsyncTask->Init(m_DBConfs[i]);
        pAsyncTask->Start();
        m_pAsyncTasks.push_back(pAsyncTask);
    }
    return true;
}

// 停止日志异步线程
bool CDBMysqlMgr::StopAsyncDB() {
    for (uint16_t i = 0; i < m_pAsyncTasks.size(); ++i)
    {
        if (m_pAsyncTasks[i] != nullptr)
        {
            m_pAsyncTasks[i]->Stop();
            m_pAsyncTasks[i]->Join();
        }
    }
    m_pAsyncTasks.clear();
    return true;
}

// 连接配置服务器
bool CDBMysqlMgr::ConnectSyncDB() {
    stDBConf &refConf = m_DBConfs[0];
    bool bRet = m_syncDBOper.dbOpen(refConf.sHost, refConf.sUser, refConf.sPwd, refConf.sDBName, refConf.uPort);
    if (bRet == false)
    {
        LOG_ERROR("connect config database fail :");
        return false;
    }
    LOG_DEBUG("connect config database successful ");
    return true;
}

void CDBMysqlMgr::AddAsyncSql(uint8_t dbType, string strSql) {
    AutoProfile("CDBMysqlMgr::AddAsyncSql");
    if (dbType >= m_pAsyncTasks.size())
    {
        LOG_ERROR("overstep dbIndexTypeMax:{}", dbType);
        return;
    }
    if (m_pAsyncTasks[dbType] != nullptr)
    {
        m_pAsyncTasks[dbType]->PushAndSelectDB("", strSql);
    }
    else
    {
        LOG_ERROR("m_pAsyncTask is null");
    }
}

// 上报服务器在线人数
void CDBMysqlMgr::ReportOnlines() {
    AutoProfile("CDBMysqlMgr::ReportOnlines");
    uint32_t onlines = CPlayerMgr::Instance().GetOnlines();
    SQLJoin sqlJoin;
    sqlJoin.add_pair("svrid", CApplication::Instance().GetServerID());
    sqlJoin.add_pair("name", CDataCfgMgr::Instance().GetSvrName());
    sqlJoin.add_pair("group", 1);
    sqlJoin.add_pair("svr_type", CDataCfgMgr::Instance().GetSvrType());
    sqlJoin.add_pair("game_type", CDataCfgMgr::Instance().GetGameType());
    sqlJoin.add_pair("play_types","");
    sqlJoin.add_pair("svrip",CDataCfgMgr::Instance().GetSvrIP());
    sqlJoin.add_pair("svrlanip",CDataCfgMgr::Instance().GetSvrLanIP());
    sqlJoin.add_pair("svrport",CDataCfgMgr::Instance().GetPort());
    sqlJoin.add_pair("svrlanport",CDataCfgMgr::Instance().GetLanPort());
    sqlJoin.add_pair("phpport",CDataCfgMgr::Instance().GetPhpPort());

    sqlJoin.add_pair("state", CApplication::Instance().GetStatus());
    sqlJoin.add_pair("onlines", onlines);
    sqlJoin.add_pair("report_time", time::getSysTime());
    auto strSql = CDBWrap::GetUpdateOrInsertSql("serverinfo", sqlJoin);
    AddAsyncSql(DB_INDEX_TYPE_CFG, strSql);
}

void CDBMysqlMgr::UpdatePlayerLoginInfo(uint32_t uid, uint32_t offlinetime, uint32_t clogin, uint32_t weeklogin, uint32_t bankrupt) {
    AutoProfile("CDBMysqlMgr::UpdatePlayerLoginInfo");
    auto strSql = CStringUtility::FormatToString("UPDATE user SET offlinetime=%d,clogin=%d,weeklogin=%d,bankrupt=%d WHERE uid=%u;", \
            offlinetime, clogin, weeklogin, bankrupt, uid);
    AddAsyncSql(DB_INDEX_TYPE_ACC, strSql);
}

// 更新gps信息
void CDBMysqlMgr::UpdatePlayerGPS(uint32_t uid, double lon, double lat) {
    auto strSql = CStringUtility::FormatToString("UPDATE user SET lon=%f,lat=%f WHERE uid=%u;", lon, lat, uid);
    AddAsyncSql(DB_INDEX_TYPE_ACC, strSql);
}

// 保存任务数据
void CDBMysqlMgr::SaveUserMission(uint32_t uid, map<uint32_t, stUserMission> &missions) {
    for (auto &it:missions)
    {
        stUserMission &refMiss = it.second;
        switch (refMiss.update)
        {
            case emDB_ACTION_UPDATE:
            {
                auto strSql = CStringUtility::FormatToString("update umission set rtimes = %d,ctimes = %d,ptime = %d,cptime = %d where uid = %d and msid = %d",
                                                             refMiss.rtimes, refMiss.ctimes, refMiss.ptime, refMiss.cptime, uid, refMiss.msid);
                AddAsyncSql(DB_INDEX_TYPE_ACC, strSql);
                break;
            }
            case emDB_ACTION_INSERT:
            {
                auto strSql = CStringUtility::FormatToString("insert into umission set rtimes = %d,ctimes = %d,ptime = %d,cptime = %d, uid = %d, msid = %d",
                                                             refMiss.rtimes, refMiss.ctimes, refMiss.ptime, refMiss.cptime, uid, refMiss.msid);
                AddAsyncSql(DB_INDEX_TYPE_ACC, strSql);
                break;
            }
            case emDB_ACTION_DELETE:
            {
                auto strSql = CStringUtility::FormatToString("delete from umission where uid = %d and msid = %d", uid, refMiss.msid);
                AddAsyncSql(DB_INDEX_TYPE_ACC, strSql);
                break;
            }
            default:
                break;
        }
    }
}

// 修改玩家账号数值（增量修改）
void CDBMysqlMgr::ChangeAccountValue(uint32_t uid, int64_t coin, int64_t safecoin) {
    auto strSql = CStringUtility::FormatToString("CALL p_change_coin_safecoin(%d,%lld,%lld);", uid, coin, safecoin);
    AddAsyncSql(DB_INDEX_TYPE_ACC, strSql);
}

CDBOperator &CDBMysqlMgr::GetSyncDBOper(uint8_t dbIndex) {
    m_syncDBOper.dbSelect(GetDBName(dbIndex).c_str());
    return m_syncDBOper;
}

string CDBMysqlMgr::GetDBName(uint8_t dbType) {
    if (dbType < m_DBConfs.size())
    {
        return m_DBConfs[dbType].sDBName;
    }
    return "";
}

// 添加DBEvent
void CDBMysqlMgr::AddAsyncDBEvent(uint8_t dbType, shared_ptr<CDBEventReq> &pReq) {
    if (dbType >= m_pAsyncTasks.size())
    {
        LOG_ERROR("dbType is more than max index:{}", dbType);
        return;
    }
    if (m_pAsyncTasks[dbType] != nullptr)
    {
        m_pAsyncTasks[dbType]->AsyncQuery(pReq);
    }
    else
    {
        LOG_ERROR("m_pAsyncTask is null");
    }
}

/**
 * param: atype 货币类型
 * param: ptype 操作类型
 * param: oldv 操作之前多少
 * param: newv 操作之后多少
 */
void CDBMysqlMgr::AccountTransction(uint32_t uid, uint16_t atype, uint16_t ptype, uint32_t sptype, int64_t amount, int64_t oldv, int64_t newv, const string &chessid) {
    SQLJoin sqlJoin;
    sqlJoin.add_pair("ptime", time::getSysTime());
    sqlJoin.add_pair("uid", uid);
    sqlJoin.add_pair("atype", atype);
    sqlJoin.add_pair("ptype", ptype);
    sqlJoin.add_pair("sptype", sptype);
    sqlJoin.add_pair("amount", amount);
    sqlJoin.add_pair("oldv", oldv);
    sqlJoin.add_pair("newv", newv);
    sqlJoin.add_pair("chessid", chessid);

    string dayTable = GetDayTranscTableName(time::getSysTime());
    string sql = CDBWrap::GetInsertSql(dayTable, sqlJoin);
    AddAsyncSql(DB_INDEX_TYPE_LOG, sql);
}

// 离线玩家金流日志
void CDBMysqlMgr::OfflineAccountTransctionCoin(uint32_t uid, uint16_t operType, uint16_t subType, int64_t coin, int64_t safecoin, int64_t curCoin, int64_t curSafeCoin, const string &chessid) {
    if (coin != 0)
    {
        AccountTransction(uid, emACC_VALUE_COIN, operType, subType, coin, curCoin - coin, curCoin, chessid);
    }
    if (safecoin != 0)
    {
        AccountTransction(uid, emACC_VALUE_SAFECOIN, operType, subType, safecoin, curSafeCoin - safecoin, curSafeCoin, chessid);
    }
}

// 牌局日志
void CDBMysqlMgr::WriteGameBlingLog(stGameBlingLog &log) {
    json logValue;
    int64_t fee = 0;
    for (uint32_t i = 0; i < log.users.size(); ++i)
    {
        stBlingUser &user = log.users[i];
        json juser;
        juser["uid"] = user.uid;
        juser["oldv"] = user.oldValue;
        juser["newv"] = user.newValue;
        juser["win"] = user.win;
        juser["fee"] = user.fee;
        juser["chair"] = user.chairid;
        fee += user.fee;
        logValue.push_back(juser);
    }

    SQLJoin sqlJoin;
    sqlJoin.add_pair("gid", log.gameType);
    sqlJoin.add_pair("roomtype", log.roomType);
    sqlJoin.add_pair("consume", log.consume);
    sqlJoin.add_pair("play_type", log.playType);
    sqlJoin.add_pair("basescore", log.baseScore);
    sqlJoin.add_pair("fee", fee);
    sqlJoin.add_pair("begintime", log.startTime);
    sqlJoin.add_pair("endtime", log.endTime);
    sqlJoin.add_pair("content", log.operLog.str().data());
    sqlJoin.add_pair("tid", log.tableID);
    sqlJoin.add_pair("chessid", log.chessid);
    sqlJoin.add_pair("rid", log.roomID);
    sqlJoin.add_pair("uids", logValue.dump());

    string dayTable = GetDayGameTableName(log.startTime);
    string sql = CDBWrap::GetInsertSql(dayTable, sqlJoin);
    LOG_DEBUG("---- GameBlingLog Size is {}", sql.length());

    AddAsyncSql(DB_INDEX_TYPE_LOG, sql);
}

string CDBMysqlMgr::GetDayGameTableName(int64_t time) {
    return CStringUtility::FormatToString("daygame%s", time::date_format(time));
}

string CDBMysqlMgr::GetDayTranscTableName(int64_t time) {
    return CStringUtility::FormatToString("daytrans%s", time::date_format(time));
}

void CDBMysqlMgr::AsyncLoadPlayerData(uint32_t uid, uint8_t dataType, std::function<void(shared_ptr<CDBEventRep> &pRep)> callBack) {
    shared_ptr<CDBEventReq> pReq = m_pAsyncTasks[DB_INDEX_TYPE_ACC]->MallocDBEventReq();
    pReq->callBack = callBack;
    pReq->sqlStr = CStringUtility::FormatToString("SELECT * FROM user WHERE uid =%u limit 1;", uid);
    AddAsyncDBEvent(DB_INDEX_TYPE_ACC, pReq);
}

void CDBMysqlMgr::AsyncLoadMissionData(uint32_t uid, std::function<void(shared_ptr<CDBEventRep> &pRep)> callBack) {
    shared_ptr<CDBEventReq> pReq = m_pAsyncTasks[DB_INDEX_TYPE_ACC]->MallocDBEventReq();
    pReq->callBack = callBack;
    pReq->sqlStr = CStringUtility::FormatToString("SELECT * FROM umission WHERE uid =%u;", uid);
    AddAsyncDBEvent(DB_INDEX_TYPE_ACC, pReq);
}








