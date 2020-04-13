
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
    static const uint32_t s_OfflineTime = MINUTE * 10; // ����10��������
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
        // ��������
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
    //��ȡ����
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

    // ͬ����Ϸ��ID
    SyncGameSvrID();

    // �������ݵ��ͻ���
    SendAllPlayerData2Client();
    NotifyEnterGame();
    NotifyLobbyLogin();

    SOL_CALL_LUA(CApplication::Instance().GetSolLuaState()["login_on"](this));

    SavePlayerBaseInfo();//test toney
    EnterGameSvr(1);//test toney,1������
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
    // ��������ʱ��
    m_baseInfo.offline_time = uTime;
    // �µ�һ��
    if (bNewDay)
    {
        tm local_time;
        time::localtime(uTime, &local_time);

        // ����0�����죬1����1
        if (local_time.tm_wday == 0)
            WeeklyCleanup();
        // ����
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

// �Ƿ���Ҫ����
bool CPlayer::NeedRecover() {
    AutoProfile("CPlayer::NeedRecover");
    //������ά��״̬
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
        if ((time::getSysTime() - m_disconnectTime) > s_OfflineTime)// ������Ϸ�У����߳�ʱ����
        {
            LOG_DEBUG("not playing,time out loginout {} time {}", GetUID(), s_OfflineTime);
            return true;
        }
    }

    if (!IsInLobby() || GetPlayerState() == PLAYER_STATE_LOAD_DATA)
        return false;

    return false;
}

// ���ش����ص�
void CPlayer::BackLobby() {
    SetGameSvrID(0, true);
}

bool CPlayer::CanModifyData() {
    if (m_bPlayerState >= PLAYER_STATE_PLAYING)
        return true;
    return false;
}

//--- ÿ������
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

//--- ÿ������
void CPlayer::WeeklyCleanup() {
    m_missionMgr.ResetMission(net::MISSION_CYCLE_TYPE_WEEK);
    //UnsetRewardBitFlag(net::REWARD_WLOGIN3);
    //UnsetRewardBitFlag(net::REWARD_WLOGIN5);
    //UnsetRewardBitFlag(net::REWARD_WLOGIN6);
    m_baseInfo.weeklogin = 1;

    SOL_CALL_LUA(CApplication::Instance().GetSolLuaState()["new_week"](this));
}

//--- ÿ������
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

// ֪ͨ���ش���
void CPlayer::NotifyClientBackLobby(uint8_t result, uint8_t reason) {
    net::cli::msg_back_lobby_rep rep;
    rep.set_result(result);
    rep.set_reason(reason);
    SendMsgToClient(&rep, net::S2C_MSG_BACK_LOBBY_REP);
}

// �㲥֪ͨ��¼
void CPlayer::NotifyLobbyLogin() {
    net::svr::msg_notify_player_lobby_login msg;
    msg.set_lobby_id(CApplication::Instance().GetServerID());
    msg.set_uid(GetUID());
    //�㲥ȫ������
    CCenterClientMgr::Instance().SendMsg2Svr(&msg, net::svr::GS2L_MSG_NOTIFY_PLAYER_LOBBY_LOGIN, GetUID(), emROUTE_TYPE_ALL_SERVER, emSERVER_TYPE_LOBBY);
}

