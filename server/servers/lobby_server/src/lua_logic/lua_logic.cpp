
#include "lua_logic.h"
#include "player.h"

using namespace svrlib;
using namespace std;


// µ¼³öLuaº¯Êý
void defLuaLogic(sol::state &lua) {
    lua.new_usertype<CPlayer>
            (
                    "CPlayer",
                    "GetUID", &CPlayer::GetUID
            );

    LOG_DEBUG("export lua logic function ");
}


