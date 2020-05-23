/*
* game_server.cpp
*
*  modify on: 2019-9-6
*      Author: toney
*/
#include "game_define.h"
#include "framework/application.h"
#include "data_cfg_mgr.h"
#include "svrlib.h"
#include <iostream>
#include "game_server_config.h"
#include "time/time.hpp"
#include "lua_logic/lua_logic.h"
#include "net/center_client.h"
#include "mysql_mgr/dbmysql_mgr.h"
#include "player_mgr.h"
#include "redis_mgr/redis_mgr.h"
#include "player.h"
#include "lua_service/lua_bind.h"
#include "client_msg/msg_client_handle.h"
#include "client_msg/msg_plat_handle.h"
#include "net/game_server_mgr.h"

using namespace svrlib;
using namespace std;

bool CApplication::Initialize() {
    defLuaConfig(m_solLua);
    defLuaLogic(m_solLua);

    // 加载lua 配置
    auto lubBind = lua_bind(m_solLua);
    lubBind.add_lua_cpath({"clualib"});
    lubBind.add_lua_path({"lualib", "lua", "scp_lua"});

    lubBind.reload_lua_dir("lua");

    SOL_CALL_LUA(m_solLua["init_lua_service"](m_luaService));

    bool bRet = m_solLua["lobby_config"](m_uiServerID, &GameServerConfig::Instance());
    if (bRet == false)
    {
        LOG_ERROR("load lobby_config fail ");
        return false;
    }
    LOG_INFO("load config is:id:{},uuid:{}", m_uiServerID, m_uuid);
    // db
    if (CDBMysqlMgr::Instance().Init(GameServerConfig::Instance().DBConfs) == false)
    {
        LOG_ERROR("init mysqlmgr fail ");
        return false;
    }
    // 读取配置信息
    if (CDataCfgMgr::Instance().Init(emSERVER_TYPE_LOBBY) == false)
    {
        LOG_ERROR("init datamgr fail ");
        return false;
    }
    if (CGameServerMgr::Instance().Init() == false)
    {
        LOG_ERROR("GameServerMgr init fail");
        return false;
    }


    {
        string ip = CDataCfgMgr::Instance().GetSvrIP();
        string lanip = CDataCfgMgr::Instance().GetSvrLanIP();
        uint32_t port = CDataCfgMgr::Instance().GetPort();
        uint32_t in_port = CDataCfgMgr::Instance().GetLanPort();
        uint32_t php_port = CDataCfgMgr::Instance().GetPhpPort();

        // 客户端对外端口
        auto tcpSvr = std::make_shared<TCPServer>(m_ioContext, ip, port, "lobbyServerCli", false);
        tcpSvr->SetConnectionCallback([](const TCPConnPtr &conn)
                                      {
                                          if (conn->IsConnected())
                                          {
                                              LOG_DEBUG("{},connection accepted", conn->GetName());
                                              //conn->SetHeartTimeOut(20,20);
                                          }
                                          else
                                          {
                                              LOG_ERROR("client ondisconnect:{}--{}", conn->GetUID(), conn->GetRemoteAddress());
                                              uint32_t uid = conn->GetUID();
                                              auto pPlayer = CPlayerMgr::Instance().GetPlayer<CPlayer>(uid);
                                              if (pPlayer != nullptr)
                                              {
                                                  // 不直接断线，保留一定时间
                                                  pPlayer->SetSession(nullptr);
                                                  pPlayer->NotifyNetState2GameSvr(0);
                                              }
                                              LOG_DEBUG("{},connection disconnecting", conn->GetName());
                                          }
                                      });
        tcpSvr->SetMessageCallback([](const TCPConnPtr &conn, uint8_t* pData,uint32_t length)
                                   {
                                       LOG_DEBUG("recv client msg {}",length);
                                       CHandleClientMsg::Instance().OnHandleClientMsg(conn, pData, length);
                                   });
        tcpSvr->Start();
        m_tcpServers.push_back(tcpSvr);

        //游戏服端口
        tcpSvr = std::make_shared<TCPServer>(m_ioContext, lanip, in_port, "lobbyServerSvr");
        tcpSvr->SetConnectionCallback([](const TCPConnPtr &conn)
                                      {
                                          if (conn->IsConnected())
                                          {
                                              LOG_DEBUG("{},connection accepted", conn->GetName());
                                          }
                                          else
                                          {
                                              LOG_ERROR("gameServer ondisconnect:{}--{}", conn->GetUID(), conn->GetRemoteAddress());
                                              CGameServerMgr::Instance().RemoveServer(conn);
                                          }
                                      });
        tcpSvr->SetMessageCallback([](const TCPConnPtr &conn, uint8_t* pData,uint32_t length)
                                   {
                                       //LOG_DEBUG("recv msg {}",std::string(buffer.Data(), buffer.Size()));
                                       CGameServerMgr::Instance().OnHandleClientMsg(conn, pData, length);
                                   });
        tcpSvr->Start();
        m_tcpServers.push_back(tcpSvr);

        //php服务器端口
        tcpSvr = std::make_shared<TCPServer>(m_ioContext, lanip, php_port, "lobbyServerPHP");
        tcpSvr->SetConnectionCallback([](const TCPConnPtr &conn)
                                      {
                                          if (conn->IsConnected())
                                          {
                                              LOG_DEBUG("{},connection accepted", conn->GetName());
                                              conn->SetHeartTimeOut(20,20);
                                          }
                                          else
                                          {
                                              LOG_DEBUG("PHPServer ondisconnect:{}--{}", conn->GetUID(), conn->GetRemoteAddress());
                                          }
                                      });
        tcpSvr->SetMessageCallback([](const TCPConnPtr &conn, uint8_t* pData,uint32_t length)
                                   {
                                       LOG_DEBUG("recv PHP msg {}",std::string((char*)pData, length));
                                       CHandlePlatMsg::Instance().OnRecvClientMsg(conn, pData, length);
                                   });
        tcpSvr->Start();
        m_tcpServers.push_back(tcpSvr);

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

    net::svr::server_info info;
    info.set_svrid(GetServerID());
    info.set_game_type(0);

    info.set_svr_type(emSERVER_TYPE_LOBBY);
    info.set_uuid(m_uuid);

    //连接中心服
    auto centerIp = m_solLua.get<sol::table>("server_config").get<sol::table>("center");
    if (CCenterClientMgr::Instance().Init(info, centerIp.get<string>("ip"), centerIp.get<int>("port"), "center_connector", 1) == false)
    {
        LOG_ERROR("init center client mgr fail");
        return false;
    }

    m_luaService->start();

    //test toney
    static TimerEvent<std::function<void()>> timer([]()
                                                   {
                                                       auto pPlayer = std::make_shared<CPlayer>(PLAYER_TYPE_ONLINE);
                                                       pPlayer->SetUID(110);
                                                       CPlayerMgr::Instance().AddPlayer(pPlayer);
                                                       pPlayer->OnLogin();
                                                   });
    schedule(&timer, rand_range(10000,20000));

    return true;
}

void CApplication::ShutDown() {

    CPlayerMgr::Instance().ShutDown();
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
    AutoProfile("CApplication::Tick");

}

void CApplication::ExceptionHandle() {

}

int main(int argc, char *argv[]) {
    return FrameworkMain(argc, argv);
}