// ������ʼ��
void CPlayer::BuildInit() {
    LOG_DEBUG("player build init :{}", GetUID());

    // ����ճ�
    uint32_t uBuildTime = time::getSysTime();
    uint32_t offlinetime = m_baseInfo.offline_time;
    uint32_t uOfflineSecond = ((offlinetime && uBuildTime > offlinetime) ? (uBuildTime - offlinetime) : 0);
    //�����鲢����
    int32_t iOfflineDay = time::diffTimeDay(offlinetime, uBuildTime);
    if (offlinetime == 0 && iOfflineDay <= 0)//�����߾����ǵ�һ��
    {
        DailyCleanup(2);
        iOfflineDay = 0;
    }
    //���ܼ�鲢����
    int32_t iOfflineWeek = time::diffTimeWeek(offlinetime, uBuildTime);
    if (offlinetime == 0 && iOfflineWeek <= 0)
    {
        WeeklyCleanup();
        iOfflineWeek = 0;
    }
    //���¼�鲢����
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

// �Ƿ��ڴ�����
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

// ֪ͨ����״̬
void CPlayer::NotifyNetState2GameSvr(uint8_t state) {
    net::svr::msg_notify_net_state msg;
    msg.set_uid(GetUID());
    msg.set_state(state);
    msg.set_newip(m_baseInfo.login_ip);
    msg.set_no_player(0);
    SendMsgToGameSvr(&msg, net::svr::L2GS_MSG_NOTIFY_NET_STATE);
}

// ���󷵻ش���
void CPlayer::ActionReqBackLobby(uint8_t action) {
    net::cli::msg_back_lobby_req msg;
    msg.set_uid(GetUID());
    msg.set_is_action(action);
    SendMsgToGameSvr(&msg, net::C2S_MSG_BACK_LOBBY);
}

// ������Ϸ������
uint16_t CPlayer::EnterGameSvr(uint16_t gameType) {
    auto curID = GetGameSvrID();
    if (curID != 0)
    {//����
        auto pServer = CGameServerMgr::Instance().GetServerBySvrID(curID);
        if (pServer == nullptr)
        {
            LOG_DEBUG("����������������:{}", curID);
            SetGameSvrID(0, true);
            CGameServerMgr::Instance().UpdateServerList2Client(shared_from_this());
            goto REENTER;
        }

        // ������Ϸ���ݵ���Ϸ��
        net::svr::msg_enter_into_game_svr msg;
        msg.set_player_type(GetPlayerType());
        msg.set_play_type(0);
        GetPlayerGameData(&msg);
        SendMsgToGameSvr(&msg, net::svr::L2GS_MSG_ENTER_INTO_SVR);
        LOG_DEBUG("����������Ϸ������:{}-->{}", GetUID(), curID);
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
        LOG_DEBUG("ά��״̬,��������Ҳ��ܽ���:uid {}--cursvrID:{}--gameType:{}", GetUID(), GetGameSvrID(), gameType);
        return RESULT_CODE_SVR_REPAIR;
    }
    SetGameSvrID(pServer->GetSvrID(), false);
    // ������Ϸ���ݵ���Ϸ��
    net::svr::msg_enter_into_game_svr msg;
    msg.set_player_type(GetPlayerType());
    msg.set_play_type(0);
    GetPlayerGameData(&msg);
    SendMsgToGameSvr(&msg, net::svr::L2GS_MSG_ENTER_INTO_SVR);
    LOG_DEBUG("������Ϸ������:{}-->{}", GetUID(), pServer->GetSvrID());
    return RESULT_CODE_SUCCESS;
}

// ˢ���޸���ֵ����Ϸ��
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

// �޸�����˺���ֵ�������޸ģ�
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

// �����½����״̬
void CPlayer::SaveLoginInfo() {
    LOG_DEBUG("�����½login��Ϣ:{}", GetUID());
    CDBMysqlMgr::Instance().UpdatePlayerLoginInfo(GetUID(), m_baseInfo.offline_time, m_baseInfo.clogin, m_baseInfo.weeklogin, 0);
}

CMissionMgr &CPlayer::GetMissionMgr() {
    return m_missionMgr;
}

// ���reloginʱ��
uint32_t CPlayer::GetReloginTime() {
    return m_reloginTime;
}

// �����ӳ�
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

// ��������
void CPlayer::SavePlayerBaseInfo() {

    net::base_info baseInfo;
    GetPlayerBaseData(&baseInfo);

    DUMP_PROTO_MSG_INFO(baseInfo);

    string baseData;
    baseInfo.SerializeToString(&baseData);

    LOG_DEBUG("save player data uid:{},datalen:{},{}", GetUID(), baseData.length(), m_baseInfo.offline_time);

}








