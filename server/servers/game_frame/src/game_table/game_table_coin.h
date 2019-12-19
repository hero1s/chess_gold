//
// Created by toney on 2019/9/13.
//

#pragma once

#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "msg_define.pb.h"
#include "nlohmann/json_wrap.h"

using namespace svrlib;
using namespace std;

class CGamePlayer;

class CGameRoom;

// 游戏金币场桌子
class CGameCoinTable : public CGameTable, public enable_shared_from_this<CGameCoinTable> {
public:
    CGameCoinTable(std::shared_ptr<CGameRoom> pRoom, int64_t tableID);

    virtual ~CGameCoinTable();

public://重载函数
    virtual bool Init();

    virtual void ShutDown();

    //复位桌子
    virtual void ResetTable();

    //游戏消息
    virtual int OnMessage(std::shared_ptr<CGamePlayer> pPlayer, uint16_t cmdID, const uint8_t *pkt_buf, uint16_t buf_len);

    virtual bool EnterTable(std::shared_ptr<CGamePlayer> pPlayer);

    virtual bool LeaveTable(std::shared_ptr<CGamePlayer> pPlayer, bool bNotify = false, uint8_t leaveType = 0);

    // 能否进入
    virtual int32_t CanEnterTable(std::shared_ptr<CGamePlayer> pPlayer);

    // 能否离开
    virtual bool CanLeaveTable(std::shared_ptr<CGamePlayer> pPlayer);

    // 是否参与游戏
    virtual bool IsJoinPlay(uint16_t chairID);

    // 能否坐下
    virtual bool CanSitDown(std::shared_ptr<CGamePlayer> pPlayer, uint16_t chairID);

    // 能否站起
    virtual bool CanStandUp(std::shared_ptr<CGamePlayer> pPlayer);

    // 需要坐下
    virtual bool NeedSitDown();

    // 玩家准备
    virtual bool PlayerReady(std::shared_ptr<CGamePlayer> pPlayer, bool bReady);

    //用户断线或重连
    virtual bool OnActionUserNetState(std::shared_ptr<CGamePlayer> pPlayer, bool bConnected, bool isJoin = true);

    //用户坐下
    virtual bool OnActionUserSitDown(uint16_t wChairID, std::shared_ptr<CGamePlayer> pPlayer);

    //用户起立
    virtual bool OnActionUserStandUp(uint16_t wChairID, std::shared_ptr<CGamePlayer> pPlayer);

    //用户同意
    virtual bool OnActionUserOnReady(uint16_t wChairID, std::shared_ptr<CGamePlayer> pPlayer);

    //玩家进入或离开
    virtual void OnPlayerJoin(bool isJoin, uint16_t chairID, std::shared_ptr<CGamePlayer> pPlayer);

    // 是否需要回收
    virtual bool NeedRecover();

    // 即将回收
    virtual bool WantNeedRecover();

protected:
    // 没积分的玩家自动站起
    virtual void StandUpNotScore();

    virtual void LeaveAllPlayer();

    // 扣除开始台费
    virtual void DeductStartFee(bool bNeedReady);

    // 扣除结算台费
    virtual void DeducEndFee(uint32_t uid, int64_t &winScore);

    // 上报游戏战报
    virtual void ReportGameResult(shared_ptr<CGamePlayer> pPlayer, int64_t winScore);

    // 结算玩家信息
    virtual void CalcPlayerGameInfo(uint32_t uid, int64_t winScore);

    virtual int64_t ChangeScoreValueByUID(uint32_t uid, int64_t &score, uint16_t operType, uint16_t subType);

    // 一局开始
    virtual void OnGameRoundStart();

    // 一局结束
    virtual void OnGameRoundOver();

    virtual void OnGameRoundFlush();

    // 超时清理玩家
    void OnTimerClearPlayer();

protected:
    CCooling m_coolRobot;//机器人思考CD
    MemberTimerEvent<CGameCoinTable, &CGameCoinTable::OnTimerClearPlayer> m_timerClear;   // 清理玩家定时器
};



