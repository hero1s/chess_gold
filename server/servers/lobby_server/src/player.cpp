
#include "player.h"
#include "helper/bufferStream.h"
#include "helper/helper.h"
#include <time.h>
#include "player_mgr.h"
#include "common_logic.h"
#include "mysql_mgr/dbmysql_mgr.h"
#include "net/game_server_mgr.h"
#include "net/center_client.h"
#include "client_logic_msg.pb.h"
#include "error_code.pb.h"

using namespace svrlib;
using namespace std;
using namespace Network;

namespace {
    static const uint32_t s_OfflineTime = MINUTE * 10; // 离线10分钟下线
};

CPlayer::CPlayer(PLAYER_TYPE type)
        : CPlayerBase(type) {
    m_disconnectTime = 0;
    m_reloginTime = 0;
    m_netDelay = 0;
    m_limitTime.fill(0);
}

CPlayer::~CPlayer() {
    if (m_pSession)
    {
        m_pSession->SetUID(0);
        m_pSession->Close();
    }
}

void CPlayer::OnLoginOut() {
    LOG_DEBUG("player login out:{}", GetUID());
    if (IsPlaying())
    {
        // 保存数据
        m_missionMgr.SaveMiss();
        SavePlayerBaseInfo();
        SaveLoginInfo();
    }
    SetPlayerState(PLAYER_STATE_LOGINOUT);
    if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
    {
        NotifyLoginOut(0);
    }

    SOL_CALL_LUA(CApplication::Instance().GetSolLuaState()["login_out"](this));
}

void CPlayer::OnLogin() {
    LOG_DEBUG("OnLogin:{}", GetUID());
    net::cli::msg_login_rep repmsg;
    repmsg.set_result(net::RESULT_CODE_SUCCESS);
    repmsg.set_server_time(time::getSysTime());
    SendMsgToClient(&repmsg, net::S2C_MSG_LOGIN_REP);
    uint32_t uid = GetUID();
    m_missionMgr.AttachPlayer(shared_from_this());
    //拉取数据
    CDBMysqlMgr::Instance().AsyncLoadPlayerData(uid, emACCDATA_TYPE_BASE, [uid](shared_ptr<CDBEventRep> &pRep)
    {
        LOG_DEBUG("OnLoadPlayerData:{}", uid);
        auto pPlayer = CPlayerMgr::Instance().GetPlayer<CPlayer>(uid);
        if (pPlayer == nullptr || pPlayer->GetPlayerState() != PLAYER_STATE_LOAD_DATA)
        {
            LOG_DEBUG("load mission player is not find:{}", uid);
            return;
        }
        stBaseInfo data;
        if (pRep->vecData.size() > 0)
        {
            auto &refRows = pRep->vecData[0];

            data.name = refRows["nickname"].as<string>();
            data.sex = refRows["sex"];
            data.offline_time = refRows["offlinetime"];
            data.clogin = refRows["clogin"];
            data.weeklogin = refRows["weeklogin"];
            data.all_login_days = refRows["alllogin"];
            data.login_ip = CHelper::IPToValue(refRows["loginip"].as<string>());
            data.coin = refRows["coin"];
            data.vip = refRows["vip"];
            data.safecoin = refRows["safecoin"];
            data.lon = refRows["lon"];
            data.lat = refRows["lat"];

            pPlayer->SetBaseInfo(data);
            if (pPlayer->IsLoadOver())
            {
                pPlayer->OnGetAllData();
            }
        }
        else
        {
            LOG_ERROR("the base data is can't load:{}", uid);
        }

    });
    CDBMysqlMgr::Instance().AsyncLoadMissionData(uid, [uid](shared_ptr<CDBEventRep> &pRep)
    {
        LOG_DEBUG("OnLoadMissionData");
        auto pPlayer = CPlayerMgr::Instance().GetPlayer<CPlayer>(uid);
        if (pPlayer == nullptr || pPlayer->GetPlayerState() != PLAYER_STATE_LOAD_DATA)
        {
            LOG_DEBUG("load mission player is not find:{}", uid);
            return;
        }
        map<uint32_t, stUserMission> mission;
        if (pRep->vecData.size() > 0)
        {
            for (uint32_t i = 0; i < pRep->vecData.size(); ++i)
            {
                auto &refRows = pRep->vecData[i];
                stUserMission data;
                data.ctimes = refRows["ctimes"];
                data.msid = refRows["msid"];
                data.ptime = refRows["ptime"];
                data.rtimes = refRows["rtimes"];
                data.cptime = refRows["cptime"];
                data.update = 0;
                mission[data.msid] = data;
            }
        }
        pPlayer->GetMissionMgr().SetMission(mission);
        if (pPlayer->IsLoadOver())
        {
            pPlayer->OnGetAllData();
        }
    });

    SetPlayerState(PLAYER_STATE_LOAD_DATA);

    m_loadTime = time::getSysTime();
    m_reloginTime = time::getSysTime();

}

