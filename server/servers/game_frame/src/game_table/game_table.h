
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
    GER_NORMAL = 0,        //常规结束
    GER_NO_PLAYER,         //没有玩家
    GER_DISMISS,           //游戏解散
    GER_USER_LEAVE,        //用户强退
    GER_NETWORK_ERROR,     //网络中断

};

// 座位信息
struct stSeat {
    std::shared_ptr<CGamePlayer> pPlayer;
    uint8_t readyStatus;      // 准备状态
    uint8_t autoStatus;       // 托管状态
    uint32_t readyTime;       // 准备时间
    uint8_t overTimes;        // 超时次数
    uint8_t playStatus;       // 游戏状态
    stSeat() {
        Reset();
    }

    bool IsReady() {
        return pPlayer != nullptr && readyStatus == 1;
    }

    void Reset() {
        pPlayer = nullptr;
        readyStatus = 0;      // 准备状态
        autoStatus = 0;       // 托管状态
        readyTime = 0;        // 准备时间
        overTimes = 0;        // 超时次数
        playStatus = 0;       // 游戏状态
    }
};

// 分账算法
struct stDevide {
    uint16_t chairID;     // 座位ID
    bool isBanker;        // 是否庄家
    int64_t curScore;     // 当前分数
    int64_t winScore;     // 输赢分数
    int64_t realWin;      // 实际输赢

    stDevide() {
        Reset();
    }

    void Reset() {
        memset(this, 0, sizeof(stDevide));
    }
};

// 桌子类型
enum TABLE_TYPE {
    emTABLE_TYPE_SYSTEM = 0, // 系统桌子
    emTABLE_TYPE_PLAYER = 1, // 玩家组局桌子
};

// 桌子信息
struct stTableConf {
    int64_t baseScore;    // 底分
    int64_t enterMin;     // 最低进入
    int64_t enterMax;     // 最大带入
    uint8_t feeType;      // 台费类型
    int64_t feeValue;     // 台费值
    uint16_t seatNum;     // 座位数
    uint8_t playType;     // 玩法
    string addParam;      // 附加参数

    stTableConf() {
        baseScore = 0;
        enterMin = 0;
        enterMax = 0;
        feeType = 0;
        feeValue = 0;
        seatNum = 0;  // 座位数
        playType = 0;
        addParam = "";
    }
};

// 游戏桌子
class CGameTable {
public:
    CGameTable(std::shared_ptr<CGameRoom> pRoom, int64_t tableID, uint8_t tableType);

    virtual ~CGameTable();

    void OnProccess();

    int64_t GetTableID();

    uint8_t GetTableType();

    std::shared_ptr<CGameRoom> GetHostRoom();

    // 玩家坐下或者站起
    virtual bool PlayerSitDownStandUp(std::shared_ptr<CGamePlayer> pPlayer, bool sitDown, uint16_t chairID);

    // 添加旁观者
    bool AddLooker(std::shared_ptr<CGamePlayer> pPlayer);

    // 移除旁观者
    bool RemoveLooker(std::shared_ptr<CGamePlayer> pPlayer);

    // 是否旁观
    bool IsExistLooker(uint32_t uid);

    // 重置准备
    virtual bool ResetPlayerReady();

    // 自动准备
    void AutoReadyAll();

    // 全部准备
    bool IsAllReady();

    // 游戏状态
    void SetPlayStatus(uint16_t chairID, uint8_t status);

    //获取状态
    uint8_t GetPlayStatus(uint16_t chairID);

    //重置状态
    void ResetPlayStatus(uint8_t status = 0);

    // 设置玩家自动准备
    bool PlayerSetAuto(std::shared_ptr<CGamePlayer> pPlayer, uint8_t autoType);

    //是否准备
    bool IsReady(std::shared_ptr<CGamePlayer> pPlayer);

    //是否存在玩家
    bool IsExistPlayer(uint32_t uid);

    //玩家数
    uint32_t GetPlayerNum();

    //在线人数
    uint32_t GetOnlinePlayerNum();

    //准备人数
    uint32_t GetReadyNum();

    //未准备人数
    uint32_t GetNoReadyNum();

    //是否满人
    virtual bool IsFullTable();

