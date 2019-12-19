

#pragma once

#include <string>
#include "svrlib.h"
#include <string.h>
#include "config/config.h"
#include "game_define.h"
#include "sol/sol.hpp"

using namespace std;
using namespace svrlib;

/**
 * 单例，用于存放配置
 */

struct GameServerConfig : public AutoDeleteSingleton<GameServerConfig> {
public:
    vector<stDBConf> DBConfs;
    vector<stRedisConf> redisConfs;

    stDBConf *GetDBConf(uint8_t index) {
        if (DBConfs.size() < (size_t) (index + 1))
        {
            DBConfs.resize(index + 1);
        }
        return &DBConfs[index];
    }

    string GetDBName(uint8_t index) {
        if (index < DBConfs.size())
        {
            return DBConfs[index].sDBName;
        }
        return "";
    }

    stRedisConf *GetRedisConf(uint8_t index) {
        if (redisConfs.size() < (size_t) (index + 1))
        {
            redisConfs.resize(index + 1);
        }
        return &redisConfs[index];
    }

};

// 导出Lua函数
extern void defLuaConfig(sol::state &lua);

