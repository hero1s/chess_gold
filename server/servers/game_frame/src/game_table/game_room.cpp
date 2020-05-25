
#include <data_cfg_mgr.h>
#include "game_room.h"
#include "game_table.h"
#include "game_server_config.h"
#include "game_room_mgr.h"
#include "msg_define.pb.h"

using namespace svrlib;
using namespace std;
using namespace net;

namespace {
    const static uint8_t s_MaxSendTable = 30;
}

CGameRoom::CGameRoom()
        : m_timer(this) {
    m_playerNum = 0;        // 玩家人数
    m_showonline = 0;
    m_roomIndex = 0;        // 分配房间索引
    m_gameType = 0;
}

CGameRoom::~CGameRoom() {
}

void CGameRoom::OnTimer() {
    AUTOPROFILE("CGameRoom::OnTimer");
    OnTimeTick();
    CApplication::Instance().schedule(&m_timer, 1000);
}

bool CGameRoom::Init(uint16_t gameType) {
    LOG_DEBUG("init game gameType:{},playType:{} room：roomID {}- baseScore {}", gameType, m_roomCfg.playType, m_roomCfg.roomID, m_roomCfg.baseScore);
    m_gameType = gameType;
    // 普通房初始化桌子
    if(!IsNeedMarry())
    {
        for (uint32_t i = 0; i < m_roomCfg.tableNum; ++i)
        {
            MallocTable();
        }
    }
    CApplication::Instance().schedule(&m_timer, 1000);

    return true;
}

void CGameRoom::ShutDown() {
    for (auto &it : m_mpTables)
    {
        auto pTable = it.second;
        pTable->ShutDown();
    }

    m_timer.cancel();
}

void CGameRoom::OnTimeTick() {
    AUTOPROFILE("CGameRoom::OnTimeTick");
    if (m_coolMarry.isTimeOut())
    {
        MarryTable();
        m_coolMarry.beginCooling(1000);
    }
    if (m_coolRecover.isTimeOut())
    {
        CheckRecover();
        m_coolRecover.beginCooling(2000);
    }
    if (m_coolNewTable.isTimeOut())
    {
        CheckNewTable();
        m_coolNewTable.beginCooling(5000);
        CalcShowOnline();
    }
}

bool CGameRoom::EnterRoom(std::shared_ptr<CGamePlayer> pGamePlayer) {
    if (pGamePlayer->GetRoomID() != GetRoomID() && !CanEnterRoom(pGamePlayer))
    {
        LOG_DEBUG("can't join room");
        return false;
    }
    if (pGamePlayer->GetRoomID() != GetRoomID())
    {
        pGamePlayer->SetRoom(shared_from_this());
        pGamePlayer->SetAutoReady(false);
        m_playerNum++;
    }
    auto pTable = pGamePlayer->GetTable();

    net::cli::msg_enter_room_rep rep;
    rep.set_result(1);
    rep.set_cur_table(0);
    if (pTable != nullptr)
    {// 是否还需要进入
        if (pTable->WantNeedRecover())
        {
            pTable = pGamePlayer->TryLeaveCurTable();
        }
    }
    if (pTable != nullptr)
    {
        rep.set_cur_table(pTable->GetTableID());
    }

    GetRoomInfo(rep.mutable_room());
    pGamePlayer->SendMsgToClient(&rep, net::S2C_MSG_ENTER_ROOM_REP);

    CalcShowOnline();
    LOG_DEBUG("EnterRoom uid:{},roomid:{},curtable:{}", pGamePlayer->GetUID(), GetRoomID(), rep.cur_table());

    return true;
}