    //是否空桌
    virtual bool IsEmptyTable();

    //全体掉线时间
    virtual bool IsAllDisconnect(uint32_t disconnectTime);

    //是否掉线
    virtual bool IsDisconnect(uint16_t chairID);

    // 游戏状态
    void SetGameState(uint8_t state);

    uint8_t GetGameState();

    std::shared_ptr<CGamePlayer> GetPlayer(uint16_t chairID);

    uint16_t GetChairID(std::shared_ptr<CGamePlayer> pPlayer);

    //底分
    int64_t GetBaseScore();

    //最小进入
    int64_t GetEnterMin();

    //最大进入
    int64_t GetEnterMax();

    //玩法
    uint8_t GetPlayType();

    //设置桌子配置
    void SetTableConf(stTableConf &conf);

    //获取桌子配置
    const stTableConf &GetTableConf();

    // 玩家当前分数
    virtual int64_t GetPlayerCurScore(std::shared_ptr<CGamePlayer> pPlayer);

    // 是否可为负
    virtual bool CanMinus();

    // 分赃算法
    virtual stDevide GetDevide(uint16_t chairID, int64_t winScore, bool isBanker);

    // 按权重分配
    virtual void DevideByWeight(vector<stDevide> &players, bool isHaveBanker);

    // 按顺序分配
    virtual void DevideByOrder(vector<stDevide> &players, bool isHaveBanker);

public:
    virtual bool EnterTable(std::shared_ptr<CGamePlayer> pPlayer) = 0;

    virtual bool LeaveTable(std::shared_ptr<CGamePlayer> pPlayer, bool bNotify = false, uint8_t leaveType = 0) = 0;

    // 能否进入
    virtual int32_t CanEnterTable(std::shared_ptr<CGamePlayer> pPlayer) = 0;

    // 能否离开
    virtual bool CanLeaveTable(std::shared_ptr<CGamePlayer> pPlayer) = 0;

    // 是否参与游戏
    virtual bool IsJoinPlay(uint16_t chairID) = 0;

    // 能否坐下
    virtual bool CanSitDown(std::shared_ptr<CGamePlayer> pPlayer, uint16_t chairID) = 0;

    // 能否站起
    virtual bool CanStandUp(std::shared_ptr<CGamePlayer> pPlayer) = 0;

    // 需要坐下
    virtual bool NeedSitDown() = 0;

    // 玩家准备
    virtual bool PlayerReady(std::shared_ptr<CGamePlayer> pPlayer, bool bReady) = 0;

    // 是否需要回收
    virtual bool NeedRecover() = 0;

    // 即将回收
    virtual bool WantNeedRecover() = 0;

    // 获得桌子信息
    virtual void GetTableFaceInfo(net::table_info *pInfo) = 0;

public:
    //配置桌子
    virtual bool Init() = 0;

    virtual void ShutDown() = 0;

    //复位桌子
    virtual void ResetTable() = 0;

    virtual void OnTimeTick() = 0;

    //游戏消息
    virtual int OnMessage(std::shared_ptr<CGamePlayer> pPlayer, uint16_t cmdID, const uint8_t *pkt_buf, uint16_t buf_len) = 0;

    virtual int OnGameMessage(std::shared_ptr<CGamePlayer> pPlayer, uint16_t cmdID, const uint8_t *pkt_buf, uint16_t buf_len) = 0;

    //用户断线或重连
    virtual bool OnActionUserNetState(std::shared_ptr<CGamePlayer> pPlayer, bool bConnected, bool isJoin = true) = 0;

    //用户坐下
    virtual bool OnActionUserSitDown(uint16_t wChairID, std::shared_ptr<CGamePlayer> pPlayer) = 0;

    //用户起立
    virtual bool OnActionUserStandUp(uint16_t wChairID, std::shared_ptr<CGamePlayer> pPlayer) = 0;

    //用户同意
    virtual bool OnActionUserOnReady(uint16_t wChairID, std::shared_ptr<CGamePlayer> pPlayer) = 0;

    //玩家进入或离开
    virtual void OnPlayerJoin(bool isJoin, uint16_t chairID, std::shared_ptr<CGamePlayer> pPlayer) = 0;

