
#pragma once

#include "game_player.h"
#include "svrlib.h"
#include "msg_define.pb.h"
#include "nlohmann/json_wrap.h"
#include "time/cooling.h"
#include "msg_define.pb.h"
#include "client_logic_msg.pb.h"
#include "game_define.h"

using namespace svrlib;
using namespace std;

class CGamePlayer;

class CGameRoom;

//table log
#ifndef LOG_TAB
#define LOG_TAB_DEBUG(x, ...) spdlog::get("log")->debug("[{}:{}][{}][{}] " x,__FILENAME__,__LINE__,__FUNCTION__,GetTableID(), ##__VA_ARGS__)
#define LOG_TAB_ERROR(x, ...) spdlog::get("log")->error("[{}:{}][{}][{}] " x,__FILENAME__,__LINE__,__FUNCTION__,GetTableID(), ##__VA_ARGS__)
#endif

enum GAME_END_TYPE {
    GER_NORMAL = 0,        //�������
    GER_NO_PLAYER,         //û�����
    GER_DISMISS,           //��Ϸ��ɢ
    GER_USER_LEAVE,        //�û�ǿ��
    GER_NETWORK_ERROR,     //�����ж�

};

// ��λ��Ϣ
struct stSeat {
    std::shared_ptr<CGamePlayer> pPlayer;
    uint8_t readyStatus;      // ׼��״̬
    uint8_t autoStatus;       // �й�״̬
    uint32_t readyTime;       // ׼��ʱ��
    uint8_t overTimes;        // ��ʱ����
    uint8_t playStatus;       // ��Ϸ״̬
    stSeat() {
        Reset();
    }

    bool IsReady() {
        return pPlayer != nullptr && readyStatus == 1;
    }

    void Reset() {
        pPlayer = nullptr;
        readyStatus = 0;      // ׼��״̬
        autoStatus = 0;       // �й�״̬
        readyTime = 0;        // ׼��ʱ��
        overTimes = 0;        // ��ʱ����
        playStatus = 0;       // ��Ϸ״̬
    }
};

// �����㷨
struct stDevide {
    uint16_t chairID;     // ��λID
    bool isBanker;        // �Ƿ�ׯ��
    int64_t curScore;     // ��ǰ����
    int64_t winScore;     // ��Ӯ����
    int64_t realWin;      // ʵ����Ӯ

    stDevide() {
        Reset();
    }

    void Reset() {
        memset(this, 0, sizeof(stDevide));
    }
};

// ��������
enum TABLE_TYPE {
    emTABLE_TYPE_SYSTEM = 0, // ϵͳ����
    emTABLE_TYPE_PLAYER = 1, // ����������
};

// ������Ϣ
struct stTableConf {
    int64_t baseScore;    // �׷�
    int64_t enterMin;     // ��ͽ���
    int64_t enterMax;     // ������
    uint8_t feeType;      // ̨������
    int64_t feeValue;     // ̨��ֵ
    uint16_t seatNum;     // ��λ��
    uint8_t playType;     // �淨
    string addParam;      // ���Ӳ���

    stTableConf() {
        baseScore = 0;
        enterMin = 0;
        enterMax = 0;
        feeType = 0;
        feeValue = 0;
        seatNum = 0;  // ��λ��
        playType = 0;
        addParam = "";
    }
};

// ��Ϸ����
class CGameTable {
public:
    CGameTable(std::shared_ptr<CGameRoom> pRoom, int64_t tableID, uint8_t tableType);

    virtual ~CGameTable();

    void OnProccess();

    int64_t GetTableID();

    uint8_t GetTableType();

    std::shared_ptr<CGameRoom> GetHostRoom();

    // ������»���վ��
    virtual bool PlayerSitDownStandUp(std::shared_ptr<CGamePlayer> pPlayer, bool sitDown, uint16_t chairID);

    // ����Թ���
    bool AddLooker(std::shared_ptr<CGamePlayer> pPlayer);

    // �Ƴ��Թ���
    bool RemoveLooker(std::shared_ptr<CGamePlayer> pPlayer);

    // �Ƿ��Թ�
    bool IsExistLooker(uint32_t uid);

    // ����׼��
    virtual bool ResetPlayerReady();

    // �Զ�׼��
    void AutoReadyAll();

    // ȫ��׼��
    bool IsAllReady();

    // ��Ϸ״̬
    void SetPlayStatus(uint16_t chairID, uint8_t status);

    //��ȡ״̬
    uint8_t GetPlayStatus(uint16_t chairID);

    //����״̬
    void ResetPlayStatus(uint8_t status = 0);

