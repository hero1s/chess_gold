
#pragma once

#include <string>
#include "svrlib.h"
#include "dbmysql/dbmysql.h"
#include <vector>
#include "db_operator.h"
#include "db_struct_define.h"
#include <queue>
#include "dbmysql/db_task.h"
#include "game_define.h"
#include "config/config.h"
#include <memory>

using namespace std;
using namespace svrlib;

class CDBMysqlMgr : public AutoDeleteSingleton<CDBMysqlMgr> {
public:
    CDBMysqlMgr();

    ~CDBMysqlMgr();

    void OnTimer();

    bool Init(const vector<stDBConf> &DBConfs);

    void ShutDown();

    // 加载玩家数据
    void AsyncLoadPlayerData(uint32_t uid, uint8_t dataType, std::function<void(shared_ptr<CDBEventRep> &pRep)> callBack);
    void AsyncLoadMissionData(uint32_t uid, std::function<void(shared_ptr<CDBEventRep>& pRep)> callBack);

    // 添加异步SQL语句
    void AddAsyncSql(uint8_t dbType, string strSql);
// 数据库操作接口    
public:
    // 上报服务器在线人数
    void ReportOnlines();
    // 更新登录信息
    void UpdatePlayerLoginInfo(uint32_t uid, uint32_t offlinetime, uint32_t clogin, uint32_t weeklogin, uint32_t bankrupt);
    // 更新gps信息
    void UpdatePlayerGPS(uint32_t uid, double lon, double lat);
    // 保存用户任务信息
    void SaveUserMission(uint32_t uid, map<uint32_t, stUserMission>& missions);
    // 修改玩家账号数值（增量修改）
    void ChangeAccountValue(uint32_t uid, int64_t coin, int64_t safecoin);
public:
    // 在线玩家金流日志
    void AccountTransction(uint32_t uid, uint16_t atype, uint16_t ptype, uint32_t sptype, int64_t amount, int64_t oldv, int64_t newv, const string& chessid);
    // 离线玩家金流日志
    void OfflineAccountTransctionCoin(uint32_t uid, uint16_t operType, uint16_t subType, int64_t coin, int64_t safecoin, int64_t curCoin, int64_t curSafeCoin, const string& chessid);
    // 牌局日志
    void WriteGameBlingLog(stGameBlingLog& log);

// 同步数据库操作
public:
    CDBOperator &GetSyncDBOper(uint8_t dbIndex);

protected:
    string GetDBName(uint8_t dbType);

    // 添加DBEvent
    void AddAsyncDBEvent(uint8_t dbType, shared_ptr<CDBEventReq> &pReq);

public:

protected:
    string GetDayGameTableName(int64_t time);

    string GetDayTranscTableName(int64_t time);

private:
    // 启动日志异步线程
    bool StartAsyncDB();

    // 停止日志异步线程
    bool StopAsyncDB();

    // 连接配置数据库
    bool ConnectSyncDB();

private:
    // 同步数据库操作
    CDBOperator m_syncDBOper;                           // 同步数据库
    // 异步数据库操作
    vector<shared_ptr<CDBTask>> m_pAsyncTasks;          // 异步数据库线程

    vector<stDBConf> m_DBConfs;                        // 数据库配置信息

    uint16_t m_svrID;
    MemberTimerEvent<CDBMysqlMgr, &CDBMysqlMgr::OnTimer> m_reportTimer;

    SQLJoin m_sqlJoinData;
    SQLJoin m_sqlJoinWhere;
};



