
#include "msg_plat_handle.h"
#include "player_mgr.h"
#include "msg_define.pb.h"
#include "nlohmann/json_wrap.h"
#include "crypt/aes.h"
#include "common_logic.h"

using namespace Network;
using namespace svrlib;

CHandlePlatMsg::CHandlePlatMsg() {
    bind_handler(net::PLAT_MSG_PING, &CHandlePlatMsg::handle_plat_ping);
    bind_handler(net::PLAT_MSG_CHANGE_SAFEPWD, &CHandlePlatMsg::handle_plat_change_safepwd);
    bind_handler(net::PLAT_MSG_KILL_PLAYER, &CHandlePlatMsg::handle_plat_kill_player);
    bind_handler(net::PLAT_MSG_CHANGE_ACCVALUE, &CHandlePlatMsg::handle_plat_change_accvalue);
    bind_handler(net::PLAT_MSG_BROADCAST, &CHandlePlatMsg::handle_plat_broadcast);
}

CHandlePlatMsg::~CHandlePlatMsg() {

}

int CHandlePlatMsg::OnRecvClientMsg(TCPConnPtr connPtr, const uint8_t *pkt_buf, uint16_t buf_len) {
    m_connPtr = connPtr;
    uint16_t cmd = 0;
    CBufferStream stream((char *) pkt_buf, buf_len);
    stream.read_(cmd);
    static uint8_t pOutBuf[PACKET_MAX_SIZE];
    memset(pOutBuf, 0, sizeof(pOutBuf));
    string msg;
    stream.read(stream.getSpareSize(), pOutBuf);
    msg = CStringUtility::FormatToString("%s", pOutBuf);

    LOG_DEBUG("收到平台消息:{}--{}", cmd, msg);
    auto it = m_handlers.find(cmd);
    if (it != m_handlers.end())
    {
        it->second(msg);
    }
    else
    {
        LOG_ERROR("未处理的平台消息:%d", cmd);
    }
    return 0;
}

/*------------------PLAT消息---------------------------------*/
// 检测状态
int CHandlePlatMsg::handle_plat_ping(string msg) {
    json jvalue;
    json jrep;
    if (!ParseJsonFromString(jvalue, msg))
    {
        LOG_ERROR("解析json错误");
        jrep["ret"] = 0;
        SendPlatMsg(jrep.dump(), net::PLAT_MSG_PING);
        return 0;
    }

    jrep["ret"] = 1;
    SendPlatMsg(jrep.dump(), net::PLAT_MSG_PING);
    return 0;
}

// PHP修改保险箱密码
int CHandlePlatMsg::handle_plat_change_safepwd(string msg) {
    json jvalue;
    json rep;
    if (!ParseJsonFromString(jvalue, msg))
    {
        LOG_ERROR("解析json错误:{}", msg);
        rep["ret"] = 0;
        SendPlatMsg(rep.dump(), net::PLAT_MSG_CHANGE_SAFEPWD);
        return 0;
    }
    try
    {
        uint32_t uid = jvalue.value("uid", 0);
        string passwd = jvalue.value("pwd", "");
        LOG_DEBUG("php修改保险箱密码:{}-{}", uid, passwd);
        auto pPlayer = GetPlayer(uid);
        if (pPlayer != nullptr)
        {
            //pPlayer->SetSafePasswd(passwd);
        }
    }
    catch (json::exception &e)
    {
        LOG_ERROR("json error:{}", e.what());
    }
    rep["ret"] = 1;
    SendPlatMsg(rep.dump(), net::PLAT_MSG_CHANGE_SAFEPWD);
    return 0;
}

// PLAT踢出玩家
int CHandlePlatMsg::handle_plat_kill_player(string msg) {
    json jvalue;
    json jrep;
    if (!ParseJsonFromString(jvalue, msg))
    {
        LOG_ERROR("解析json错误");
        jrep["ret"] = 0;
        SendPlatMsg(jrep.dump(), net::PLAT_MSG_KILL_PLAYER);
        return 0;
    }
    try
    {
        uint32_t uid = jvalue.value("uid", 0);
        LOG_DEBUG("php踢出玩家:{}", uid);
        auto pPlayer = GetPlayer(uid);
        if (pPlayer != nullptr)
        {
            auto pSession = pPlayer->GetSession();
            if (pSession)
            {
                pSession->Close();
            }
            pPlayer->SetNeedRecover(true);
        }
    }
    catch (json::exception &e)
    {
        LOG_ERROR("json error:{}", e.what());
    }
    jrep["ret"] = 1;
    SendPlatMsg(jrep.dump(), net::PLAT_MSG_KILL_PLAYER);
    return 0;
}

