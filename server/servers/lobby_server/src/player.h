
#pragma once

#include "svrlib.h"
#include "player_base.h"
#include "game_define.h"
#include "msg_define.pb.h"
#include "mission/mission_mgr.h"

using namespace svrlib;
using namespace std;
using namespace net;
using namespace Network;

class CPlayer : public CPlayerBase, public std::enable_shared_from_this<CPlayer> {
    enum LIMIT_TIME {
        emLIMIT_TIME_NETDELAY,
        emLIMIT_TIME_GPS,
        emLIMIT_TIME_MAX,
    };
public:
    CPlayer(PLAYER_TYPE type);

    virtual ~CPlayer();

    virtual void OnLoginOut();

    virtual void OnLogin();

    virtual void OnGetAllData();

    virtual void ReLogin();

    virtual void OnTimeTick(uint64_t uTime, bool bNewDay);

    // �Ƿ���Ҫ����
    virtual bool NeedRecover();

    // ���ش����ص�
    virtual void BackLobby();

    bool CanModifyData();

    //--- ÿ������
    void DailyCleanup(int32_t iOfflineDay);

    //--- ÿ������
    void WeeklyCleanup();

    //--- ÿ������
    void MonthlyCleanup();

    // ��Ϣͬ��
    void NotifyEnterGame();

    void NotifyLoginOut(uint32_t code, string deviceid = "");

    bool SendAllPlayerData2Client();

    bool SendAccData2Client();

    bool UpdateAccValue2Client();

    // ֪ͨ���ش���
    void NotifyClientBackLobby(uint8_t result, uint8_t reason);

    // �㲥֪ͨ��¼
    void NotifyLobbyLogin();

    // ������ʼ��
    void BuildInit();

public:
    // �Ƿ��ڴ�����
    bool IsInLobby();

    bool SendMsgToGameSvr(const google::protobuf::Message *msg, uint16_t msg_type);

    bool SendMsgToGameSvr(const void *msg, uint16_t msg_len, uint16_t msg_type);

    // ֪ͨ����״̬
    void NotifyNetState2GameSvr(uint8_t state);

    // ���󷵻ش���
    void ActionReqBackLobby(uint8_t action);

    // ������Ϸ������
    uint16_t EnterGameSvr(uint16_t gameType);

    // ˢ���޸���ֵ����Ϸ��
    void FlushChangeAccData2GameSvr(int64_t coin, int64_t safecoin);

    // �޸�����˺���ֵ�������޸ģ�
    virtual void SyncChangeAccountValue(uint16_t operType, uint16_t subType, int64_t coin, int64_t safecoin, const string& chessid = "");

    // �����½����״̬
    void SaveLoginInfo();

    CMissionMgr &GetMissionMgr();

    // ���reloginʱ��
    uint32_t GetReloginTime();

    // �����ӳ�
    uint32_t GetNetDelay();

    bool SetNetDelay(uint32_t netDelay);

    // Flush GPS
    void FlushGPS(double lon, double lat);

protected:
    // ��������
    void SavePlayerBaseInfo();

protected:
    uint32_t m_disconnectTime;                                              // ����ʱ��
    uint32_t m_reloginTime;                                                 // �ϴ�����ʱ��(���ƿ��ܵ�bug����Ƶ������)
    uint32_t m_loadTime;                                                    // ��������ʱ��
    uint32_t m_netDelay;                                                    // �����ӳ�
    std::array<uint32_t, emLIMIT_TIME_MAX> m_limitTime;                     // ����ʱ��
    CMissionMgr m_missionMgr;                                               // ���������
};




