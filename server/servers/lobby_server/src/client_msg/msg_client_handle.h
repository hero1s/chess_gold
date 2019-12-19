
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
    // 转发给游戏服
    int route_to_game_svr();

    // 心跳包
    int handle_msg_heart();

    // 登录
    int handle_msg_login();

    // 请求服务器信息
    int handle_msg_req_svrs_info();

    // 请求进入游戏服务器
    int handle_msg_enter_gamesvr();

    // 上报GPS
    int handle_msg_report_gps();

    // 上报网络延迟
    int handle_msg_report_net_delay();

    // 请求房间列表信息
    int handle_msg_req_rooms_info();

    std::shared_ptr<CPlayer> GetPlayer(const TCPConnPtr &connPtr);
};
/*
 * 客户端玩家的连接管理及消息处理
*/


















