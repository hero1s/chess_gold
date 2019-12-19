
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

    // 是否需要回收
    virtual bool NeedRecover();

    // 返回大厅回调
    virtual void BackLobby();

    bool CanModifyData();

    //--- 每日清理
    void DailyCleanup(int32_t iOfflineDay);

    //--- 每周清理
    void WeeklyCleanup();

    //--- 每月清理
    void MonthlyCleanup();

    // 信息同步
    void NotifyEnterGame();

    void NotifyLoginOut(uint32_t code, string deviceid = "");

    bool SendAllPlayerData2Client();

    bool SendAccData2Client();

    bool UpdateAccValue2Client();

    // 通知返回大厅
    void NotifyClientBackLobby(uint8_t result, uint8_t reason);

    // 广播通知登录
    void NotifyLobbyLogin();

    // 构建初始化
    void BuildInit();

public:
    // 是否在大厅中
    bool IsInLobby();

    bool SendMsgToGameSvr(const google::protobuf::Message *msg, uint16_t msg_type);

    bool SendMsgToGameSvr(const void *msg, uint16_t msg_len, uint16_t msg_type);

    // 通知网络状态
    void NotifyNetState2GameSvr(uint8_t state);

    // 请求返回大厅
    void ActionReqBackLobby(uint8_t action);

    // 进入游戏服务器
    uint16_t EnterGameSvr(uint16_t gameType);

    // 刷新修改数值到游戏服
    void FlushChangeAccData2GameSvr(int64_t coin, int64_t safecoin);

    // 修改玩家账号数值（增量修改）
    virtual void SyncChangeAccountValue(uint16_t operType, uint16_t subType, int64_t coin, int64_t safecoin, const string& chessid = "");

    // 保存登陆奖励状态
    void SaveLoginInfo();

    CMissionMgr &GetMissionMgr();

    // 获得relogin时间
    uint32_t GetReloginTime();

    // 网络延迟
    uint32_t GetNetDelay();

    bool SetNetDelay(uint32_t netDelay);

    // Flush GPS
    void FlushGPS(double lon, double lat);

protected:
    // 保存数据
    void SavePlayerBaseInfo();

protected:
    uint32_t m_disconnectTime;                                              // 断线时间
    uint32_t m_reloginTime;                                                 // 上次重连时间(限制可能的bug导致频繁重连)
    uint32_t m_loadTime;                                                    // 加载数据时间
    uint32_t m_netDelay;                                                    // 网络延迟
    std::array<uint32_t, emLIMIT_TIME_MAX> m_limitTime;                     // 限制时间
    CMissionMgr m_missionMgr;                                               // 任务管理器
};