void CPlayer::OnGetAllData() {
    LOG_DEBUG("all data loaded over :{}", GetUID());
    SetPlayerState(PLAYER_STATE_PLAYING);
    BuildInit();

    // 同步游戏服ID
    SyncGameSvrID();

    // 发送数据到客户端
    SendAllPlayerData2Client();
    NotifyEnterGame();
    NotifyLobbyLogin();

    SOL_CALL_LUA(CApplication::Instance().GetSolLuaState()["login_on"](this));

    SavePlayerBaseInfo();//test toney
    EnterGameSvr(1);//test toney,1斗地主
}

void CPlayer::ReLogin() {
    LOG_DEBUG("player relogin:{}", GetUID());
    net::cli::msg_login_rep repmsg;
    repmsg.set_result(net::RESULT_CODE_SUCCESS);
    repmsg.set_server_time(time::getSysTime());
    SendMsgToClient(&repmsg, net::S2C_MSG_LOGIN_REP);

    SendAllPlayerData2Client();
    NotifyEnterGame();
    NotifyNetState2GameSvr(1);

    m_reloginTime = time::getSysTime();
}

void CPlayer::OnTimeTick(uint64_t uTime, bool bNewDay) {
    AutoProfile("CPlayer::OnTimeTick");
    if (!IsPlaying())
    {
        return;
    }
    if (CCommonLogic::IsNeedReset(m_baseInfo.offline_time, uTime))
    {
        DailyCleanup(1);
        SaveLoginInfo();
    }
    // 更新离线时间
    m_baseInfo.offline_time = uTime;
    // 新的一天
    if (bNewDay)
    {
        tm local_time;
        time::localtime(uTime, &local_time);

        // 跨周0星期天，1星期1
        if (local_time.tm_wday == 0)
            WeeklyCleanup();
        // 跨月
        if (local_time.tm_mday == 1)
            MonthlyCleanup();
        SaveLoginInfo();
        SendAllPlayerData2Client();
    }
    if (m_pSession == nullptr)
    {
        if (m_disconnectTime == 0)
        {
            m_disconnectTime = time::getSysTime();
        }
    }
    else
    {
        m_disconnectTime = 0;
    }
}

// 是否需要回收
bool CPlayer::NeedRecover() {
    AutoProfile("CPlayer::NeedRecover");
    //服务器维护状态
    if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
    {
        if (IsInLobby())
        {
            LOG_ERROR("server is repair status,not playing killed loginout");
            return true;
        }
    }
    if (m_needRecover)
    {
        return true;
    }
    if (GetPlayerState() == PLAYER_STATE_LOAD_DATA && (time::getSysTime() - m_loadTime) > MINUTE)
    {
        LOG_ERROR("load player data time out :{}", GetUID());
        return true;
    }

    if (m_pSession == nullptr)
    {
        if ((time::getSysTime() - m_disconnectTime) > s_OfflineTime)// 不在游戏中，或者超时下线
        {
            LOG_DEBUG("not playing,time out loginout {} time {}", GetUID(), s_OfflineTime);
            return true;
        }
    }

    if (!IsInLobby() || GetPlayerState() == PLAYER_STATE_LOAD_DATA)
        return false;

    return false;
}

// 返回大厅回调
void CPlayer::BackLobby() {
    SetGameSvrID(0, true);
}

bool CPlayer::CanModifyData() {
    if (m_bPlayerState >= PLAYER_STATE_PLAYING)
        return true;
    return false;
}

