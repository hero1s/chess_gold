
#pragma once

#include "svrlib.h"
#include "db_struct_define.h"
#include "msg_define.pb.h"
#include "servers_msg.pb.h"
#include "network/tcp_conn.h"
#include "packet/protobuf_pkg.h"
#include<bitset>

using namespace svrlib;
using namespace std;
using namespace Network;

enum PLAYER_TYPE {
    PLAYER_TYPE_ONLINE = 0,        // �������
    PLAYER_TYPE_ROBOT,             // ������
};
enum PLAYER_STATE {
    PLAYER_STATE_NULL,              // ��״̬
    PLAYER_STATE_LOAD_DATA,         // ��ȡ����
    PLAYER_STATE_PLAYING,           // ��Ϸ״̬
    PLAYER_STATE_LOGINOUT,          // ����
};

class CPlayerBase {
public:
    CPlayerBase(PLAYER_TYPE type);

    virtual ~CPlayerBase();

    //������Ϣ
    bool SetBaseInfo(stBaseInfo &info);

    bool IsLoadData(uint8_t dataType);

    void SetLoadState(uint8_t dataType);

    //�Ƿ�������
    bool IsLoadOver();

    //���״̬
    void SetPlayerState(uint8_t state);

    uint8_t GetPlayerState();

    bool IsPlaying();

    uint32_t GetUID();

    void SetUID(uint32_t uid);

    PLAYER_TYPE GetPlayerType();

    bool IsRobot();

    string GetPlayerName();

    uint8_t GetSex();

    void SetSession(const TCPConnPtr &conn);

    TCPConnPtr GetSession();

    void SetIP(uint32_t ip);

    uint32_t GetIP();

    string GetIPStr();

    void SetLon(double lon);

    void SetLat(double lat);

    double GetLon();

    double GetLat();

    virtual bool SendMsgToClient(const google::protobuf::Message *msg, uint16_t msg_type);

    virtual bool SendMsgToClient(const void *msg, uint16_t msg_len, uint16_t msg_type);

    virtual void OnLoginOut();

    virtual void OnLogin();

    virtual void OnGetAllData();

    virtual void ReLogin();

    virtual void OnTimeTick(uint64_t uTime, bool bNewDay);

    // �Ƿ���Ҫ����
    virtual bool NeedRecover();

    // ���û���
    void SetNeedRecover(bool bNeed);

    // ��½key
    void SetLoginKey(const string &key);

    string GetLoginKey();

    // ��������
    void GetPlayerBaseData(net::base_info *pInfo);

    void SetPlayerBaseData(const net::base_info &info, bool bSetUid = true);

    // ������Ϸ������
    void SetPlayerGameData(const net::svr::msg_enter_into_game_svr &info);

    void GetPlayerGameData(net::svr::msg_enter_into_game_svr *pInfo);

    // �޸�����˺���ֵ�������޸ģ�
    bool CanChangeAccountValue(emACC_VALUE_TYPE valueType, int64_t value);

    int64_t ChangeAccountValue(emACC_VALUE_TYPE valueType, int64_t value);

    // �޸�����˺���ֵ�������޸ģ�
    virtual void SyncChangeAccountValue(uint16_t operType, uint16_t subType, int64_t coin, int64_t safecoin, const string &chessid = "") = 0;

    // ԭ�Ӳ����˺���ֵ
    virtual bool AtomChangeAccountValue(uint16_t operType, uint16_t subType, int64_t coin, int64_t safecoin);

    int64_t GetAccountValue(emACC_VALUE_TYPE valueType);

    void SetAccountValue(emACC_VALUE_TYPE valueType, int64_t value);

    void SetOfflineTime(uint32_t _time) { m_baseInfo.offline_time = _time; };

    // ��¼lobby
    void SetLoginLobbySvrID(uint16_t svrID);

    uint16_t GetLoginLobbySvrID();

    // ��Ϸ��
    void SetGameSvrID(uint16_t svrID, bool sync);

    void SyncGameSvrID();

    uint16_t GetGameSvrID();

private:

protected:
    uint32_t m_uid;
    PLAYER_TYPE m_bPlayerType;
    TCPConnPtr m_pSession = nullptr;
    uint8_t m_bPlayerState;
    bool m_needRecover;                             // ��Ҫ���߻���
    string m_loginKey = "";                         // ��½key
    uint16_t m_loginLobbySvrID = 0;                 // ��¼����������
    uint16_t m_curSvrID = 0;                        // ��ǰ���ڷ�����ID
    stBaseInfo m_baseInfo;                          // ������Ϣ
    std::bitset<emACCDATA_TYPE_MAX> m_loadState;    // ���ݼ���״̬

};


