
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

    // 通知返回大厅(退出游戏)
    void NotifyLeaveGameSvr();

    // 修改玩家账号数值（增量修改）
    virtual void SyncChangeAccountValue(uint16_t operType, uint16_t subType, int64_t coin, int64_t safecoin, const string &chessid = "");

    // 能否退出
    bool CanBackLobby();

    // 重置心跳时间
    void ResetHeart();

    // 断线时间
    uint32_t GetDisconnectTime();

    virtual void OnTimeTick(uint64_t uTime, bool bNewDay);

    // 是否需要回收
    virtual bool NeedRecover();

    // 是否正在游戏中
    bool IsInGamePlaying();

    // 消息处理
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

    // 离开当前桌子
    std::shared_ptr<CGameTable> TryLeaveCurTable();

protected:
    uint8_t m_netState = 1;                                                 // 网络状态
    uint32_t m_msgHeartTime = 0;                                            // 消息心跳时间
    bool m_playDisconnect = false;                                          // 游戏是否断线中
    std::shared_ptr<CGameRoom> m_pGameRoom = nullptr;                       // 所在房间
    std::shared_ptr<CGameTable> m_pGameTable = nullptr;                     // 所在桌子
    bool m_autoReady = false;                                               // 自动准备

};





