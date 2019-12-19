
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

    // �����������
    void AsyncLoadPlayerData(uint32_t uid, uint8_t dataType, std::function<void(shared_ptr<CDBEventRep> &pRep)> callBack);
    void AsyncLoadMissionData(uint32_t uid, std::function<void(shared_ptr<CDBEventRep>& pRep)> callBack);

    // ����첽SQL���
    void AddAsyncSql(uint8_t dbType, string strSql);
// ���ݿ�����ӿ�    
public:
    // �ϱ���������������
    void ReportOnlines();
    // ���µ�¼��Ϣ
    void UpdatePlayerLoginInfo(uint32_t uid, uint32_t offlinetime, uint32_t clogin, uint32_t weeklogin, uint32_t bankrupt);
    // ����gps��Ϣ
    void UpdatePlayerGPS(uint32_t uid, double lon, double lat);
    // �����û�������Ϣ
    void SaveUserMission(uint32_t uid, map<uint32_t, stUserMission>& missions);
    // �޸�����˺���ֵ�������޸ģ�
    void ChangeAccountValue(uint32_t uid, int64_t coin, int64_t safecoin);
public:
    // ������ҽ�����־
    void AccountTransction(uint32_t uid, uint16_t atype, uint16_t ptype, uint32_t sptype, int64_t amount, int64_t oldv, int64_t newv, const string& chessid);
    // ������ҽ�����־
    void OfflineAccountTransctionCoin(uint32_t uid, uint16_t operType, uint16_t subType, int64_t coin, int64_t safecoin, int64_t curCoin, int64_t curSafeCoin, const string& chessid);
    // �ƾ���־
    void WriteGameBlingLog(stGameBlingLog& log);

// ͬ�����ݿ����
public:
    CDBOperator &GetSyncDBOper(uint8_t dbIndex);

protected:
    string GetDBName(uint8_t dbType);

    // ���DBEvent
    void AddAsyncDBEvent(uint8_t dbType, shared_ptr<CDBEventReq> &pReq);

public:

protected:
    string GetDayGameTableName(int64_t time);

    string GetDayTranscTableName(int64_t time);

private:
    // ������־�첽�߳�
    bool StartAsyncDB();

    // ֹͣ��־�첽�߳�
    bool StopAsyncDB();

    // �����������ݿ�
    bool ConnectSyncDB();

private:
    // ͬ�����ݿ����
    CDBOperator m_syncDBOper;                           // ͬ�����ݿ�
    // �첽���ݿ����
    vector<shared_ptr<CDBTask>> m_pAsyncTasks;          // �첽���ݿ��߳�

    vector<stDBConf> m_DBConfs;                        // ���ݿ�������Ϣ

    uint16_t m_svrID;
    MemberTimerEvent<CDBMysqlMgr, &CDBMysqlMgr::OnTimer> m_reportTimer;

    SQLJoin m_sqlJoinData;
    SQLJoin m_sqlJoinWhere;
};



