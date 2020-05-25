
#include "server_connector.h"
#include "data_cfg_mgr.h"
#include "player_mgr.h"
#include "player_base.h"

using namespace std;
using namespace svrlib;

//----------------------------------------------------------------------------------------------------------------------------

CSvrConnectorMgr::CSvrConnectorMgr()
        : m_timer(this) {
    m_pClientPtr = nullptr;
    m_isRun = false;
    bind_handler(this, net::svr::S2S_MSG_REGISTER_REP, &CSvrConnectorMgr::handle_msg_register_svr_rep);
    bind_handler(this, net::svr::S2S_MSG_SERVER_LIST_REP, &CSvrConnectorMgr::handle_msg_server_list_rep);
}

CSvrConnectorMgr::~CSvrConnectorMgr() {
    if (m_pClientPtr)
    {
        m_pClientPtr->Disconnect();
        m_pClientPtr = nullptr;
    }
}

void CSvrConnectorMgr::OnTimer() {
    AUTOPROFILE("CSvrConnectorMgr::OnTimer");
    CApplication::Instance().schedule(&m_timer, 3000);

    if (CApplication::Instance().GetStatus() != m_curSvrInfo.status())
    {
        LOG_DEBUG("server status change and report status :{},{}", m_pClientPtr->GetName(), CApplication::Instance().GetStatus());
        m_curSvrInfo.set_status(CApplication::Instance().GetStatus());

        net::svr::msg_change_server_info msg;
        auto info = msg.mutable_info();
        *info = m_curSvrInfo;
        SendMsg2Svr(&msg, net::svr::S2S_MSG_CHANGE_SERVER_INFO, 0);
    }

}

bool CSvrConnectorMgr::Init(const net::svr::server_info &info, string ip, uint32_t port, string svrName, uint16_t svrid) {
    LOG_DEBUG("server connector to :{}:{}", ip, port);
    m_pClientPtr = std::make_shared<TCPClient>(CApplication::Instance().GetAsioContext(), ip, port, svrName);
    m_pClientPtr->SetUID(svrid);
    m_pClientPtr->SetConnCallback([this](const TCPConnPtr &conn)
                                  {
                                      if (conn->IsConnected())
                                      {
                                          this->OnConnect(true, conn);
                                          LOG_DEBUG("{},connection accepted", conn->GetName());
                                      }
                                      else
                                      {
                                          LOG_DEBUG("{},connection disconnecting", conn->GetName());
                                          this->OnCloseClient(conn);
                                      }
                                  });
    m_pClientPtr->SetMessageCallback([this](const TCPConnPtr &conn, uint8_t* pData,uint32_t length)
                                     {
                                         this->OnHandleClientMsg(conn, pData, length);
                                         //LOG_DEBUG("{} recv msg {}",m_pClientPtr->GetName(), buffer.Size());
                                     });

    m_pClientPtr->Connect();

    m_curSvrInfo = info;
    m_isRun = false;
    m_svrID = svrid;
    m_curSvrInfo.set_status(CApplication::Instance().GetStatus());
    CApplication::Instance().schedule(&m_timer, 3000);

    return true;
}

void CSvrConnectorMgr::Register() {
    net::svr::msg_register_svr_req msg;
    net::svr::server_info *info = msg.mutable_info();
    *info = m_curSvrInfo;

    SendMsg2Svr(&msg, net::svr::S2S_MSG_REGISTER, 0);
    LOG_DEBUG("{} register server svrid:{} svrtype:{}--gameType:{}", m_pClientPtr->GetName(), msg.info().svrid(), msg.info().svr_type(),
              msg.info().game_type());
}

void CSvrConnectorMgr::RegisterRep(uint16_t svrid, bool rep) {
    LOG_DEBUG("{} register server Rep svrid:{}--res:{}", m_pClientPtr->GetName(), svrid, rep);

    m_isRun = rep;
}

void CSvrConnectorMgr::OnConnect(bool bSuccess, const TCPConnPtr &conn) {
    LOG_ERROR("{} on connect {},{}", conn->GetName(), bSuccess, conn->GetUID());
    if (bSuccess)
    {
        m_isRun = true;
        Register();
    }
}

void CSvrConnectorMgr::OnCloseClient(const TCPConnPtr &conn) {
    LOG_ERROR("{} OnClose:{}", conn->GetName(), conn->GetUID());
    m_isRun = false;
}

bool CSvrConnectorMgr::IsRun() {
    return m_isRun;
}

uint16_t CSvrConnectorMgr::GetSvrID() {
    return m_svrID;
}

void CSvrConnectorMgr::SendMsg2Svr(const google::protobuf::Message *msg, uint16_t msg_type, uint32_t uin, uint8_t route,
                                   uint32_t routeID) {
    if (!m_isRun || m_pClientPtr == nullptr)
    {
        LOG_ERROR("the connector is not runing");
        return;
    }
    pkg_inner::SendProtobufMsg(m_pClientPtr->GetTCPConn(), msg, msg_type, uin, route, routeID);
}

bool CSvrConnectorMgr::IsExistSvr(uint16_t sid) {
    return m_allSvrList.find(sid) != m_allSvrList.end();
}

//服务器注册
int CSvrConnectorMgr::handle_msg_register_svr_rep() {
    net::svr::msg_register_svr_rep msg;
    PARSE_MSG(msg);

    LOG_DEBUG("{} server register result :{}", m_pClientPtr->GetName(), msg.result());
    if (msg.result() == 1)
    {
        RegisterRep(m_pClientPtr->GetTCPConn()->GetUID(), true);
    }
    else
    {
        RegisterRep(m_pClientPtr->GetTCPConn()->GetUID(), false);
        LOG_ERROR("server register fail {} -->:{}", m_pClientPtr->GetTCPConn()->GetUID(),
                  CApplication::Instance().GetServerID());
    }
    return 0;
}

//更新服务器列表
int CSvrConnectorMgr::handle_msg_server_list_rep() {
    net::svr::msg_server_list_rep msg;
    PARSE_MSG(msg);

    LOG_DEBUG("{} server rep svrlist :{}", m_pClientPtr->GetName(), msg.server_list_size());
    m_allSvrList.clear();
    for (int i = 0; i < msg.server_list_size(); ++i)
    {
        m_allSvrList.insert(make_pair(msg.server_list(i).svrid(), msg.server_list(i)));
    }

    return 0;

}









