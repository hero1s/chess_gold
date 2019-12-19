
#pragma once

#include <string>
#include "svrlib.h"
#include "db_struct_define.h"
#include <vector>
#include "game_define.h"
#include <unordered_map>
#include <unordered_set>

using namespace std;
using namespace svrlib;

/*************************************************************/
class CDataCfgMgr : public AutoDeleteSingleton<CDataCfgMgr> {
public:
    CDataCfgMgr();

    virtual ~CDataCfgMgr();

public:
    bool Init(emSERVER_TYPE svrType);

    void ShutDown();

    bool Reload();

    // 获取当前服务器信息
    emSERVER_TYPE GetSvrType();

    uint16_t GetGameType();

    string GetSvrName();

    string GetSvrIP();

    string GetSvrLanIP();

    uint32_t GetPort();

    uint32_t GetLanPort();

    uint32_t GetPhpPort();


    const vector<uint16_t> &GetGamePlayType();

    // 任务配置
    const stMissionCfg *GetMissionCfg(uint32_t missid);

    // 任务配置
    const unordered_map<uint32_t, stMissionCfg> &GetAllMissionCfg() {
        return m_Missioncfg;
    }

    const string &GetMd5Key() {
        return m_md5Key;
    }

protected:
    using MAP_MISSCFG = unordered_map<uint32_t, stMissionCfg>;
    MAP_MISSCFG m_Missioncfg;
    emSERVER_TYPE m_svrType;
    uint16_t m_gameType = 0;
    vector<uint16_t> m_playTypes;
    string m_md5Key;                 // 登录key

    string m_svrName = "test";
    string m_svrip = "";
    string m_svrlanip = "";
    uint32_t m_svrport = 0;
    uint32_t m_svrlanport = 0;
    uint32_t m_phpport = 0;

};

