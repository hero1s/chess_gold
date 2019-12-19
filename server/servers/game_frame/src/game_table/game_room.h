
#pragma once

#include <game_player.h>
#include <queue>
#include "svrlib.h"
#include "db_struct_define.h"
#include "msg_define.pb.h"
#include "game_table.h"
#include <unordered_map>

using namespace svrlib;
using namespace std;

class CGamePlayer;

class CGameTable;

using create_table_func = std::function<std::shared_ptr<CGameTable>(int64_t tableID)>;

// �����߼�
class CGameRoom : public enable_shared_from_this<CGameRoom> {
public:
    CGameRoom();

    virtual ~CGameRoom();

    void OnTimer();

    bool Init(uint16_t gameType);

    void ShutDown();

    void OnTimeTick();

    bool EnterRoom(std::shared_ptr<CGamePlayer> pGamePlayer);

    bool LeaveRoom(std::shared_ptr<CGamePlayer> pGamePlayer);

    uint8_t EnterTable(std::shared_ptr<CGamePlayer> pGamePlayer, int64_t tableID);

    bool FastJoinTable(std::shared_ptr<CGamePlayer> pGamePlayer);

    void SetRoomCfg(stRoomCfg &cfg);

    const stRoomCfg& GetRoomCfg();

    uint16_t GetRoomID();

    //��С����
    int64_t GetEnterMin();

    //������
    int64_t GetEnterMax();

    //�׷�
    int32_t GetBaseScore();

    //����Я��
    int64_t GetSitDown();

    //�Ƿ���Ҫ����
    bool IsNeedMarry();

    //�������
    int32_t GetPlayerNum();

    //��Ϸ����
    uint16_t GetGameType();
    uint16_t GetPlayType();

    bool CanEnterRoom(std::shared_ptr<CGamePlayer> pGamePlayer);

    bool CanLeaveRoom(std::shared_ptr<CGamePlayer> pGamePlayer);

    void GetRoomInfo(net::room_info *pRoom);

    //���ͽ�ҳ�������Ϣ�����
    void SendTableListToPlayer(std::shared_ptr<CGamePlayer> pGamePlayer, int64_t tableID);

    //��ѯ�����б���Ϣ
    void QueryTableListToPlayer(std::shared_ptr<CGamePlayer> pGamePlayer, uint32_t start, uint32_t end);

    //��������������Ϣ
    void CopyRoomCfgToTableCfg(stTableConf &conf);

    std::shared_ptr<CGameTable> MallocTable();

    void FreeTable(std::shared_ptr<CGameTable> pTable);

    // ��ȡ����ID����
    std::shared_ptr<CGameTable> GetTable(int64_t tableID);

    // �����տ�����
    void CheckRecover();

    // ����Ƿ���Ҫ����������
    void CheckNewTable();

    // ƥ������
    void MarryTable();

    // �����������
    bool JoinNoFullTable(std::shared_ptr<CGamePlayer> pPlayer, uint32_t excludeID);

    // ����ƥ��
    void JoinMarry(std::shared_ptr<CGamePlayer> pPlayer, uint32_t excludeID);

    // �뿪ƥ��
    void LeaveMarry(std::shared_ptr<CGamePlayer> pPlayer);

    // �Ƿ���ƥ����
    bool IsJoinMarry(std::shared_ptr<CGamePlayer> pPlayer);

    // ����������
    uint32_t GetFreeTableNum();

    // ���ô�������
    void SetCreateTableFunc(create_table_func func);

protected:
    void CalcShowOnline();

protected:
    using QUEUE_TABLE = queue<std::shared_ptr<CGameTable>>;
    using MAP_TABLE = unordered_map<uint32_t, std::shared_ptr<CGameTable>>;
    using QUEUE_PLAYER = map<std::shared_ptr<CGamePlayer>, uint32_t>;

    MAP_TABLE m_mpTables;                           // ��������
    QUEUE_TABLE m_freeTable;                        // ��������
    QUEUE_PLAYER m_marryPlayers;                    // ƥ��������
    int64_t m_roomIndex;                            // ���䷿������
    uint32_t m_playerNum;                           // �������
    uint32_t m_showonline;                          // ��ʾ����
    stRoomCfg m_roomCfg;                            // ��������
    uint16_t m_gameType;                            // ��Ϸ����
    CCooling m_coolMarry;                           // ����CD
    CCooling m_coolRecover;                         // ��������CD
    CCooling m_coolNewTable;                        // �������������
    MemberTimerEvent<CGameRoom, &CGameRoom::OnTimer> m_timer;
    create_table_func m_pCreateFunc = nullptr;      // ������������
};


