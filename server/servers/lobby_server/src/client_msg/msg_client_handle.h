
#pragma once

#include "packet/protobuf_pkg.h"
#include "crypt/md5.h"

class CPlayer;

using namespace Network;

class CHandleClientMsg : public CProtobufMsgHanlde, public AutoDeleteSingleton<CHandleClientMsg> {
public:
    CHandleClientMsg();

    virtual ~CHandleClientMsg();

    virtual int OnRecvClientMsg();

protected:
    // ת������Ϸ��
    int route_to_game_svr();

    // ������
    int handle_msg_heart();

    // ��¼
    int handle_msg_login();

    // �����������Ϣ
    int handle_msg_req_svrs_info();

    // ���������Ϸ������
    int handle_msg_enter_gamesvr();

    // �ϱ�GPS
    int handle_msg_report_gps();

    // �ϱ������ӳ�
    int handle_msg_report_net_delay();

    // ���󷿼��б���Ϣ
    int handle_msg_req_rooms_info();

    std::shared_ptr<CPlayer> GetPlayer(const TCPConnPtr &connPtr);
};
/*
 * �ͻ�����ҵ����ӹ�����Ϣ����
*/


