    // ��������Զ�׼��
    bool PlayerSetAuto(std::shared_ptr<CGamePlayer> pPlayer, uint8_t autoType);

    //�Ƿ�׼��
    bool IsReady(std::shared_ptr<CGamePlayer> pPlayer);

    //�Ƿ�������
    bool IsExistPlayer(uint32_t uid);

    //�����
    uint32_t GetPlayerNum();

    //��������
    uint32_t GetOnlinePlayerNum();

    //׼������
    uint32_t GetReadyNum();

    //δ׼������
    uint32_t GetNoReadyNum();

    //�Ƿ�����
    virtual bool IsFullTable();

    //�Ƿ����
    virtual bool IsEmptyTable();

    //ȫ�����ʱ��
    virtual bool IsAllDisconnect(uint32_t disconnectTime);

    //�Ƿ����
    virtual bool IsDisconnect(uint16_t chairID);

    // ��Ϸ״̬
    void SetGameState(uint8_t state);

    uint8_t GetGameState();

    std::shared_ptr<CGamePlayer> GetPlayer(uint16_t chairID);

    uint16_t GetChairID(std::shared_ptr<CGamePlayer> pPlayer);

    //�׷�
    int64_t GetBaseScore();

    //��С����
    int64_t GetEnterMin();

    //������
    int64_t GetEnterMax();

    //�淨
    uint8_t GetPlayType();

    //������������
    void SetTableConf(stTableConf &conf);

    //��ȡ��������
    const stTableConf &GetTableConf();

    // ��ҵ�ǰ����
    virtual int64_t GetPlayerCurScore(std::shared_ptr<CGamePlayer> pPlayer);

    // �Ƿ��Ϊ��
    virtual bool CanMinus();

    // �����㷨
    virtual stDevide GetDevide(uint16_t chairID, int64_t winScore, bool isBanker);

    // ��Ȩ�ط���
    virtual void DevideByWeight(vector<stDevide> &players, bool isHaveBanker);

    // ��˳�����
    virtual void DevideByOrder(vector<stDevide> &players, bool isHaveBanker);

public:
    virtual bool EnterTable(std::shared_ptr<CGamePlayer> pPlayer) = 0;

    virtual bool LeaveTable(std::shared_ptr<CGamePlayer> pPlayer, bool bNotify = false, uint8_t leaveType = 0) = 0;

    // �ܷ����
    virtual int32_t CanEnterTable(std::shared_ptr<CGamePlayer> pPlayer) = 0;

    // �ܷ��뿪
    virtual bool CanLeaveTable(std::shared_ptr<CGamePlayer> pPlayer) = 0;

    // �Ƿ������Ϸ
    virtual bool IsJoinPlay(uint16_t chairID) = 0;

    // �ܷ�����
    virtual bool CanSitDown(std::shared_ptr<CGamePlayer> pPlayer, uint16_t chairID) = 0;

    // �ܷ�վ��
    virtual bool CanStandUp(std::shared_ptr<CGamePlayer> pPlayer) = 0;

    // ��Ҫ����
    virtual bool NeedSitDown() = 0;

    // ���׼��
    virtual bool PlayerReady(std::shared_ptr<CGamePlayer> pPlayer, bool bReady) = 0;

    // �Ƿ���Ҫ����
    virtual bool NeedRecover() = 0;

    // ��������
    virtual bool WantNeedRecover() = 0;

    // ���������Ϣ
    virtual void GetTableFaceInfo(net::table_info *pInfo) = 0;

public:
    //��������
    virtual bool Init() = 0;

    virtual void ShutDown() = 0;

    //��λ����
    virtual void ResetTable() = 0;

    virtual void OnTimeTick() = 0;

    //��Ϸ��Ϣ
    virtual int OnMessage(std::shared_ptr<CGamePlayer> pPlayer, uint16_t cmdID, const uint8_t *pkt_buf, uint16_t buf_len) = 0;

    virtual int OnGameMessage(std::shared_ptr<CGamePlayer> pPlayer, uint16_t cmdID, const uint8_t *pkt_buf, uint16_t buf_len) = 0;

    //�û����߻�����
    virtual bool OnActionUserNetState(std::shared_ptr<CGamePlayer> pPlayer, bool bConnected, bool isJoin = true) = 0;

    //�û�����
    virtual bool OnActionUserSitDown(uint16_t wChairID, std::shared_ptr<CGamePlayer> pPlayer) = 0;

    //�û�����
    virtual bool OnActionUserStandUp(uint16_t wChairID, std::shared_ptr<CGamePlayer> pPlayer) = 0;

