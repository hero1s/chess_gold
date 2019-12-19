#include "error_code.pb.h"
#include "data_cfg_mgr.h"
#include "common_logic.h"
#include "crypt/md5.h"
#include "db_struct_define.h"
#include "msg_define.pb.h"
#include "nlohmann/json_wrap.h"
#include "file/file.hpp"
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include "mysql_mgr/dbmysql_mgr.h"

using namespace std;
using namespace svrlib;

CDataCfgMgr::CDataCfgMgr() {

}

CDataCfgMgr::~CDataCfgMgr() {

}

bool CDataCfgMgr::Init(emSERVER_TYPE svrType) {
    m_svrType = svrType;
    if (!Reload())return false;

    return true;
}

void CDataCfgMgr::ShutDown() {

}

bool CDataCfgMgr::Reload() {

    uint16_t sid = CApplication::Instance().GetServerID();

    if (m_svrType == emSERVER_TYPE_LOBBY)
    {
        CDBOperator &oper = CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG);
        m_Missioncfg.clear();
        oper.LoadMissionCfg(m_Missioncfg);
        //登陆key
        sol::table svr_keys = CApplication::Instance().GetSolLuaState()["server_key"];
        m_md5Key = svr_keys.get<string>("loginkey");
        LOG_INFO("login config md5key:{}", m_md5Key);


        auto lobbyIp = CApplication::Instance().GetSolLuaState().get<sol::table>("server_config").get<sol::table>("lobby");
        m_svrip = lobbyIp.get<sol::table>(sid).get<string>("ip");
        m_svrlanip = lobbyIp.get<sol::table>(sid).get<string>("lanip");
        m_svrport = lobbyIp.get<sol::table>(sid).get<int>("port");
        m_svrlanport = lobbyIp.get<sol::table>(sid).get<int>("in_port");
        m_phpport = lobbyIp.get<sol::table>(sid).get<int>("php_port");
        m_svrName = lobbyIp.get<sol::table>(sid).get<string>("name");
    }
    if (m_svrType == emSERVER_TYPE_GAME)
    {
        auto curSid = CApplication::Instance().GetServerID();
        sol::table game_cfg = CApplication::Instance().GetSolLuaState()["game_server_config"][curSid];
        m_gameType = game_cfg.get<int>("game_type");
        m_svrName = game_cfg.get<string>("name");
        //m_gameSubType = game_cfg.get<int>("game_sub_type");
    }
    return true;
}

// 获取当前服务器信息
emSERVER_TYPE CDataCfgMgr::GetSvrType() {
    return m_svrType;
}

uint16_t CDataCfgMgr::GetGameType() {
    return m_gameType;
}

string CDataCfgMgr::GetSvrName(){
    return m_svrName;
}

string CDataCfgMgr::GetSvrIP() {
    return m_svrip;
}

string CDataCfgMgr::GetSvrLanIP() {
    return m_svrlanip;
}

uint32_t CDataCfgMgr::GetPort() {
    return m_svrport;
}

uint32_t CDataCfgMgr::GetLanPort() {
    return m_svrlanport;
}

uint32_t CDataCfgMgr::GetPhpPort() {
    return m_phpport;
}

const vector<uint16_t> &CDataCfgMgr::GetGamePlayType() {
    return m_playTypes;
}

const stMissionCfg *CDataCfgMgr::GetMissionCfg(uint32_t missid) {
    auto it = m_Missioncfg.find(missid);
    if (it != m_Missioncfg.end())
    {
        return &it->second;
    }

    return NULL;
}





