    // 发送场景信息(断线重连)
    virtual void SendGameScene(std::shared_ptr<CGamePlayer> pPlayer) = 0;

// 消息发送
public:
    virtual void TableMsgToLooker(const google::protobuf::Message *msg, uint16_t msg_type);

    virtual void TableMsgToPlayer(const google::protobuf::Message *msg, uint16_t msg_type, bool bRecord = true);

    virtual void TableMsgToAll(const google::protobuf::Message *msg, uint16_t msg_type, bool bRecord = true);

    virtual void
    TableMsgToClient(uint16_t chairID, const google::protobuf::Message *msg, uint16_t msg_type, bool bRecord = true);

    // 发送桌子信息
    void SendTableInfoToClient(std::shared_ptr<CGamePlayer> pPlayer);

    // 发送准备状态
    void SendReadyStateToClient();

    // 发送座位信息
    void SendSeatInfoToClient(std::shared_ptr<CGamePlayer> pGamePlayer = nullptr);

    // 刷新座位数值飘分
    void FlushSeatValueInfoToClient(bool bShowChange = false);

    // 发送旁观列表
    void SendLookerListToClient(std::shared_ptr<CGamePlayer> pGamePlayer = nullptr);

    // 通知玩家加入
    void NotifyPlayerJoin(std::shared_ptr<CGamePlayer> pPlayer, bool isJoin);

    // 通知离开
    void NotifyPlayerLeave(std::shared_ptr<CGamePlayer> pPlayer, uint8_t leaveType = 0);

protected:
    // 重置座位信息
    void ResetInitSeat(uint8_t seatNum);

    // 玩家坐下
    void SitDown(stSeat &seat, std::shared_ptr<CGamePlayer> pPlayer);

protected:
    void GetSeatInfo(net::cli::msg_seat_info_rep &msg);

    void GetReadyInfo(net::cli::msg_table_ready_rep &msg);

    // 获得桌子基本信息
    void GetTableFaceBaseInfo(net::table_info *pBaseInfo);

protected:
    // 牌局日志
    void InitBlingLog(bool bNeedReady = false);

    // 修改用户积分到日志
    void ChangeUserBlingLog(std::shared_ptr<CGamePlayer> pPlayer, int64_t winScore);

    // 抽水记录
    void ChangeUserBlingLogFee(uint32_t uid, int64_t fee);

    // 保存牌局记录
    void SaveBlingLog();

    // 操作日志
    void WriteBankerLog(uint16_t chairID);

    // 手牌
    void WriteHandleCardLog(uint16_t chairID, uint8_t cardData[], uint8_t cardCount, uint8_t cardType, int64_t score);

    // 出牌
    void WriteOutCardLog(uint16_t chairID, uint8_t cardData[], uint8_t cardCount, uint8_t eFlag);

    // 牌局录像
    void InitRecordGameMsg();

    // 添加牌局录像消息
    void PushRecordGameMsg(const google::protobuf::Message *msg, uint16_t msg_type, uint32_t uid = 0);

    // 保存牌局录像
    void SaveRecordGameMsg();

protected:
    std::shared_ptr<CGameRoom> m_pHostRoom;                                   // 所属房间
    MemberTimerEvent<CGameTable, &CGameTable::OnProccess> m_timer;            // 定时器
    stTableConf m_conf;                                                       // 桌子配置
    vector<stSeat> m_vecPlayers;                                              // 玩家
    map<uint32_t, std::shared_ptr<CGamePlayer>> m_mpLookers;                  // 围观者
    uint8_t m_gameState;                                                      // 游戏状态
    int64_t m_tableID;                                                        // 桌子ID
    CCooling m_coolLogic;                                                     // 逻辑CD
    uint8_t m_tableType;                                                      // 桌子类型(动态桌子，私人桌子);
    //日志
    stGameBlingLog m_blingLog;                                                // 牌局日志
    net::game_record m_gameRecord;                                            // 游戏录像
    json m_operLog;                                                           // 操作日志
    bool m_openRecord;                                                        // 开启录像
    bool m_needOpenRecord;                                                    // 需要开启录像
    string m_chessid;                                                         // 牌局ID
    uint32_t m_round;                                                         // 当前轮数
};




