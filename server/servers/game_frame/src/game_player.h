
#pragma once

#include "svrlib.h"
#include "player_base.h"
#include "game_define.h"
#include "msg_define.pb.h"
#include "game_table/game_table.h"

using namespace svrlib;
using namespace std;
using namespace net;
using namespace Network;

class CGameRoom;

class CGameTable;

class CGamePlayer : public CPlayerBase, public std::enable_shared_from_this<CGamePlayer> {
public:
    CGamePlayer(PLAYER_TYPE type);

    virtual ~CGamePlayer();

    virtual bool SendMsgToClient(const google::protobuf::Message *msg, uint16_t msg_type);

    virtual void OnLoginOut();

    virtual void OnLogin();

    virtual void ReLogin();

    // ֪ͨ���ش���(�˳���Ϸ)
    void NotifyLeaveGameSvr();

    // �޸�����˺���ֵ�������޸ģ�
    virtual void SyncChangeAccountValue(uint16_t operType, uint16_t subType, int64_t coin, int64_t safecoin, const string &chessid = "");

    // �ܷ��˳�
    bool CanBackLobby();

    // ��������ʱ��
    void ResetHeart();

    // ����ʱ��
    uint32_t GetDisconnectTime();

    virtual void OnTimeTick(uint64_t uTime, bool bNewDay);

    // �Ƿ���Ҫ����
    virtual bool NeedRecover();

    // �Ƿ�������Ϸ��
    bool IsInGamePlaying();

    // ��Ϣ����
    int OnMessage(uint16_t cmdID, const uint8_t *pkt_buf, uint16_t buf_len);

public:
    uint8_t GetNetState();

    void SetNetState(uint8_t state);

    void SetPlayDisconnect(bool bFlag);

    bool IsPlayDisconnect();

    std::shared_ptr<CGameRoom> GetRoom();

    void SetRoom(std::shared_ptr<CGameRoom> pRoom);

    uint16_t GetRoomID();

    std::shared_ptr<CGameTable> GetTable();

    void SetTable(std::shared_ptr<CGameTable> pTable);

    int64_t GetTableID();

    void SetAutoReady(bool bAuto);

    bool IsAutoReady();

    // �뿪��ǰ����
    std::shared_ptr<CGameTable> TryLeaveCurTable();

protected:
    uint8_t m_netState = 1;                                                 // ����״̬
    uint32_t m_msgHeartTime = 0;                                            // ��Ϣ����ʱ��
    bool m_playDisconnect = false;                                          // ��Ϸ�Ƿ������
    std::shared_ptr<CGameRoom> m_pGameRoom = nullptr;                       // ���ڷ���
    std::shared_ptr<CGameTable> m_pGameTable = nullptr;                     // ��������
    bool m_autoReady = false;                                               // �Զ�׼��

};





