//
// Created by toney on 19/9/20.
//

#pragma once

#include "svrlib.h"
#include "network/tcp_conn.h"
#include "helper/bufferStream.h"
#include "player.h"

using namespace Network;

class CHandlePlatMsg : public AutoDeleteSingleton<CHandlePlatMsg> {
public:
    CHandlePlatMsg();

    virtual ~CHandlePlatMsg();

    //  添加消息映射
    template<class F>
    void bind_handler(int key, F f) {
        m_handlers[key] = std::bind(f, this, std::placeholders::_1);
    }

public:
    virtual int OnRecvClientMsg(TCPConnPtr connPtr, const uint8_t *pkt_buf, uint16_t buf_len);

protected:
    /*------------------PLAT消息---------------------------------*/
    // 检测状态
    int handle_plat_ping(string msg);

    // 修改保险箱密码
    int handle_plat_change_safepwd(string msg);

    // 踢出玩家
    int handle_plat_kill_player(string msg);

    // 修改玩家数值
    int handle_plat_change_accvalue(string msg);


    // 广播消息
    int handle_plat_broadcast(string msg);


    void SendPlatMsg(string jmsg, uint16_t cmd);

protected:
    std::shared_ptr<CPlayer> GetPlayer(uint32_t uid);

protected:
    TCPConnPtr m_connPtr;
    std::unordered_map<int, function<int(string msg)>> m_handlers;

};




