bool CGameRoom::LeaveRoom(std::shared_ptr<CGamePlayer> pGamePlayer) {
    if (pGamePlayer->GetRoomID() != GetRoomID())
    {
        LOG_ERROR("not the same room:{}", pGamePlayer->GetUID());
        return false;
    }

    auto pTable = pGamePlayer->GetTable();
    if (pTable != nullptr)
    {
        if (!pTable->CanLeaveTable(pGamePlayer))
            return false;
        pTable->LeaveTable(pGamePlayer);
    }
    LeaveMarry(pGamePlayer);
    pGamePlayer->SetRoom(nullptr);
    pGamePlayer->SetAutoReady(false);
    m_playerNum--;

    CalcShowOnline();
    LOG_DEBUG("LeaveRoom uid:{},roomid:{}", pGamePlayer->GetUID(), GetRoomID());

    return true;
}

uint8_t CGameRoom::EnterTable(std::shared_ptr<CGamePlayer> pGamePlayer, int64_t tableID) {
    if (pGamePlayer->GetRoomID() != GetRoomID())
    {
        LOG_ERROR("not the same room:{}", pGamePlayer->GetUID());
        return net::RESULT_CODE_FAIL;
    }
    //获取新旧桌子
    auto pOldTable = pGamePlayer->GetTable();
    auto pNewTable = GetTable(tableID);
    //新桌子不存在直接返回
    if (pNewTable == nullptr)
    {
        LOG_DEBUG("the table is not have:{}", tableID);
        return net::RESULT_CODE_NOT_TABLE;
    }
    if (pOldTable != nullptr)
    {
        if (pOldTable->GetTableID() == tableID)
        {
            LOG_DEBUG("back table");
            pOldTable->OnActionUserNetState(pGamePlayer, true);
            return net::RESULT_CODE_SUCCESS;
        }
        if (!pOldTable->CanLeaveTable(pGamePlayer))
        {
            LOG_DEBUG("can't leave table:{}", pGamePlayer->GetUID());
            return net::RESULT_CODE_FAIL;
        }
        if (!pOldTable->LeaveTable(pGamePlayer))
        {
            LOG_DEBUG("leave table faild:{}", pGamePlayer->GetUID());
            return net::RESULT_CODE_FAIL;
        }
    }
    if (IsNeedMarry())
    {
        JoinMarry(pGamePlayer, 0);
    }
    else
    {
        int32_t ret = pNewTable->CanEnterTable(pGamePlayer);
        if (ret != net::RESULT_CODE_SUCCESS)
        {
            if (FastJoinTable(pGamePlayer))
            {
                return net::RESULT_CODE_SUCCESS;
            }
            else
            {
                LOG_DEBUG("can't join the table");
                return ret;
            }
        }
        if (pNewTable->EnterTable(pGamePlayer))
        {
            return net::RESULT_CODE_SUCCESS;
        }
        return net::RESULT_CODE_FAIL;
    }
    return net::RESULT_CODE_SUCCESS;
}

bool CGameRoom::FastJoinTable(std::shared_ptr<CGamePlayer> pGamePlayer) {
    if (pGamePlayer->GetRoomID() != GetRoomID())
    {
        LOG_ERROR("not the same room:{}", pGamePlayer->GetUID());
        return false;
    }
    auto pOldTable = pGamePlayer->GetTable();
    if (pOldTable == nullptr)
    {
        if (IsNeedMarry())
        {
            JoinMarry(pGamePlayer, 0);
            return true;
        }
        else
        {
            return JoinNoFullTable(pGamePlayer, 0);
        }
    }
    else
    {
        if (pOldTable->CanLeaveTable(pGamePlayer) && pOldTable->LeaveTable(pGamePlayer))
        {
            if (IsNeedMarry())
            {
                JoinMarry(pGamePlayer, pOldTable->GetTableID());
                return true;
            }
            else
            {
                return JoinNoFullTable(pGamePlayer, pOldTable->GetTableID());
            }
        }
        else
        {
            EnterTable(pGamePlayer, pOldTable->GetTableID());
        }
    }
    return false;
}

void CGameRoom::SetRoomCfg(stRoomCfg &cfg) {
    m_roomCfg = cfg;
}

const stRoomCfg &CGameRoom::GetRoomCfg() {
    return m_roomCfg;
}

