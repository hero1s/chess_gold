/*
* game_server.cpp
*
*  modify on: 2019-9-6
*      Author: toney
*/
#include "game_define.h"
#include "framework/application.h"
#include "svrlib.h"
#include <iostream>
#include "game_server_config.h"
#include "time/time.hpp"
#include "data_cfg_mgr.h"
#include "lua_service/lua_bind.h"
#include "net/center_client.h"
#include "redis_mgr/redis_mgr.h"
#include "player_mgr.h"
#include "net/lobby_mgr.h"
#include "lua_logic/lua_logic.h"
#include "game_table/game_room_mgr.h"
#include "game_logic_mgr.h"
#include "mysql_mgr/dbmysql_mgr.h"

using namespace svrlib;
using namespace std;

bool CApplication::Initialize() {
    defLuaConfig(m_solLua);
    defLuaLogic(m_solLua);

    // 加载lua 配置
    auto lubBind = lua_bind(m_solLua);
    lubBind.add_lua_cpath({"clualib"});
    lubBind.add_lua_path({"lualib", "lua"});

    lubBind.reload_lua_dir("lua");
    SOL_CALL_LUA(m_solLua["init_lua_service"](m_luaService));

    bool bRet = m_solLua["game_config"](m_uiServerID, &GameServerConfig::Instance());
    if (bRet == false)
    {
        LOG_ERROR("load game_config fail ");
        return false;
    }
    LOG_INFO("load config is:id:{}", m_uiServerID);
    // db
    if (CDBMysqlMgr::Instance().Init(GameServerConfig::Instance().DBConfs) == false)
    {
        LOG_ERROR("init mysqlmgr fail ");
        return false;
    }
    if (CDataCfgMgr::Instance().Init(emSERVER_TYPE_GAME) == false)
    {
        LOG_ERROR("init datamgr fail ");
        return false;
    }

    if (!CRedisMgr::Instance().Init(m_ioContext, GameServerConfig::Instance().redisConfs[0]))
    {
        LOG_ERROR("init redis fail");
        return false;
    }
    if (!CPlayerMgr::Instance().Init())
    {
        LOG_ERROR("playermgr init fail");
        return false;
    }
    if (CLobbyMgr::Instance().Init() == false)
    {
        LOG_ERROR("init lobbymgr fail");
        return false;
    }
    if (CGameRoomMgr::Instance().Init() == false)
    {
        LOG_ERROR("init game room mgr fail");
        return false;
    }
    if (CGameLogicMgr::Instance().Init() == false)
    {
        LOG_ERROR("init game logic mgr fail");
        return false;
    }

    net::svr::server_info info;
    info.set_svrid(GetServerID());
    info.set_game_type(CDataCfgMgr::Instance().GetGameType());
    auto plays = CDataCfgMgr::Instance().GetGamePlayType();
    for (uint16_t i = 0; i < plays.size(); ++i)
    {
        info.add_play_types(plays[i]);
    }
    info.set_svr_type(emSERVER_TYPE_GAME);
    info.set_uuid(m_uuid);

    //连接中心服
    auto centerIp = m_solLua.get<sol::table>("server_config").get<sol::table>("center");
    if (CCenterClientMgr::Instance().Init(info, centerIp.get<string>("ip"), centerIp.get<int>("port"), "center_connector", 1) == false)
    {
        LOG_ERROR("init center client mgr fail");
        return false;
    }

    m_luaService->start();

    return true;
}

void CApplication::ShutDown() {

    CRedisMgr::Instance().ShutDown();
    CDBMysqlMgr::Instance().ShutDown();
    m_luaService->exit();
}

/**
* 本函数将在程序启动时和每次配置改变时被调用。
* 第一次调用将在Initialize()之前
*/
void CApplication::ConfigurationChanged() {
    // 重加载配置
    LOG_INFO("configuration changed");
    CDataCfgMgr::Instance().Reload();
    auto lubBind = lua_bind(m_solLua);
    lubBind.reload_lua_dir("lua");
}

void CApplication::Tick(uint64_t diffTime) {

}

void CApplication::ExceptionHandle() {

}

int main(int argc, char *argv[]) {
    return FrameworkMain(argc, argv);
}
