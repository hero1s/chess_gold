
#pragma once

#include <vector>
#include "svrlib.h"
#include <unordered_map>
#include <memory>
#include "packet/inner_protobuf_pkg.h"
#include "servers_msg.pb.h"
#include "server_connect/server_client.h"
#include "player.h"

using namespace std;
using namespace svrlib;
using namespace Network;

// ��Ϸ���������
class CGameServerMgr : public CServerClientMgr, public AutoDeleteSingleton<CGameServerMgr> {
public:
    CGameServerMgr();

    virtual ~CGameServerMgr();

    virtual int OnRecvClientMsg();

    virtual bool Init();

    virtual void ShutDown();

    // ���·������б�����
    void UpdateServerList2Client(std::shared_ptr<CPlayer> pPlayer);

    // �����ѡ����Ӧ��Ϸ������
    shared_ptr<CServerClient> SelectGameTypeServer(std::shared_ptr<CPlayer> pPlayer, uint16_t gameType);

private:
    // ת�����ͻ���
    int route_to_client();

    // �������ϱ���Ϣ
    int handle_msg_report();

    // ���ش���
    int handle_msg_leave_svr();

    // �޸������ֵ
    int handle_msg_notify_change_account_data();

    // �ϱ���Ϸ���
    int handle_msg_report_game_result();

    std::shared_ptr<CPlayer> GetPlayer();
};
/*
 * ��Ϸ���������Ӵ����������Ĺ�����Ϣ����
*/


































