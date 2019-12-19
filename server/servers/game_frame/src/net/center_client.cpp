
#include "center_client.h"
#include "data_cfg_mgr.h"
#include "player_mgr.h"

using namespace std;
using namespace svrlib;

//----------------------------------------------------------------------------------------------------------------------------

CCenterClientMgr::CCenterClientMgr() {

}

CCenterClientMgr::~CCenterClientMgr() {

}

int CCenterClientMgr::OnRecvClientMsg() {
    if (CProtobufHandleBase::OnRecvClientMsg() == 1)
    {
        return route_to_client();
    }
    return 0;
}

// 转发给客户端
int CCenterClientMgr::route_to_client() {
    LOG_DEBUG("center server route to client message :uid:{}--cmd:{}", m_head->uin, m_head->msgID);
    auto pPlayer = GetPlayer();
    if (pPlayer != nullptr)
    {
        //pPlayer->OnMessage(m_head->msgID, m_pkt_buf, m_buf_len);
    }
    else
    {
        LOG_DEBUG("center server route to client message,client not exist:{}", m_head->uin);
    }
    return 0;
}

std::shared_ptr<CGamePlayer> CCenterClientMgr::GetPlayer() {
    return CPlayerMgr::Instance().GetPlayer<CGamePlayer>(m_head->uin);
}












