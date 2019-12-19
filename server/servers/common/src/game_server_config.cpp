

#include "game_server_config.h"

using namespace svrlib;
using namespace std;

// ����Lua����
void defLuaConfig(sol::state &lua) {
    lua.new_usertype<GameServerConfig>
            (
                    "GameServerConfig",
                    "GetDBConf", &GameServerConfig::GetDBConf,
                    "GetRedisConf", &GameServerConfig::GetRedisConf
            );
    LOG_DEBUG("export sol lua function");
}



