// PLAT修改玩家数值
int CHandlePlatMsg::handle_plat_change_accvalue(string msg) {
    json jvalue;
    json jrep;
    if (!ParseJsonFromString(jvalue, msg))
    {
        LOG_ERROR("解析json错误");
        jrep["ret"] = 0;
        SendPlatMsg(jrep.dump(), net::PLAT_MSG_CHANGE_ACCVALUE);
        return 0;
    }
    if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
    {
        jrep["ret"] = 0;
        SendPlatMsg(jrep.dump(), net::PLAT_MSG_CHANGE_ACCVALUE);
        return 0;
    }

    bool bRet = false;
    int code = 0;
    try
    {
        uint32_t uid = jvalue.value("uid", 0);
        uint32_t oper_type = jvalue.value("ptype", 0);
        uint32_t sub_type = jvalue.value("sptype", 0);
        int64_t coin = jvalue.value("coin", 0);
        int64_t safecoin = jvalue.value("safecoin", 0);

        LOG_DEBUG("plat修改玩家数值:uid {}- coin {},safecoin:{}", uid, coin, safecoin);
        auto pPlayer = GetPlayer(uid);
        //处理金币
        if (coin != 0 || safecoin != 0)
        {
            if (pPlayer != nullptr)
            {
                bool needInLobby = true;
                if (coin >= 0 && safecoin >= 0)
                {
                    needInLobby = false;
                }
                if (pPlayer->IsInLobby() || !needInLobby)
                {
                    bRet = pPlayer->AtomChangeAccountValue(oper_type, sub_type, coin, safecoin);
                    if (bRet)
                    {
                        jrep["coin"] = pPlayer->GetAccountValue(emACC_VALUE_COIN);
                        jrep["safecoin"] = pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN);
                        pPlayer->UpdateAccValue2Client();
                        pPlayer->FlushChangeAccData2GameSvr(coin, safecoin);
                    }
                    else
                    {
                        code = 1;
                        LOG_DEBUG("在线操作金币失败uid {} coin {} safecoin {}", uid, coin, safecoin);
                    }
                }
                else
                {
                    code = 2;
                    LOG_DEBUG("不在大厅，不能减少金币 {}", uid);
                }
            }
            else
            {
                if (coin != 0 || safecoin != 0)
                {
                    bRet = CCommonLogic::AtomChangeOfflineAccData(uid, oper_type, sub_type, coin, safecoin);
                }
                if (!bRet)
                {
                    code = 1;
                    LOG_DEBUG("离线操作金币失败uid {} coin {} safecoin {}", uid, coin, safecoin);
                }
            }
        }
    }
    catch (json::exception &e)
    {
        LOG_ERROR("json error:{}", e.what());
    }

    EXIT:
    jrep["ret"] = bRet ? 1 : 0;
    jrep["code"] = code;

    SendPlatMsg(jrep.dump(), net::PLAT_MSG_CHANGE_ACCVALUE);
    return 0;
}

// PLAT广播消息
int CHandlePlatMsg::handle_plat_broadcast(string msg) {
    json jvalue;
    json jrep;
    if (!ParseJsonFromString(jvalue, msg))
    {
        LOG_ERROR("解析json错误");
        jrep["ret"] = 0;
        SendPlatMsg(jrep.dump(), net::PLAT_MSG_BROADCAST);
        return 0;
    }

    try
    {
        vector<uint32_t> uids;
        for (uint32_t i = 0; i < jvalue["uid"].size(); ++i)
        {
            uint32_t uid = jvalue["uid"][i];
            uids.push_back(uid);
        }
        int64_t cid = jvalue.value("cid", 0);
        string broadCast = jvalue.value("msg", "");
        LOG_DEBUG("plat广播消息:cid {}--{}", cid, broadCast);
//        net::msg_php_broadcast_rep rep;
//        rep.set_msg(broadCast);
//        rep.set_cid(cid);
//        rep.set_uid(0);

        if (uids.size() == 0)
        {
            if (cid != 0)
            {
                //CPlayerMgr::Instance().SendMsgToClub(&rep, net::S2C_MSG_PHP_BROADCAST, cid);
            }
            else
            {
                //CPlayerMgr::Instance().SendMsgToAll(&rep, net::S2C_MSG_PHP_BROADCAST);
            }
        }
        else
        {
            for (auto uid:uids)
            {
                //rep.set_uid(uid);
                //CPlayerMgr::Instance().SendMsgToPlayer(&rep, net::S2C_MSG_PHP_BROADCAST, uid);
            }
        }
    }
    catch (json::exception &e)
    {
        LOG_ERROR("json error:{}", e.what());
    }
    jrep["ret"] = 1;
    SendPlatMsg(jrep.dump(), net::PLAT_MSG_BROADCAST);
    return 0;
}

void CHandlePlatMsg::SendPlatMsg(string jmsg, uint16_t cmd) {

    CBufferStream &sendStream = sendStream.buildStream();
    sendStream.write_(cmd);
    sendStream.write(jmsg.length(), jmsg.c_str());

    bool bRet = m_connPtr->Send(sendStream.getBuffer(), sendStream.getPosition());
    LOG_DEBUG("回复php消息:{}--{}--{}-{}", cmd, sendStream.getPosition(), bRet, jmsg);
}

std::shared_ptr<CPlayer> CHandlePlatMsg::GetPlayer(uint32_t uid) {
    auto pPlayer = CPlayerMgr::Instance().GetPlayer<CPlayer>(uid);
    if (pPlayer == nullptr)
    {
        LOG_DEBUG("玩家不存在，或者玩家不在在线状态:{}", uid);
        return nullptr;
    }
    return pPlayer;
}









