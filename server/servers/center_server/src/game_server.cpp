/*
* game_server.cpp
*
*  modify on: 2019-9-6
*      Author: toney
*/
#include "game_define.h"
#include "framework/application.h"
#include "game_server_config.h"
#include "center_mgr.h"
#include "svrlib.h"
#include <iostream>
#include "time/time.hpp"
#include "msg_define.pb.h"
#include "network/tcp_server.h"

using namespace svrlib;
using namespace std;

bool CApplication::Initialize() {
    defLuaConfig(m_solLua);

    // 加载lua 配置
    bool bRet = m_solLua["center_config"](m_uiServerID, &GameServerConfig::Instance());
    if (bRet == false)
    {
        LOG_ERROR("load center_config fail ");
        return false;
    }
    LOG_INFO("load config is:id:{},uuid:{}", m_uiServerID, m_uuid);
    if (CCenterMgr::Instance().Init() == false)
    {
        LOG_ERROR("CenterMgr init fail");
        return false;
    }

    {
        auto centerIp = m_solLua.get<sol::table>("server_config").get<sol::table>("center");
        auto tcpSvr = std::make_shared<TCPServer>(m_ioContext, centerIp.get<string>("ip"), centerIp.get<int>("port"), "centerServer");
        tcpSvr->SetConnectionCallback([](const TCPConnPtr &conn)
                                      {
                                          if (conn->IsConnected())
                                          {
                                              LOG_DEBUG("{},connection accepted", conn->GetName());
                                              //conn->SetHeartTimeOut(60);
                                          }
                                          else
                                          {
                                              CCenterMgr::Instance().RemoveServer(conn);
                                              LOG_DEBUG("{},connection disconnecting", conn->GetName());
                                          }
                                      });
        tcpSvr->SetMessageCallback([](const TCPConnPtr &conn, uint8_t* pData,uint32_t length)
                                   {
                                       //LOG_DEBUG("recv msg {}",buffer.Size());
                                       CCenterMgr::Instance().OnHandleClientMsg(conn, pData, length);
                                   });
        tcpSvr->Start();
        m_tcpServers.push_back(tcpSvr);

    }


    LOG_INFO("center server start is successed {}", m_uiServerID);

    return true;
}

void CApplication::ShutDown() {
    CCenterMgr::Instance().ShutDown();
}

/**
* 本函数将在程序启动时和每次配置改变时被调用。
* 第一次调用将在Initialize()之前
*/
void CApplication::ConfigurationChanged() {
    // 重加载配置
    LOG_INFO("configuration changed");

}

void CApplication::Tick(uint64_t diffTime) {

}

void CApplication::ExceptionHandle() {

}

int main(int argc, char *argv[]) {
    return FrameworkMain(argc, argv);
}
