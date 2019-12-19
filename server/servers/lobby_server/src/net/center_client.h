
#pragma once

#include "svrlib.h"
#include <string.h>
#include "packet/inner_protobuf_pkg.h"
#include <unordered_map>
#include "servers_msg.pb.h"
#include "server_connect/server_connector.h"
#include "player.h"

using namespace std;
using namespace svrlib;
using namespace Network;

class CCenterClientMgr : public CSvrConnectorMgr, public AutoDeleteSingleton<CCenterClientMgr> {
public:
    CCenterClientMgr();

    virtual ~CCenterClientMgr();

    virtual int OnRecvClientMsg();

protected:
    // ת�����ͻ���
    int route_to_client();

    // ��ҵ�¼֪ͨ
    int handle_msg_player_login_lobby();

    std::shared_ptr<CPlayer> GetPlayer();
};
/*
 * ����������-�������ķ��Ĺ�����Ϣ����
*/