//--- 每日清理
void CPlayer::DailyCleanup(int32_t iOfflineDay) {
    LOG_DEBUG("daily cleanup:{}", GetUID());
    m_missionMgr.ResetMission(net::MISSION_CYCLE_TYPE_DAY);
    if (iOfflineDay > 1)
    {
        m_baseInfo.clogin = 1;
    }
    else
    {
        m_baseInfo.clogin++;
    }

    m_baseInfo.weeklogin++;

    SOL_CALL_LUA(CApplication::Instance().GetSolLuaState()["new_day"](this));
}

//--- 每周清理
void CPlayer::WeeklyCleanup() {
    m_missionMgr.ResetMission(net::MISSION_CYCLE_TYPE_WEEK);
    //UnsetRewardBitFlag(net::REWARD_WLOGIN3);
    //UnsetRewardBitFlag(net::REWARD_WLOGIN5);
    //UnsetRewardBitFlag(net::REWARD_WLOGIN6);
    m_baseInfo.weeklogin = 1;

    SOL_CALL_LUA(CApplication::Instance().GetSolLuaState()["new_week"](this));
}

//--- 每月清理
void CPlayer::MonthlyCleanup() {
    m_missionMgr.ResetMission(net::MISSION_CYCLE_TYPE_MONTH);

    SOL_CALL_LUA(CApplication::Instance().GetSolLuaState()["new_month"](this));
}

void CPlayer::NotifyEnterGame() {
    net::cli::msg_enter_game_rep msg;
    msg.set_result(0);

    SendMsgToClient(&msg, net::S2C_MSG_ENTER_GAME);
    LOG_DEBUG("notify enter game:{}", GetUID());
}

void CPlayer::NotifyLoginOut(uint32_t code, string deviceid) {
    LOG_DEBUG("notify leave game :{}--{}", GetUID(), code);
    net::cli::msg_loginout_rep loginoutmsg;
    loginoutmsg.set_reason(code);
    loginoutmsg.set_deviceid(deviceid);
    SendMsgToClient(&loginoutmsg, net::S2C_MSG_LOGINOUT_REP);
}

bool CPlayer::SendAllPlayerData2Client() {
    SendAccData2Client();

    m_missionMgr.SendMissionData2Client();
    return true;
}

bool CPlayer::SendAccData2Client() {
    LOG_DEBUG("send player account data:{}", GetUID());

    net::cli::msg_player_data_rep msg;
    GetPlayerBaseData(msg.mutable_base_data());
    SendMsgToClient(&msg, net::S2C_MSG_PLAYER_INFO);

    return true;
}

bool CPlayer::UpdateAccValue2Client() {
    LOG_DEBUG("update acc value to client:coin:{}", GetAccountValue(emACC_VALUE_COIN));
    net::cli::msg_update_acc_value msg;
    msg.set_coin(GetAccountValue(emACC_VALUE_COIN));
    msg.set_safe_coin(GetAccountValue(emACC_VALUE_SAFECOIN));

    SendMsgToClient(&msg, net::S2C_MSG_UPDATE_ACC_VALUE);
    return true;
}

// 通知返回大厅
void CPlayer::NotifyClientBackLobby(uint8_t result, uint8_t reason) {
    net::cli::msg_back_lobby_rep rep;
    rep.set_result(result);
    rep.set_reason(reason);
    SendMsgToClient(&rep, net::S2C_MSG_BACK_LOBBY_REP);
}

// 广播通知登录
void CPlayer::NotifyLobbyLogin() {
    net::svr::msg_notify_player_lobby_login msg;
    msg.set_lobby_id(CApplication::Instance().GetServerID());
    msg.set_uid(GetUID());
    //广播全部大厅
    CCenterClientMgr::Instance().SendMsg2Svr(&msg, net::svr::GS2L_MSG_NOTIFY_PLAYER_LOBBY_LOGIN, GetUID(), emROUTE_TYPE_ALL_SERVER, emSERVER_TYPE_LOBBY);
}

