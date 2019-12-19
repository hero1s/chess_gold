
#pragma once

#include "svrlib.h"

// 玩家基本信息
struct stBaseInfo {
    string name;                    // 昵称
    uint32_t sex;                   // 性别
    int64_t coin;                   // 金币
    int64_t safecoin;               // 保险箱资产
    uint32_t vip;                   // vip经验值
    uint32_t clogin;                // 连续登陆天数
    uint32_t weeklogin;             // 本周累计登陆天数
    uint32_t login_ip;              // 登陆IP
    uint32_t all_login_days;        // 累计登陆天数
    uint32_t offline_time;          // 下线时间
    double lon;                     // 经纬度
    double lat;                     // 经纬度

    stBaseInfo() {
        name = "";
        sex = 0;
        coin = 0;
        safecoin = 0;
        vip = 0;
        clogin = 0;
        weeklogin = 0;
        login_ip = 0;
        all_login_days = 0;
        offline_time = 0;
        lon = 0;
        lat = 0;
    }
};

enum emACC_VALUE_TYPE {
    emACC_VALUE_COIN        = 1, // 金币
    emACC_VALUE_SAFECOIN    = 2, // 保险箱值

    emACC_VALUE_MAX,             // max
};

// 数据类型
enum emACCDATA_TYPE {
    emACCDATA_TYPE_BASE = 0,    // 基本信息
    emACCDATA_TYPE_MISS,        // 任务数据

    emACCDATA_TYPE_MAX,
};

// 房间配置信息
struct stRoomCfg {
    uint16_t roomID;            // 房间ID
    uint16_t playType;          // 玩法
    int64_t enter_min;          // 进入门槛
    int64_t enter_max;          // 进入限制
    int64_t baseScore;          // 底分
    uint16_t tableNum;          // 创建桌子数
    uint8_t marry;              // 匹配方式
    uint8_t limitEnter;         // 进入限制(通过最小携带)
    uint32_t showonline;        // 显示在线
    int64_t sitdown;            // 坐下条件
    uint8_t feeType;            // 台费类型
    int32_t feeValue;           // 台费值
    uint16_t seatNum;           // 座位数

    stRoomCfg() {
        memset(this, 0, sizeof(stRoomCfg));
    }
};

//任务奖励配置
struct stMissionPrizeCfg
{
    uint32_t poid;   //道具id
    uint32_t qty;    //数量
    stMissionPrizeCfg()
    {
        memset(this, 0, sizeof(stMissionPrizeCfg));
    }
};

//任务配置信息
struct stMissionCfg
{
    uint32_t                    msid;        //任务ID
    uint16_t                    type;        //动作类型
    uint8_t                     autoprize;   //自动领奖
    uint32_t                    cate1;       //分类
    uint32_t                    cate2;       //分类
    uint32_t                    cate3;       //分类
    uint32_t                    cate4;       //分类
    uint32_t                    mtimes;      //达到次数
    uint8_t                     straight;     //是否连续
    uint8_t                     cycle;        //周期
    uint32_t                    cycletimes;  //可完成次数
    uint8_t                     status;       //任务状态
    vector<stMissionPrizeCfg> missionprize; //任务奖励
};

//玩家任务信息
struct stUserMission
{
    uint32_t msid;    // ID
    uint32_t rtimes;  // 任务进度
    uint32_t ctimes;  // 任务完成次数
    uint32_t ptime;   // 操作时间
    uint8_t  update;  // 更新数据库类型
    uint32_t cptime;  // 完成时间
    stUserMission()
    {
        memset(this, 0, sizeof(stUserMission));
    }
};

enum emDBACTION
{
    emDB_ACTION_NONE = 0,           //不操作
    emDB_ACTION_UPDATE,             //修改
    emDB_ACTION_INSERT,             //插入
    emDB_ACTION_DELETE,             //删除
};





