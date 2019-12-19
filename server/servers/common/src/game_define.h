
#pragma once

#include "svrlib.h"
#include <vector>
#include "error_code.pb.h"

using namespace std;

#define PRO_DENO_100w    1000000
#define PRO_DENO_10w     100000
#define PRO_DENO_10000   10000
#define PRO_DENO_100     100

// 数据库标示
enum DB_INDEX_TYPE {
    DB_INDEX_TYPE_ACC = 0,
    DB_INDEX_TYPE_CFG = 1,
    DB_INDEX_TYPE_CENTER = 2,
    DB_INDEX_TYPE_LOG = 3,

    DB_INDEX_TYPE_MAX = 4,
};

// 服务器状态
enum emSERVER_STATE {
    emSERVER_STATE_NORMAL = 0,        // 正常状态
    emSERVER_STATE_REPAIR,            // 维护状态

};
// 服务器类型
enum emSERVER_TYPE {
    emSERVER_TYPE_LOBBY = 1, // 大厅服
    emSERVER_TYPE_GAME = 2, // 游戏服
    emSERVER_TYPE_CENTER = 3, // 中心服

};
// 路由类型
enum emROUTE_TYPE {
    emROUTE_TYPE_ALL_SERVER = 1,// 全部游戏服
    emROUTE_TYPE_ONE_SERVER = 2,// 指定游戏服


};

// 牌局日志
struct stBlingUser {
    uint32_t uid;
    int64_t oldValue;
    int64_t newValue;
    int64_t win;
    int64_t fee;           // 台费
    uint16_t chairid;

    stBlingUser() {
        memset(this, 0, sizeof(stBlingUser));
    }
};

struct stGameBlingLog {
    uint32_t roomID;               // 房间ID
    int64_t tableID;               // 桌子ID
    uint16_t gameType;             // 游戏类型
    uint16_t roomType;             // 房间类型
    uint8_t consume;               // 消费类型
    uint8_t playType;              // 玩法类型
    int64_t baseScore;             // 底分
    uint32_t startTime;            // 开始时间
    uint32_t endTime;              // 结束时间
    vector<stBlingUser> users;     // 用户数据
    stringstream operLog;          // 操作日志
    string chessid;                // 牌局ID

    stGameBlingLog() {
        Reset();
    }

    void Reset() {
        roomID = 0;
        tableID = 0;            // 桌子ID
        gameType = 0;           // 游戏类型
        roomType = 0;           // 房间类型
        consume = 0;            // 消费类型
        playType = 0;           // 玩法类型
        baseScore = 0;          // 底分
        startTime = 0;          // 开始时间
        endTime = 0;            // 结束时间
        users.clear();          // 用户数据
        operLog.str("");
        chessid = "";
    }
};

#ifndef PARSE_MSG_FROM_ARRAY
#define PARSE_MSG_FROM_ARRAY(msg)                   \
    if(!pkt_buf)                                    \
        return -1;                                  \
    if (!msg.ParseFromArray(pkt_buf, buf_len)) {    \
        LOG_ERROR("unpack fail");                   \
        return -1;                                  \
    }

#endif // PARSE_MSG_FROM_ARRAY





























