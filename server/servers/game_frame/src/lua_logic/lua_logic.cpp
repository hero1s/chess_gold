
#include "lua_logic.h"
#include "net/lobby_mgr.h"

using namespace svrlib;
using namespace std;


// µ¼³öLuaº¯Êý
void defLuaLogic(sol::state &lua) {
    lua.set_function("ConnectLobby", [](string ip, int32_t port, uint16_t sid)
    {
        return CLobbyMgr::Instance().ConnectLobby(ip, port, sid);
    });

    LOG_DEBUG("export lua logic function ");
}


