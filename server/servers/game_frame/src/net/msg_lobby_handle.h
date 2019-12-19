
#pragma once

#include "packet/inner_protobuf_pkg.h"

using namespace Network;

class CGamePlayer;

class CHandleLobbyMsg : public CInnerMsgHanlde {
public:
    CHandleLobbyMsg();

    virtual ~CHandleLobbyMsg();

public:
    virtual int OnRecvClientMsg();

    // ��Ϸ����Ϣ
    virtual int handle_msg_gameing_oper();

protected:
    // ֪ͨ����״̬
    int handle_msg_notify_net_state();

    // ������Ϸ������
    int handle_msg_enter_svr();

    // ���ش���
    int handle_msg_back_lobby();

    // ���󷿼��б���Ϣ
    int handle_msg_req_rooms_info();

    // ���뷿��
    int handle_msg_enter_room();

    // ���������б�
    int handle_msg_req_table_list();

    // �뿪����
    int handle_msg_leave_table_req();

    // ��������
    int handle_msg_enter_table();

    // ����׼��
    int handle_msg_table_ready();

    // ��������
    int handle_msg_table_chat();

    // ���������й�
    int handle_msg_table_set_auto();

    // ���ٿ�ʼ
    int handle_msg_fast_join_room();

    // ���ٻ���
    int handle_msg_fast_join_table();

    // �鿴������Ϣ
    int handle_msg_query_table_list();

    // վ������
    int handle_msg_sitdown_standup();

    //ˢ�����ݱ��
    int handle_msg_flush_change_acc_data();

    std::shared_ptr<CGamePlayer> GetGamePlayer();

    void ReplyMsg(const google::protobuf::Message *msg, uint16_t msg_type, uint32_t uin);

private:


};



