// 构建初始化
void CPlayer::BuildInit() {
    AutoProfile("CPlayer::BuildInit");
    LOG_DEBUG("player build init :{}", GetUID());

    // 检测日常
    uint32_t uBuildTime = time::getSysTime();
    uint32_t offlinetime = m_baseInfo.offline_time;
    uint32_t uOfflineSecond = ((offlinetime && uBuildTime > offlinetime) ? (uBuildTime - offlinetime) : 0);
    //跨天检查并清理
    int32_t iOfflineDay = time::diffTimeDay(offlinetime, uBuildTime);
    if (offlinetime == 0 && iOfflineDay <= 0)//新上线绝对是第一天
    {
        DailyCleanup(2);
        iOfflineDay = 0;
    }
    //跨周检查并清理
    int32_t iOfflineWeek = time::diffTimeWeek(offlinetime, uBuildTime);
    if (offlinetime == 0 && iOfflineWeek <= 0)
    {
        WeeklyCleanup();
        iOfflineWeek = 0;
    }
    //跨月检查并清理
    int32_t iOfflineMonth = time::diffTimeMonth(offlinetime, uBuildTime);
    if (offlinetime == 0 && iOfflineMonth <= 0)
    {
        MonthlyCleanup();
        iOfflineMonth = 0;
    }
    if (offlinetime != 0 && CCommonLogic::IsNeedReset(offlinetime, uBuildTime))
    {
        DailyCleanup(iOfflineDay);
    }
    if (iOfflineWeek > 0)
    {
        WeeklyCleanup();
    }
    if (iOfflineMonth > 0)
    {
        MonthlyCleanup();
    }
    SetOfflineTime(uBuildTime);
    if (m_baseInfo.clogin < 1)
        m_baseInfo.clogin = 1;

    SaveLoginInfo();
}

// 是否在大厅中
bool CPlayer::IsInLobby() {
    if (m_curSvrID != 0)
    {
        return false;
    }
    return true;
}

bool CPlayer::SendMsgToGameSvr(const google::protobuf::Message *msg, uint16_t msg_type) {
    if (m_curSvrID == 0)
        return false;
    CGameServerMgr::Instance().SendMsg2Server(m_curSvrID, msg, msg_type, GetUID());
    return true;
}

bool CPlayer::SendMsgToGameSvr(const void *msg, uint16_t msg_len, uint16_t msg_type) {
    if (m_curSvrID == 0)
        return false;
    CGameServerMgr::Instance().SendMsg2Server(m_curSvrID, (uint8_t *) msg, msg_len, msg_type, GetUID());
    return true;
}

// 通知网络状态
void CPlayer::NotifyNetState2GameSvr(uint8_t state) {
    net::svr::msg_notify_net_state msg;
    msg.set_uid(GetUID());
    msg.set_state(state);
    msg.set_newip(m_baseInfo.login_ip);
    msg.set_no_player(0);
    SendMsgToGameSvr(&msg, net::svr::L2GS_MSG_NOTIFY_NET_STATE);
}

// 请求返回大厅
void CPlayer::ActionReqBackLobby(uint8_t action) {
    net::cli::msg_back_lobby_req msg;
    msg.set_uid(GetUID());
    msg.set_is_action(action);
    SendMsgToGameSvr(&msg, net::C2S_MSG_BACK_LOBBY);
}

// 进入游戏服务器
uint16_t CPlayer::EnterGameSvr(uint16_t gameType) {
    AutoProfile("CPlayer::EnterGameSvr");
    auto curID = GetGameSvrID();
    if (curID != 0)
    {//重连
        auto pServer = CGameServerMgr::Instance().GetServerBySvrID(curID);
        if (pServer == nullptr)
        {
            LOG_DEBUG("重连服务器不存在:{}", curID);
            SetGameSvrID(0, true);
            CGameServerMgr::Instance().UpdateServerList2Client(shared_from_this());
            goto REENTER;
        }

        // 发送游戏数据到游戏服
        net::svr::msg_enter_into_game_svr msg;
        msg.set_player_type(GetPlayerType());
        msg.set_play_type(0);
        GetPlayerGameData(&msg);
        SendMsgToGameSvr(&msg, net::svr::L2GS_MSG_ENTER_INTO_SVR);
        LOG_DEBUG("重连进入游戏服务器:{}-->{}", GetUID(), curID);
        return RESULT_CODE_SUCCESS;
    }
    else
    {
        goto REENTER;
    }
    REENTER:
    auto pServer = CGameServerMgr::Instance().SelectGameTypeServer(shared_from_this(), gameType);
    if (pServer == nullptr)
    {
        LOG_DEBUG("维护状态,非在玩玩家不能进入:uid {}--cursvrID:{}--gameType:{}", GetUID(), GetGameSvrID(), gameType);
        return RESULT_CODE_SVR_REPAIR;
    }
    SetGameSvrID(pServer->GetSvrID(), false);
    // 发送游戏数据到游戏服
    net::svr::msg_enter_into_game_svr msg;
    msg.set_player_type(GetPlayerType());
    msg.set_play_type(0);
    GetPlayerGameData(&msg);
    SendMsgToGameSvr(&msg, net::svr::L2GS_MSG_ENTER_INTO_SVR);
    LOG_DEBUG("进入游戏服务器:{}-->{}", GetUID(), pServer->GetSvrID());
    return RESULT_CODE_SUCCESS;
}