uint16_t CGameRoom::GetRoomID() {
    return m_roomCfg.roomID;
}

int64_t CGameRoom::GetEnterMin() {
    return m_roomCfg.enter_min;
}

int64_t CGameRoom::GetEnterMax() {
    return m_roomCfg.enter_max;
}

int32_t CGameRoom::GetBaseScore() {
    return m_roomCfg.baseScore;
}

int64_t CGameRoom::GetSitDown() {
    return m_roomCfg.sitdown;
}

bool CGameRoom::IsNeedMarry() {
    return m_roomCfg.marry == 1;
}

int32_t CGameRoom::GetPlayerNum() {
    return m_playerNum;
}

uint16_t CGameRoom::GetGameType() {
    return m_gameType;
}

uint16_t CGameRoom::GetPlayType() {
    return m_roomCfg.playType;
}

bool CGameRoom::CanEnterRoom(std::shared_ptr<CGamePlayer> pGamePlayer) {
    if (pGamePlayer->GetRoom() != nullptr && pGamePlayer->GetRoomID() != GetRoomID())
    {
        LOG_DEBUG("haved join the other room");
        return false;
    }
    if (m_roomCfg.limitEnter != 0)
    {
        if (pGamePlayer->GetAccountValue(emACC_VALUE_COIN) < GetEnterMin())
        {
            LOG_DEBUG("less score--room:{}--min:{}--max:{}", GetRoomID(), GetEnterMin(), GetEnterMax());
            return false;
        }
        if (GetEnterMin() < GetEnterMax() && pGamePlayer->GetAccountValue(emACC_VALUE_COIN) > GetEnterMax())
        {
            LOG_DEBUG("less score--room:{}--min:{}--max:{}", GetRoomID(), GetEnterMin(), GetEnterMax());
            return false;
        }
    }
    return true;
}

bool CGameRoom::CanLeaveRoom(std::shared_ptr<CGamePlayer> pGamePlayer) {
    auto pTable = pGamePlayer->GetTable();
    if (pTable != nullptr && !pTable->CanLeaveTable(pGamePlayer))
        return false;

    return true;
}

void CGameRoom::GetRoomInfo(net::room_info *pRoom) {
    pRoom->set_id(GetRoomID());
    pRoom->set_game_type(m_gameType);
    pRoom->set_play_type(GetPlayType());
    pRoom->set_enter_min(m_roomCfg.enter_min);
    pRoom->set_enter_max(m_roomCfg.enter_max);
    pRoom->set_player_num(m_showonline);
    pRoom->set_basescore(m_roomCfg.baseScore);

}

//发送金币场桌子信息给玩家
void CGameRoom::SendTableListToPlayer(std::shared_ptr<CGamePlayer> pGamePlayer, int64_t tableID) {
    LOG_DEBUG("SendTableList to player:{}", tableID);
    uint8_t tableCount = 0;
    net::cli::msg_table_list_rep msg;
    msg.set_game_type(GetGameType());
    msg.set_is_alter(0);

    if (tableID != 0)
    {
        auto pTable = GetTable(tableID);
        if (pTable != nullptr)
        {
            net::table_info *pInfo = msg.add_tables();
            pTable->GetTableFaceInfo(pInfo);
        }
        pGamePlayer->SendMsgToClient(&msg, net::S2C_MSG_TABLE_LIST);
        return;
    }
    vector<std::shared_ptr<CGameTable>> tables;
    for (auto &it : m_mpTables)
    {
        auto pTable = it.second;
        // 过滤俱乐部房间，
        if (pTable->WantNeedRecover())
            continue;

        tables.push_back(pTable);
    }
    // 排序
    std::sort(tables.begin(), tables.end(), [](auto pTable1, auto pTable2)
    {
        if (pTable1->GetOnlinePlayerNum() == pTable2->GetOnlinePlayerNum())
        {
            if (pTable1->GetPlayerNum() == pTable2->GetPlayerNum())
            {
                return pTable1->GetTableID() < pTable2->GetTableID();
            }
            return pTable1->GetPlayerNum() > pTable2->GetPlayerNum();
        }
        return pTable1->GetOnlinePlayerNum() > pTable2->GetOnlinePlayerNum();
    });

    for (uint32_t i = 0; i < tables.size() && i < s_MaxSendTable; ++i)
    {
        auto pTable = tables[i];
        if (pTable != nullptr && !pTable->WantNeedRecover())
        {
            net::table_info *pInfo = msg.add_tables();
            pTable->GetTableFaceInfo(pInfo);
        }
    }
    LOG_DEBUG("send table list:{}", msg.tables_size());
    pGamePlayer->SendMsgToClient(&msg, net::S2C_MSG_TABLE_LIST);
}

