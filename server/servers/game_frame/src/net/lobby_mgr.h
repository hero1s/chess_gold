
#pragma once

#include "utility/noncopyable.hpp"
#include "svrlib.h"
#include <string.h>
#include "servers_msg.pb.h"
#include "server_connect/server_connector.h"
#include "packet/inner_protobuf_pkg.h"
#include <unordered_map>

using namespace std;
using namespace svrlib;
using namespace Network;

//��������
class CLobbyClient : public CSvrConnectorMgr {
public:
    CLobbyClient();

    virtual ~CLobbyClient();

};

/**
 * �������������
 */
class CLobbyMgr : public AutoDeleteSingleton<CLobbyMgr> {
public:
    CLobbyMgr();

    void OnTimer();

    bool Init();

    // ���Ӵ�����
    bool ConnectLobby(string ip, int32_t port, uint16_t sid);

    // ������Ϣ���ͻ���
    bool SendMsg2Client(const google::protobuf::Message *msg, uint16_t msg_type, uint32_t uin, uint16_t svrid);

    // ������Ϣ������
    bool SendMsg2Lobby(const google::protobuf::Message *msg, uint16_t msg_type, uint16_t svrid);

    // ������Ϣ��ȫ������
    bool SendMsg2AllLobby(const google::protobuf::Message *msg, uint16_t msg_type);

    // ֪ͨ�����޸���ֵ
    void NotifyLobbyChangeAccValue(uint32_t uid, int32_t operType, int32_t subType, int64_t coin, int64_t safeCoin, const string& chessid = "");
private:
    std::shared_ptr<CLobbyClient> GetLobbySvrBySvrID(uint16_t svrid);

    void ReportInfo2Lobby();

private:
    using MAP_LOBBY = unordered_map<uint32_t, std::shared_ptr<CLobbyClient>>;
    MAP_LOBBY m_lobbySvrs;// ����������
    MemberTimerEvent<CLobbyMgr, &CLobbyMgr::OnTimer> m_timer;

};