    //�û�ͬ��
    virtual bool OnActionUserOnReady(uint16_t wChairID, std::shared_ptr<CGamePlayer> pPlayer) = 0;

    //��ҽ�����뿪
    virtual void OnPlayerJoin(bool isJoin, uint16_t chairID, std::shared_ptr<CGamePlayer> pPlayer) = 0;

    // ���ͳ�����Ϣ(��������)
    virtual void SendGameScene(std::shared_ptr<CGamePlayer> pPlayer) = 0;

// ��Ϣ����
public:
    virtual void TableMsgToLooker(const google::protobuf::Message *msg, uint16_t msg_type);

    virtual void TableMsgToPlayer(const google::protobuf::Message *msg, uint16_t msg_type, bool bRecord = true);

    virtual void TableMsgToAll(const google::protobuf::Message *msg, uint16_t msg_type, bool bRecord = true);

    virtual void
    TableMsgToClient(uint16_t chairID, const google::protobuf::Message *msg, uint16_t msg_type, bool bRecord = true);

    // ����������Ϣ
    void SendTableInfoToClient(std::shared_ptr<CGamePlayer> pPlayer);

    // ����׼��״̬
    void SendReadyStateToClient();

    // ������λ��Ϣ
    void SendSeatInfoToClient(std::shared_ptr<CGamePlayer> pGamePlayer = nullptr);

    // ˢ����λ��ֵƮ��
    void FlushSeatValueInfoToClient(bool bShowChange = false);

    // �����Թ��б�
    void SendLookerListToClient(std::shared_ptr<CGamePlayer> pGamePlayer = nullptr);

    // ֪ͨ��Ҽ���
    void NotifyPlayerJoin(std::shared_ptr<CGamePlayer> pPlayer, bool isJoin);

    // ֪ͨ�뿪
    void NotifyPlayerLeave(std::shared_ptr<CGamePlayer> pPlayer, uint8_t leaveType = 0);

protected:
    // ������λ��Ϣ
    void ResetInitSeat(uint8_t seatNum);

    // �������
    void SitDown(stSeat &seat, std::shared_ptr<CGamePlayer> pPlayer);

protected:
    void GetSeatInfo(net::cli::msg_seat_info_rep &msg);

    void GetReadyInfo(net::cli::msg_table_ready_rep &msg);

    // ������ӻ�����Ϣ
    void GetTableFaceBaseInfo(net::table_info *pBaseInfo);

protected:
    // �ƾ���־
    void InitBlingLog(bool bNeedReady = false);

    // �޸��û����ֵ���־
    void ChangeUserBlingLog(std::shared_ptr<CGamePlayer> pPlayer, int64_t winScore);

    // ��ˮ��¼
    void ChangeUserBlingLogFee(uint32_t uid, int64_t fee);

    // �����ƾּ�¼
    void SaveBlingLog();

    // ������־
    void WriteBankerLog(uint16_t chairID);

    // ����
    void WriteHandleCardLog(uint16_t chairID, uint8_t cardData[], uint8_t cardCount, uint8_t cardType, int64_t score);

    // ����
    void WriteOutCardLog(uint16_t chairID, uint8_t cardData[], uint8_t cardCount, uint8_t eFlag);

    // �ƾ�¼��
    void InitRecordGameMsg();

    // ����ƾ�¼����Ϣ
    void PushRecordGameMsg(const google::protobuf::Message *msg, uint16_t msg_type, uint32_t uid = 0);

    // �����ƾ�¼��
    void SaveRecordGameMsg();

protected:
    std::shared_ptr<CGameRoom> m_pHostRoom;                                   // ��������
    MemberTimerEvent<CGameTable, &CGameTable::OnProccess> m_timer;            // ��ʱ��
    stTableConf m_conf;                                                       // ��������
    vector<stSeat> m_vecPlayers;                                              // ���
    map<uint32_t, std::shared_ptr<CGamePlayer>> m_mpLookers;                  // Χ����
    uint8_t m_gameState;                                                      // ��Ϸ״̬
    int64_t m_tableID;                                                        // ����ID
    CCooling m_coolLogic;                                                     // �߼�CD
    uint8_t m_tableType;                                                      // ��������(��̬���ӣ�˽������);
    //��־
    stGameBlingLog m_blingLog;                                                // �ƾ���־
    net::game_record m_gameRecord;                                            // ��Ϸ¼��
    json m_operLog;                                                           // ������־
    bool m_openRecord;                                                        // ����¼��
    bool m_needOpenRecord;                                                    // ��Ҫ����¼��
    string m_chessid;                                                         // �ƾ�ID
    uint32_t m_round;                                                         // ��ǰ����
};