//查询桌子列表信息
void CGameRoom::QueryTableListToPlayer(std::shared_ptr<CGamePlayer> pGamePlayer, uint32_t start, uint32_t end) {
    uint8_t tableCount = 0;
    net::cli::msg_query_table_list_rep msg;
    vector<std::shared_ptr<CGameTable>> tables;
    for (auto &it : m_mpTables)
    {
        auto pTable = it.second;
        tables.push_back(pTable);
    }
    // 排序
    msg.set_table_num(tables.size());
    std::sort(tables.begin(), tables.end(), [](auto pTable1, auto pTable2)
    {
        return pTable1->GetTableID() < pTable2->GetTableID();
    });

    for (uint32_t i = start; i < tables.size() && i <= end && tableCount < s_MaxSendTable; ++i)
    {
        auto pTable = tables[i];
        net::table_info *pInfo = msg.add_tables();
        pTable->GetTableFaceInfo(pInfo);
        tableCount++;
    }
    pGamePlayer->SendMsgToClient(&msg, net::S2C_MSG_QUERY_TABLE_LIST_REP);
}

// 拷贝房间配置信息
void CGameRoom::CopyRoomCfgToTableCfg(stTableConf &conf) {
    conf.baseScore = GetBaseScore();
    conf.enterMin = GetEnterMin();
    conf.enterMax = GetEnterMax();
    conf.feeType = m_roomCfg.feeType;
    conf.feeValue = m_roomCfg.feeValue;
    conf.seatNum = m_roomCfg.seatNum;
    conf.playType = m_roomCfg.playType;
}

std::shared_ptr<CGameTable> CGameRoom::MallocTable() {
    std::shared_ptr<CGameTable> pTable = nullptr;
    if (!m_freeTable.empty())
    {
        pTable = m_freeTable.front();
        m_freeTable.pop();
    }
    else
    {
        int64_t tableID = ++m_roomIndex;
        if (m_pCreateFunc)
        {
            pTable = m_pCreateFunc(tableID);
        }
        stTableConf conf;
        CopyRoomCfgToTableCfg(conf);
        pTable->SetTableConf(conf);
        pTable->Init();
    }
    pTable->ResetTable();
    m_mpTables.insert(make_pair(pTable->GetTableID(), pTable));
    LOG_DEBUG("room:{} malloctable:{}", GetRoomID(), pTable->GetTableID());
    return pTable;
}

void CGameRoom::FreeTable(std::shared_ptr<CGameTable> pTable) {
    m_mpTables.erase(pTable->GetTableID());
    m_freeTable.push(pTable);
    LOG_DEBUG("room:{} freetable:{},tables:{},frees:{}", GetRoomID(), pTable->GetTableID(), m_mpTables.size(), m_freeTable.size());
}

std::shared_ptr<CGameTable> CGameRoom::GetTable(int64_t tableID) {
    auto it = m_mpTables.find(tableID);
    if (it != m_mpTables.end())
    {
        return it->second;
    }
    return nullptr;
}