// 刷新修改数值到游戏服
void CPlayer::FlushChangeAccData2GameSvr(int64_t coin, int64_t safecoin) {
    if (GetGameSvrID() == 0)
        return;

    net::svr::msg_flush_change_account_data msg;
    msg.set_uid(GetUID());
    msg.set_coin(coin);
    msg.set_safe_coin(safecoin);
    msg.set_lat(GetLat());
    msg.set_lon(GetLon());

    SendMsgToGameSvr(&msg, net::svr::L2GS_MSG_FLUSH_CHANGE_ACC_DATA);
}

// 修改玩家账号数值（增量修改）
void CPlayer::SyncChangeAccountValue(uint16_t operType, uint16_t subType, int64_t coin, int64_t safecoin, const string &chessid) {
    coin = ChangeAccountValue(emACC_VALUE_COIN, coin);
    if (coin != 0)
    {
        CDBMysqlMgr::Instance().AccountTransction(GetUID(), emACC_VALUE_COIN, operType, subType, coin,
                                                  GetAccountValue(emACC_VALUE_COIN) - coin,
                                                  GetAccountValue(emACC_VALUE_COIN), chessid);
    }
    safecoin = ChangeAccountValue(emACC_VALUE_SAFECOIN, safecoin);
    if (safecoin != 0)
    {
        CDBMysqlMgr::Instance().AccountTransction(GetUID(), emACC_VALUE_SAFECOIN, operType, subType, safecoin,
                                                  GetAccountValue(emACC_VALUE_SAFECOIN) - safecoin,
                                                  GetAccountValue(emACC_VALUE_SAFECOIN), chessid);
    }
    CDBMysqlMgr::Instance().ChangeAccountValue(GetUID(), coin, safecoin);
}

// 保存登陆奖励状态
void CPlayer::SaveLoginInfo() {
    AutoProfile("CPlayer::SaveLoginInfo");
    LOG_DEBUG("保存登陆login信息:{}", GetUID());
    CDBMysqlMgr::Instance().UpdatePlayerLoginInfo(GetUID(), m_baseInfo.offline_time, m_baseInfo.clogin, m_baseInfo.weeklogin, 0);
}

CMissionMgr &CPlayer::GetMissionMgr() {
    return m_missionMgr;
}

// 获得relogin时间
uint32_t CPlayer::GetReloginTime() {
    return m_reloginTime;
}

// 网络延迟
uint32_t CPlayer::GetNetDelay() {
    return m_netDelay;
}

bool CPlayer::SetNetDelay(uint32_t netDelay) {
    m_netDelay = (m_netDelay + netDelay) / 2;
    if ((time::getSysTime() - m_limitTime[emLIMIT_TIME_NETDELAY]) > MINUTE)
    {
        m_limitTime[emLIMIT_TIME_NETDELAY] = time::getSysTime();
        return true;
    }
    return false;
}

// Flush GPS
void CPlayer::FlushGPS(double lon, double lat) {
    SetLon(lon);
    SetLat(lat);
    if ((time::getSysTime() - m_limitTime[emLIMIT_TIME_GPS]) > TimeConstants::MINUTE)
    {
        CDBMysqlMgr::Instance().UpdatePlayerGPS(GetUID(), GetLon(), GetLat());
        m_limitTime[emLIMIT_TIME_GPS] = time::getSysTime();
    }
}

// 保存数据
void CPlayer::SavePlayerBaseInfo() {

    net::base_info baseInfo;
    GetPlayerBaseData(&baseInfo);

    DUMP_PROTO_MSG_INFO(baseInfo);

    string baseData;
    baseInfo.SerializeToString(&baseData);

    LOG_DEBUG("save player data uid:{},datalen:{},{}", GetUID(), baseData.length(), m_baseInfo.offline_time);

}








