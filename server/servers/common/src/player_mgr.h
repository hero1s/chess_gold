

#pragma once

#include "player_base.h"
#include <unordered_map>
#include "svrlib.h"

#include <string>
#include <memory>

// 在线玩家管理器
using namespace svrlib;
using namespace std;
using namespace Network;

class CPlayerMgr : public AutoDeleteSingleton<CPlayerMgr> {
public:
    CPlayerMgr();

    ~CPlayerMgr();

    void OnTimer();

    bool Init();

    void ShutDown();

    void OnTimeTick();

    bool IsOnline(uint32_t uid);

    template<class T>
    std::shared_ptr<T> GetPlayer(uint32_t uid) {
        auto Iter = m_mpPlayers.find(uid);
        if (Iter != m_mpPlayers.end())
        {
            return dynamic_pointer_cast<T>(Iter->second);
        }
        return nullptr;
    }

    bool AddPlayer(std::shared_ptr<CPlayerBase> pPlayer);

    bool RemovePlayer(std::shared_ptr<CPlayerBase> pPlayer);

    void SendMsgToAll(const google::protobuf::Message *msg, uint16_t msg_type);

    void SendMsgToAll(const void *msg, uint16_t msg_len, uint16_t msg_type);

    bool SendMsgToPlayer(const google::protobuf::Message *msg, uint16_t msg_type, uint32_t uid);

    bool SendMsgToPlayer(const void *msg, uint16_t msg_len, uint16_t msg_type, uint32_t uid);

    uint32_t GetOnlines();

    void GetAllPlayers(vector<std::shared_ptr<CPlayerBase>> &refVec);

    void RecoverPlayer(std::shared_ptr<CPlayerBase> pPlayer);

protected:
    void CheckRecoverPlayer();

private:
    unordered_map<uint32_t, std::shared_ptr<CPlayerBase>> m_mpPlayers;
    MemberTimerEvent<CPlayerMgr, &CPlayerMgr::OnTimer> m_timer;

};