// 检测回收空桌子
void CGameRoom::CheckRecover() {
    vector<std::shared_ptr<CGameTable>> vecRecovers;
    for (auto &it : m_mpTables)
    {
        auto pTable = it.second;
        if (pTable->NeedRecover())
        {
            vecRecovers.push_back(pTable);
        }
    }
    for_each(vecRecovers.begin(), vecRecovers.end(), [&](auto pTable)
    {
        FreeTable(pTable);
    });
}

// 检测是否需要生成新桌子
void CGameRoom::CheckNewTable() {
    if (IsNeedMarry())
        return;
    if (GetFreeTableNum() < 1)
    {
        LOG_DEBUG("not free table and create table");
        MallocTable();
    }
}

// 匹配桌子
void CGameRoom::MarryTable() {
    if (IsNeedMarry())
    {
        std::shared_ptr<CGameTable> pTable = nullptr;
        while (!m_marryPlayers.empty())
        {
            QUEUE_PLAYER::iterator it = m_marryPlayers.begin();
            auto pPlayer = it->first;
            if (JoinNoFullTable(pPlayer, it->second))
            {
                LeaveMarry(pPlayer);
                continue;
            }
            if (pTable == nullptr || !pTable->CanEnterTable(pPlayer))
            {
                pTable = MallocTable();
            }
            if (pTable->EnterTable(pPlayer))
            {
                LeaveMarry(pPlayer);
            }
            else
            {
                LeaveRoom(pPlayer);
            }
        }
    }
}

// 进入空闲桌子
bool CGameRoom::JoinNoFullTable(std::shared_ptr<CGamePlayer> pPlayer, uint32_t excludeID) {
    vector<std::shared_ptr<CGameTable>> readyTables;
    for (auto &it : m_mpTables)
    {
        auto pTable = it.second;
        if (pTable->GetTableID() != excludeID && !pTable->WantNeedRecover() && pTable->CanEnterTable(pPlayer) == net::RESULT_CODE_SUCCESS)
        {
            readyTables.push_back(pTable);
        }
    }
    std::sort(readyTables.begin(), readyTables.end(), [](auto pTable1, auto pTable2)
    {
        if (pTable1->GetReadyNum() == pTable2->GetReadyNum())
        {
            if (pTable1->GetPlayerNum() == pTable2->GetPlayerNum())
            {
                return pTable1->GetTableID() < pTable2->GetTableID();
            }
            return pTable1->GetPlayerNum() > pTable2->GetPlayerNum();
        }
        return pTable1->GetReadyNum() > pTable2->GetReadyNum();
    });
    for (auto pTable : readyTables)
    {
        if (pTable->EnterTable(pPlayer))
        {
            return true;
        }
    }
    return false;
}

// 加入匹配
void CGameRoom::JoinMarry(std::shared_ptr<CGamePlayer> pPlayer, uint32_t excludeID) {
    if (IsJoinMarry(pPlayer))
        return;
    m_marryPlayers.insert(make_pair(pPlayer, excludeID));
}

void CGameRoom::LeaveMarry(std::shared_ptr<CGamePlayer> pPlayer) {
    m_marryPlayers.erase(pPlayer);
}

bool CGameRoom::IsJoinMarry(std::shared_ptr<CGamePlayer> pPlayer) {
    return (m_marryPlayers.find(pPlayer) != m_marryPlayers.end());
}

uint32_t CGameRoom::GetFreeTableNum() {
    uint32_t num = m_freeTable.size();
    for (auto &it : m_mpTables)
    {
        auto pTable = it.second;
        if (!pTable->IsFullTable())
            num++;
    }
    return num;
}

// 设置创桌函数
void CGameRoom::SetCreateTableFunc(create_table_func func) {
    m_pCreateFunc = func;
}

void CGameRoom::CalcShowOnline() {
    if (m_roomCfg.showonline == 0)
    {
        m_showonline = m_playerNum;
    }
    else
    {
        m_showonline = m_playerNum * m_roomCfg.showonline + rand_range(0, 100);
    }
}


