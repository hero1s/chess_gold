
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

// 房间逻辑
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

    //最小进入
    int64_t GetEnterMin();

    //最大进入
    int64_t GetEnterMax();

    //底分
    int32_t GetBaseScore();

    //坐下携带
    int64_t GetSitDown();

    //是否需要搓桌
    bool IsNeedMarry();

    //玩家数量
    int32_t GetPlayerNum();

    //游戏类型
    uint16_t GetGameType();
    uint16_t GetPlayType();

    bool CanEnterRoom(std::shared_ptr<CGamePlayer> pGamePlayer);

    bool CanLeaveRoom(std::shared_ptr<CGamePlayer> pGamePlayer);

    void GetRoomInfo(net::room_info *pRoom);

    //发送金币场桌子信息给玩家
    void SendTableListToPlayer(std::shared_ptr<CGamePlayer> pGamePlayer, int64_t tableID);

    //查询桌子列表信息
    void QueryTableListToPlayer(std::shared_ptr<CGamePlayer> pGamePlayer, uint32_t start, uint32_t end);

    //拷贝房间配置信息
    void CopyRoomCfgToTableCfg(stTableConf &conf);

    std::shared_ptr<CGameTable> MallocTable();

    void FreeTable(std::shared_ptr<CGameTable> pTable);

    // 获取桌子ID桌子
    std::shared_ptr<CGameTable> GetTable(int64_t tableID);

    // 检测回收空桌子
    void CheckRecover();

    // 检测是否需要生成新桌子
    void CheckNewTable();

    // 匹配桌子
    void MarryTable();

    // 进入空闲桌子
    bool JoinNoFullTable(std::shared_ptr<CGamePlayer> pPlayer, uint32_t excludeID);

    // 加入匹配
    void JoinMarry(std::shared_ptr<CGamePlayer> pPlayer, uint32_t excludeID);

    // 离开匹配
    void LeaveMarry(std::shared_ptr<CGamePlayer> pPlayer);

    // 是否在匹配中
    bool IsJoinMarry(std::shared_ptr<CGamePlayer> pPlayer);

    // 空闲桌子数
    uint32_t GetFreeTableNum();

    // 设置创桌函数
    void SetCreateTableFunc(create_table_func func);

protected:
    void CalcShowOnline();

protected:
    using QUEUE_TABLE = queue<std::shared_ptr<CGameTable>>;
    using MAP_TABLE = unordered_map<uint32_t, std::shared_ptr<CGameTable>>;
    using QUEUE_PLAYER = map<std::shared_ptr<CGamePlayer>, uint32_t>;

    MAP_TABLE m_mpTables;                           // 在玩桌子
    QUEUE_TABLE m_freeTable;                        // 空闲桌子
    QUEUE_PLAYER m_marryPlayers;                    // 匹配队列玩家
    int64_t m_roomIndex;                            // 分配房间索引
    uint32_t m_playerNum;                           // 玩家人数
    uint32_t m_showonline;                          // 显示在线
    stRoomCfg m_roomCfg;                            // 房间配置
    uint16_t m_gameType;                            // 游戏类型
    CCooling m_coolMarry;                           // 搓桌CD
    CCooling m_coolRecover;                         // 回收桌子CD
    CCooling m_coolNewTable;                        // 检测生成新桌子
    MemberTimerEvent<CGameRoom, &CGameRoom::OnTimer> m_timer;
    create_table_func m_pCreateFunc = nullptr;      // 创建具体桌子
};


