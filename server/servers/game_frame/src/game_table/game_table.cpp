
#include "game_table.h"
#include "game_room.h"
#include "mysql_mgr/dbmysql_mgr.h"
#include "net/lobby_mgr.h"
#include "data_cfg_mgr.h"
#include "snappy/snappy.h"

using namespace svrlib;
using namespace std;
using namespace net;

CGameTable::CGameTable(std::shared_ptr<CGameRoom> pRoom, int64_t tableID, uint8_t tableType)
        : m_pHostRoom(pRoom), m_timer(this), m_tableID(tableID), m_tableType(tableType) {
    m_coolLogic.clearCool();
    m_mpLookers.clear();
    m_round = 0;
    m_openRecord = false;
    m_needOpenRecord = false;

    CApplication::Instance().schedule(&m_timer, 200);
}

CGameTable::~CGameTable() {

}

void CGameTable::OnProccess() {
    AutoProfile("CGameTable::OnProccess");
    OnTimeTick();
    CApplication::Instance().schedule(&m_timer, 200);
}

int64_t CGameTable::GetTableID() {
    return m_tableID;
}

uint8_t CGameTable::GetTableType() {
    return m_tableType;
}

std::shared_ptr<CGameRoom> CGameTable::GetHostRoom() {
    return m_pHostRoom;
}

bool CGameTable::AddLooker(std::shared_ptr<CGamePlayer> pPlayer) {
    m_mpLookers[pPlayer->GetUID()] = pPlayer;
    NotifyPlayerJoin(pPlayer, true);
    return true;
}

bool CGameTable::RemoveLooker(std::shared_ptr<CGamePlayer> pPlayer) {
    m_mpLookers.erase(pPlayer->GetUID());
    NotifyPlayerJoin(pPlayer, false);
    return true;
}

bool CGameTable::IsExistLooker(uint32_t uid) {
    auto it = m_mpLookers.find(uid);
    if (it != m_mpLookers.end())
    {
        return true;
    }
    return false;
}

bool CGameTable::PlayerSitDownStandUp(std::shared_ptr<CGamePlayer> pPlayer, bool sitDown, uint16_t chairID) {
    if (sitDown)//坐下
    {
        if (!CanSitDown(pPlayer, chairID))
            return false;
        auto &seat = m_vecPlayers[chairID];
        if (seat.pPlayer == nullptr)
        {
            SitDown(seat, pPlayer);
            LOG_TAB_DEBUG("sitdown：room:{}--tb:{},chairID:{},uid:{}", m_pHostRoom->GetRoomID(), GetTableID(), chairID,
                          pPlayer->GetUID());
            RemoveLooker(pPlayer);
            OnActionUserSitDown(chairID, pPlayer);
            return true;
        }
        else
        {
            LOG_TAB_DEBUG("该位置已经有人占了:{}--{}", pPlayer->GetUID(), chairID);
            return false;
        }
    }
    else
    {//站起
        if (!CanStandUp(pPlayer))
        {
            LOG_TAB_DEBUG("can standup in gameing");
            return false;
        }
        for (uint8_t i = 0; i < m_vecPlayers.size(); ++i)
        {
            if (m_vecPlayers[i].pPlayer == pPlayer)
            {
                LOG_TAB_DEBUG("standup:room:{}--tb:{},chairID:{},uid:{}", m_pHostRoom->GetRoomID(), GetTableID(), i,
                              pPlayer->GetUID());
                m_vecPlayers[i].Reset();
                AddLooker(pPlayer);
                OnActionUserStandUp(i, pPlayer);
                return true;
            }
        }

    }

    return true;
}

bool CGameTable::ResetPlayerReady() {
    for (auto &seat : m_vecPlayers)
    {
        seat.readyStatus = 0;
        seat.autoStatus = 0;
        seat.readyTime = 0;
        seat.overTimes = 0;
        if (seat.pPlayer)
        {
            seat.pPlayer->SetAutoReady(false);
        }
    }
    SendReadyStateToClient();
    LOG_TAB_DEBUG("Reset Player Ready State");
    return true;
}

// 自动准备
void CGameTable::AutoReadyAll() {
    for (auto &seat:m_vecPlayers)
    {
        if (seat.pPlayer == nullptr)continue;
        if (seat.readyStatus == 1)continue;
        PlayerReady(seat.pPlayer, true);
    }
}

bool CGameTable::IsAllReady() {
    for (auto &seat : m_vecPlayers)
    {
        if (seat.pPlayer == nullptr || seat.readyStatus == 0)
        {
            return false;
        }
    }
    return true;
}

// 游戏状态
void CGameTable::SetPlayStatus(uint16_t chairID, uint8_t status) {
    if (chairID < m_vecPlayers.size())
    {
        m_vecPlayers[chairID].playStatus = status;
    }
}

uint8_t CGameTable::GetPlayStatus(uint16_t chairID) {
    if (chairID < m_vecPlayers.size())
    {
        return m_vecPlayers[chairID].playStatus;
    }
    return 0;
}

void CGameTable::ResetPlayStatus(uint8_t status) {
    for_each(m_vecPlayers.begin(), m_vecPlayers.end(), [&](auto &seat)
    {
        seat.playStatus = status;
    });
}

bool CGameTable::PlayerSetAuto(std::shared_ptr<CGamePlayer> pPlayer, uint8_t autoType) {
    for (auto &seat : m_vecPlayers)
    {
        if (seat.pPlayer == pPlayer)
        {
            if (seat.autoStatus != autoType)
            {
                seat.autoStatus = (autoType == 1) ? 1 : 0;
                seat.overTimes = 0;
                SendReadyStateToClient();
            }
            return true;
        }
    }
    return false;
}

bool CGameTable::IsReady(std::shared_ptr<CGamePlayer> pPlayer) {
    for (auto &seat : m_vecPlayers)
    {
        if (seat.pPlayer == pPlayer)
        {
            return (seat.readyStatus == 1);
        }
    }
    return false;
}

bool CGameTable::IsExistPlayer(uint32_t uid) {
    for (auto &seat : m_vecPlayers)
    {
        if (seat.pPlayer != nullptr && seat.pPlayer->GetUID() == uid)
        {
            return true;
        }
    }
    return false;
}

bool CGameTable::IsFullTable() {
    for (uint8_t i = 0; i < m_vecPlayers.size() && i < m_conf.seatNum; ++i)
    {
        if (m_vecPlayers[i].pPlayer == nullptr)
            return false;
    }
    return true;
}

uint32_t CGameTable::GetPlayerNum() {
    uint32_t num = 0;
    for_each(m_vecPlayers.begin(), m_vecPlayers.end(), [&](stSeat &seat)mutable
    {
        if (seat.pPlayer != nullptr)
            num++;
    });
    return num;
}

uint32_t CGameTable::GetOnlinePlayerNum() {
    uint32_t num = 0;
    for_each(m_vecPlayers.begin(), m_vecPlayers.end(), [&](stSeat &seat)mutable
    {
        if (seat.pPlayer != nullptr && seat.pPlayer->GetPlayerType() == PLAYER_TYPE_ONLINE)
        {
            num++;
        }
    });
    return num;
}

uint32_t CGameTable::GetReadyNum() {
    uint32_t num = 0;
    for_each(m_vecPlayers.begin(), m_vecPlayers.end(), [&](stSeat &seat)mutable
    {
        if (seat.pPlayer != nullptr && seat.readyStatus == 1)
            num++;
    });
    return num;
}

uint32_t CGameTable::GetNoReadyNum() {
    uint32_t num = 0;
    for_each(m_vecPlayers.begin(), m_vecPlayers.end(), [&](stSeat &seat)mutable
    {
        if (seat.pPlayer != nullptr && seat.readyStatus == 0)
            num++;
    });
    return num;
}

bool CGameTable::IsEmptyTable() {
    for (auto &seat : m_vecPlayers)
    {
        if (seat.pPlayer != nullptr)
            return false;
    }
    if (!m_mpLookers.empty())
        return false;

    return true;
}

bool CGameTable::IsAllDisconnect(uint32_t disconnectTime) {
    for (auto &seat : m_vecPlayers)
    {
        auto pPlayer = seat.pPlayer;
        if (pPlayer == nullptr)continue;
        if (pPlayer->GetDisconnectTime() <= disconnectTime)
            return false;
    }
    return true;
}

bool CGameTable::IsDisconnect(uint16_t chairID) {
    if (chairID >= m_vecPlayers.size())return true;
    if (m_vecPlayers[chairID].pPlayer == nullptr)return true;

    return m_vecPlayers[chairID].pPlayer->GetNetState() == 0;
}

void CGameTable::SetGameState(uint8_t state) {
    m_gameState = state;
}

uint8_t CGameTable::GetGameState() {
    return m_gameState;
}

std::shared_ptr<CGamePlayer> CGameTable::GetPlayer(uint16_t chairID) {
    if (chairID < m_vecPlayers.size())
        return m_vecPlayers[chairID].pPlayer;

    return nullptr;
}

uint16_t CGameTable::GetChairID(std::shared_ptr<CGamePlayer> pPlayer) {
    for (uint8_t i = 0; i < m_vecPlayers.size(); ++i)
    {
        if (m_vecPlayers[i].pPlayer == pPlayer)
        {
            return i;
        }
    }
    LOG_TAB_ERROR("not this table:{}", pPlayer->GetUID());
    return 0xFF;
}

int64_t CGameTable::GetBaseScore() {
    return m_conf.baseScore;
}

int64_t CGameTable::GetEnterMin() {
    return m_conf.enterMin;
}

int64_t CGameTable::GetEnterMax() {
    return m_conf.enterMax;
}

uint8_t CGameTable::GetPlayType() {
    return m_conf.playType;
}

void CGameTable::SetTableConf(stTableConf &conf) {
    m_conf = conf;
}

const stTableConf &CGameTable::GetTableConf() {
    return m_conf;
}

int64_t CGameTable::GetPlayerCurScore(std::shared_ptr<CGamePlayer> pPlayer) {
    if (pPlayer == nullptr)
        return 0;
    return pPlayer->GetAccountValue(emACC_VALUE_COIN);
}

// 是否可为负
bool CGameTable::CanMinus() {
    return false;
}

// 分赃算法
stDevide CGameTable::GetDevide(uint16_t chairID, int64_t winScore, bool isBanker) {
    std::shared_ptr<CGamePlayer> pPlayer = GetPlayer(chairID);

    stDevide devide;
    devide.chairID = chairID;
    devide.isBanker = isBanker;
    devide.winScore = winScore;
    devide.curScore = GetPlayerCurScore(pPlayer);
    LOG_TAB_DEBUG("get devide:chairid {}-- winScore {}-- isBanker {},curScore {}", chairID, winScore, isBanker,
                  devide.curScore);
    return devide;
}

void CGameTable::DevideByWeight(vector<stDevide> &players, bool isHaveBanker) {
    //计算可以输的总分，输光为止
    int64_t loseScore = 0;
    int64_t winScore = 0;
    uint16_t bankerPos = 0;
    //如果有庄家，先扣除输的闲家到庄家身上
    if (isHaveBanker)
    {
        for (uint16_t i = 0; i < players.size(); ++i)
        {
            if (players[i].isBanker)
            {
                bankerPos = i;
            }
        }
        for (auto &p:players)
        {//庄家收钱
            if (!p.isBanker)
            {
                if (p.winScore < 0)
                {// 闲家输
                    if ((p.winScore + p.curScore) >= 0)
                    {// 够输
                        p.realWin = p.winScore;
                    }
                    else
                    {// 不够输
                        p.realWin = -p.curScore;
                    }
                    loseScore += abs(p.realWin);
                }
                if (p.winScore > 0)
                {//赢的钱
                    winScore += p.winScore;
                }
            }
        }
        //庄家分钱(身上钱加上收款)
        if ((players[bankerPos].curScore + loseScore) > winScore)
        {// 够输
            LOG_TAB_DEBUG("庄家够赔付 cur {}--- lose {}--- win {}", players[bankerPos].curScore, loseScore, winScore);
            for (auto &p:players)
            {
                if (!p.isBanker)
                {
                    if (p.winScore > 0)
                    {
                        p.realWin = p.winScore;
                    }
                    players[bankerPos].realWin += (-p.realWin);
                }
            }
        }
        else
        {
            LOG_TAB_DEBUG("庄家不够赔付,均等分钱 cur {}--- lose {}--- win {}", players[bankerPos].curScore, loseScore, winScore);
            players[bankerPos].realWin = -players[bankerPos].curScore;
            auto totalScore = players[bankerPos].curScore + loseScore;
            auto tmpScore = totalScore;
            for (auto &p:players)
            {//赢的闲家分钱
                if (p.winScore > 0 && !p.isBanker)
                {
                    p.realWin = p.winScore * totalScore / winScore;
                    tmpScore -= p.realWin;
                }
            }
            //多出来的
            if (tmpScore != 0)
            {
                for (auto &p:players)
                {//赢的闲家分钱
                    if (p.winScore > 0 && !p.isBanker)
                    {
                        p.realWin += tmpScore;
                        break;
                    }
                }
            }
        }
    }
    else
    {//没有庄家，先算输的玩家
        for (auto &p:players)
        {//收钱
            if (p.winScore < 0)
            {// 闲家输
                if ((p.winScore + p.curScore) >= 0)
                {// 够输
                    p.realWin = p.winScore;
                }
                else
                {// 不够输
                    p.realWin = -p.curScore;
                }
                loseScore += abs(p.realWin);
            }
            if (p.winScore > 0)
            {//赢的钱
                winScore += p.winScore;
            }
        }
        //分钱
        if (winScore == loseScore)
        {
            LOG_TAB_DEBUG("输家够支付{}---{}", winScore, loseScore);
            for (auto &p:players)
            {
                if (p.winScore > 0)
                {
                    p.realWin = p.winScore;
                }
            }
        }
        else
        {
            LOG_TAB_DEBUG("输家不够支付,输光为止{}---{}", winScore, loseScore);
            auto tmpScore = loseScore;
            for (auto &p:players)
            {//赢的分钱
                if (p.winScore > 0)
                {
                    p.realWin = p.winScore * loseScore / winScore;
                    tmpScore -= p.realWin;
                }
            }
            //多出来的
            if (tmpScore != 0)
            {
                for (auto &p:players)
                {//赢的闲家分钱
                    if (p.winScore > 0)
                    {
                        p.realWin += tmpScore;
                        break;
                    }
                }
            }
        }
    }
    for (auto &p:players)
    {
        LOG_TAB_DEBUG("分钱结果:chair {} cur {}--win {}-- realWin {}", p.chairID, p.curScore, p.winScore, p.realWin);
    }
}

void CGameTable::DevideByOrder(vector<stDevide> &players, bool isHaveBanker) {
    //计算可以输的总分，输光为止
    int64_t loseScore = 0;
    int64_t winScore = 0;
    uint16_t bankerPos = 0;
    //如果有庄家，先扣除输的闲家到庄家身上
    if (isHaveBanker)
    {
        for (uint16_t i = 0; i < players.size(); ++i)
        {
            if (players[i].isBanker)
            {
                bankerPos = i;
            }
        }
        for (auto &p:players)
        {//庄家收钱
            if (!p.isBanker)
            {
                if (p.winScore < 0)
                {// 闲家输
                    if ((p.winScore + p.curScore) >= 0)
                    {// 够输
                        p.realWin = p.winScore;
                    }
                    else
                    {// 不够输
                        p.realWin = -p.curScore;
                    }
                    loseScore += abs(p.realWin);
                }
                if (p.winScore > 0)
                {//赢的钱
                    winScore += p.winScore;
                }
            }
        }
        //庄家分钱(身上钱加上收款)
        if ((players[bankerPos].curScore + loseScore) > winScore)
        {// 够输
            LOG_TAB_DEBUG("庄家够赔付 cur {}--- lose {}--- win {}", players[bankerPos].curScore, loseScore, winScore);
            for (auto &p:players)
            {
                if (!p.isBanker)
                {
                    if (p.winScore > 0)
                    {
                        p.realWin = p.winScore;
                    }
                    players[bankerPos].realWin += (-p.realWin);
                }
            }
        }
        else
        {
            LOG_TAB_DEBUG("庄家不够赔付,按顺序分钱 cur {}--- lose {}--- win {}", players[bankerPos].curScore, loseScore, winScore);
            players[bankerPos].realWin = -players[bankerPos].curScore;
            auto totalScore = players[bankerPos].curScore + loseScore;
            auto tmpScore = totalScore;
            for (auto &p:players)
            {//赢的闲家分钱
                if (!p.isBanker)
                {
                    if (p.winScore > 0)
                    {
                        if (tmpScore > p.winScore)
                        {
                            p.realWin = p.winScore;
                        }
                        else
                        {
                            p.realWin = tmpScore;
                        }
                        tmpScore -= p.realWin;
                        if (tmpScore <= 0)break;
                    }
                }
            }
        }
    }
    else
    {//没有庄家，先算输的玩家
        for (auto &p:players)
        {//收钱
            if (p.winScore < 0)
            {// 闲家输
                if ((p.winScore + p.curScore) >= 0)
                {// 够输
                    p.realWin = p.winScore;
                }
                else
                {// 不够输
                    p.realWin = -p.curScore;
                }
                loseScore += abs(p.realWin);
            }
            if (p.winScore > 0)
            {//赢的钱
                winScore += p.winScore;
            }
        }
        //分钱
        if (winScore == loseScore)
        {
            LOG_TAB_DEBUG("输家够支付{}---{}", winScore, loseScore);
            for (auto &p:players)
            {
                if (p.winScore > 0)
                {
                    p.realWin = p.winScore;
                }
            }
        }
        else
        {
            LOG_TAB_DEBUG("输家不够支付,输光为止{}---{}", winScore, loseScore);
            auto tmpScore = loseScore;
            for (auto &p:players)
            {//赢的分钱
                if (p.winScore > 0)
                {
                    if (tmpScore > p.winScore)
                    {
                        p.realWin = p.winScore;
                    }
                    else
                    {
                        p.realWin = tmpScore;
                    }
                    tmpScore -= p.realWin;
                    if (tmpScore <= 0)break;
                }
            }
        }
    }
    for (auto &p:players)
    {
        LOG_TAB_DEBUG("分钱结果:chair {} cur {}--win {}-- realWin {}", p.chairID, p.curScore, p.winScore, p.realWin);
    }
}

void CGameTable::TableMsgToLooker(const google::protobuf::Message *msg, uint16_t msg_type) {
    for (auto &it : m_mpLookers)
    {
        std::shared_ptr<CGamePlayer> pPlayer = it.second;
        pPlayer->SendMsgToClient(msg, msg_type);
    }
}

void CGameTable::TableMsgToPlayer(const google::protobuf::Message *msg, uint16_t msg_type, bool bRecord) {
    for_each(m_vecPlayers.begin(), m_vecPlayers.end(), [&](stSeat &seat)mutable
    {
        if (seat.pPlayer != nullptr)
        {
            seat.pPlayer->SendMsgToClient(msg, msg_type);
        }
    });
    if (m_openRecord && bRecord)
    {
        PushRecordGameMsg(msg, msg_type);
    }
}

void CGameTable::TableMsgToAll(const google::protobuf::Message *msg, uint16_t msg_type, bool bRecord) {
    TableMsgToPlayer(msg, msg_type, bRecord);
    TableMsgToLooker(msg, msg_type);
}

void CGameTable::TableMsgToClient(uint16_t chairID, const google::protobuf::Message *msg, uint16_t msg_type, bool bRecord) {
    auto pGamePlayer = GetPlayer(chairID);
    if (pGamePlayer != nullptr)
    {
        pGamePlayer->SendMsgToClient(msg, msg_type);
        if (m_openRecord && bRecord)
        {
            PushRecordGameMsg(msg, msg_type, pGamePlayer->GetUID());
        }
    }
}

void CGameTable::SendTableInfoToClient(std::shared_ptr<CGamePlayer> pPlayer) {
    net::cli::msg_table_info_rep msg;
    // 桌子信息
    net::table_info *pTableInfo = msg.mutable_table_info();
    GetTableFaceInfo(pTableInfo);
    pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TABLE_INFO);
}

void CGameTable::SendReadyStateToClient() {
    net::cli::msg_table_ready_rep msg;
    GetReadyInfo(msg);
    TableMsgToAll(&msg, S2C_MSG_TABLE_READY_REP);
}

void CGameTable::SendSeatInfoToClient(std::shared_ptr<CGamePlayer> pGamePlayer) {
    net::cli::msg_seat_info_rep msg;
    GetSeatInfo(msg);
    if (pGamePlayer != nullptr)
    {
        pGamePlayer->SendMsgToClient(&msg, net::S2C_MSG_SEATS_INFO);
    }
    else
    {
        TableMsgToAll(&msg, net::S2C_MSG_SEATS_INFO);
    }
}

void CGameTable::FlushSeatValueInfoToClient(bool bShowChange) {
    net::cli::msg_seat_value_info_rep msg;
    msg.set_show_change(bShowChange ? 1 : 0);
    // 座位玩家信息
    for (uint8_t i = 0; i < m_vecPlayers.size(); ++i)
    {
        net::seat_value_info *pSeat = msg.add_players();
        pSeat->set_chairid(i);
        std::shared_ptr<CGamePlayer> pPlayer = m_vecPlayers[i].pPlayer;
        if (pPlayer != nullptr)
        {
            pSeat->set_uid(pPlayer->GetUID());
            pSeat->set_coin(pPlayer->GetAccountValue(emACC_VALUE_COIN));
        }
        else
        {
            pSeat->set_uid(0);
            pSeat->set_coin(0);
        }
    }
    TableMsgToAll(&msg, net::S2C_MSG_SEAT_VALUE_INFO_REP);
}

void CGameTable::SendLookerListToClient(std::shared_ptr<CGamePlayer> pGamePlayer) {
    uint32_t sendNum = 0;
    net::cli::msg_looker_list_info_rep msg;
    msg.set_is_reset(1);

    for (auto &it : m_mpLookers)
    {
        net::looker_info *pInfo = msg.add_lookers();
        auto pPlayer = it.second;
        pInfo->set_uid(pPlayer->GetUID());

        pInfo->set_coin(pPlayer->GetAccountValue(emACC_VALUE_COIN));

        sendNum++;
        if (sendNum > 20)
        {
            if (pGamePlayer != nullptr)
            {
                pGamePlayer->SendMsgToClient(&msg, net::S2C_MSG_LOOKER_LIST_INFO);
            }
            else
            {
                TableMsgToAll(&msg, net::S2C_MSG_LOOKER_LIST_INFO);
            }
            sendNum = 0;
            msg.set_is_reset(0);
        }
    }
    if (sendNum > 0)
    {
        if (pGamePlayer != nullptr)
        {
            pGamePlayer->SendMsgToClient(&msg, net::S2C_MSG_LOOKER_LIST_INFO);
        }
        else
        {
            TableMsgToAll(&msg, net::S2C_MSG_LOOKER_LIST_INFO);
        }
    }
}

void CGameTable::NotifyPlayerJoin(std::shared_ptr<CGamePlayer> pPlayer, bool isJoin) {
    net::cli::msg_notify_player_join_rep msg;
    msg.set_join_leave(isJoin);

    net::looker_info *pSeat = msg.mutable_player();
    pSeat->set_uid(pPlayer->GetUID());
    pSeat->set_coin(pPlayer->GetAccountValue(emACC_VALUE_COIN));

    TableMsgToAll(&msg, net::S2C_MSG_NOTIFY_PLAYER_JOIN);
}

void CGameTable::NotifyPlayerLeave(std::shared_ptr<CGamePlayer> pPlayer, uint8_t leaveType) {
    net::cli::msg_leave_table_rep rep;
    rep.set_result(1);
    rep.set_leave_type(leaveType);
    pPlayer->SendMsgToClient(&rep, net::S2C_MSG_LEAVE_TABLE_REP);
}

void CGameTable::ResetInitSeat(uint8_t seatNum) {
    m_vecPlayers.resize(seatNum);
    for (uint8_t i = 0; i < seatNum; ++i)
    {
        m_vecPlayers[i].Reset();
    }
}

void CGameTable::SitDown(stSeat &seat, std::shared_ptr<CGamePlayer> pPlayer) {
    seat.pPlayer = pPlayer;
    seat.readyStatus = 0;
    seat.autoStatus = 0;
    seat.readyTime = time::getSysTime();
    seat.overTimes = 0;
    seat.playStatus = 0;
}

void CGameTable::GetSeatInfo(net::cli::msg_seat_info_rep &msg) {
    // 座位玩家信息
    for (uint8_t i = 0; i < m_vecPlayers.size(); ++i)
    {
        net::seat_info *pSeat = msg.add_players();
        pSeat->set_chairid(i);
        std::shared_ptr<CGamePlayer> pPlayer = m_vecPlayers[i].pPlayer;
        if (pPlayer != nullptr)
        {
            pSeat->set_uid(pPlayer->GetUID());
            pSeat->set_coin(pPlayer->GetAccountValue(emACC_VALUE_COIN));
            pSeat->set_login_ip(pPlayer->GetIP());
            pSeat->set_net_state(pPlayer->GetNetState());
            pSeat->set_lon(pPlayer->GetLon());
            pSeat->set_lat(pPlayer->GetLat());

        }
        else
        {
            pSeat->set_uid(0);
            pSeat->set_coin(0);
            pSeat->set_login_ip(0);
            pSeat->set_net_state(0);
            pSeat->set_lon(0);
            pSeat->set_lat(0);

        }
    }
}

void CGameTable::GetReadyInfo(net::cli::msg_table_ready_rep &msg) {
    for (auto &seat : m_vecPlayers)
    {
        msg.add_readys(seat.readyStatus);
        msg.add_auto_states(seat.autoStatus);
    }
}

void CGameTable::GetTableFaceBaseInfo(net::table_info *pBaseInfo) {
    pBaseInfo->set_tableid(GetTableID());
    pBaseInfo->set_basescore(m_conf.baseScore);
    pBaseInfo->set_feetype(m_conf.feeType);
    pBaseInfo->set_feevalue(m_conf.feeValue);
    pBaseInfo->set_table_state(GetGameState());
    pBaseInfo->set_seat_num(m_conf.seatNum);
    pBaseInfo->set_game_type(m_pHostRoom->GetGameType());
    pBaseInfo->set_play_type(GetPlayType());
    pBaseInfo->set_add_param(m_conf.addParam);
    for (uint8_t i = 0; i < m_conf.seatNum && i < m_vecPlayers.size(); ++i)
    {
        net::seat_face *pSeat = pBaseInfo->add_seats();
        pSeat->set_chairid(i);
        pSeat->set_ready(m_vecPlayers[i].readyStatus);
        pSeat->set_uid(m_vecPlayers[i].pPlayer ? m_vecPlayers[i].pPlayer->GetUID() : 0);
    }

}

// 牌局日志
void CGameTable::InitBlingLog(bool bNeedReady) {
    m_chessid = CStringUtility::FormatToString("%lld%lld", GetTableID(), time::getSysTime());
    LOG_TAB_DEBUG("初始化牌局日志信息:{}", m_chessid);
    m_blingLog.Reset();
    m_blingLog.baseScore = m_conf.baseScore;
    m_blingLog.startTime = time::getSysTime();
    m_blingLog.gameType = m_pHostRoom->GetGameType();
    m_blingLog.roomType = 0;
    m_blingLog.tableID = GetTableID();
    m_blingLog.chessid = m_chessid;
    m_blingLog.roomID = m_pHostRoom->GetRoomID();

    for (uint16_t i = 0; i < m_vecPlayers.size(); ++i)
    {
        stBlingUser user;
        std::shared_ptr<CGamePlayer> pPlayer = m_vecPlayers[i].pPlayer;
        if (pPlayer == nullptr || (bNeedReady && m_vecPlayers[i].readyStatus == 0))
            continue;
        user.uid = pPlayer->GetUID();
        user.oldValue = GetPlayerCurScore(pPlayer);
        user.chairid = i;
        m_blingLog.users.push_back(user);
    }
    m_operLog.clear();
    InitRecordGameMsg();
}

void CGameTable::ChangeUserBlingLog(std::shared_ptr<CGamePlayer> pPlayer, int64_t winScore) {
    if (pPlayer == nullptr)return;
    for (uint32_t i = 0; i < m_blingLog.users.size(); ++i)
    {
        if (m_blingLog.users[i].uid == pPlayer->GetUID())
        {
            m_blingLog.users[i].win += winScore;
            m_blingLog.users[i].newValue = GetPlayerCurScore(pPlayer);
            return;
        }
    }
}

void CGameTable::ChangeUserBlingLogFee(uint32_t uid, int64_t fee) {
    for (uint32_t i = 0; i < m_blingLog.users.size(); ++i)
    {
        if (m_blingLog.users[i].uid == uid)
        {
            m_blingLog.users[i].fee += fee;
            return;
        }
    }
}

void CGameTable::SaveBlingLog() {
    SaveRecordGameMsg();

    m_blingLog.endTime = time::getSysTime();
    m_blingLog.operLog << m_operLog.dump();
    CDBMysqlMgr::Instance().WriteGameBlingLog(m_blingLog);

}

// 操作日志
void CGameTable::WriteHandleCardLog(uint16_t chairID, uint8_t cardData[], uint8_t cardCount, uint8_t cardType, int64_t score) {
    json logValue;
    logValue["p"] = chairID;
    logValue["s"] = score;
    logValue["ct"] = cardType;
    for (uint32_t i = 0; i < cardCount; ++i)
    {
        logValue["c"].push_back(cardData[i]);
    }
    m_operLog["card"].push_back(logValue);
}

void CGameTable::WriteOutCardLog(uint16_t chairID, uint8_t cardData[], uint8_t cardCount, uint8_t eFlag) {
    json logValue;
    logValue["p"] = chairID;
    logValue["e"] = eFlag;
    for (uint32_t i = 0; i < cardCount; ++i)
    {
        logValue["c"].push_back(cardData[i]);
    }
    m_operLog["outcard"].push_back(logValue);
}

void CGameTable::WriteBankerLog(uint16_t chairID) {
    m_operLog["banker"] = chairID;
}

// 牌局录像
void CGameTable::InitRecordGameMsg() {
    if (!m_needOpenRecord)
    {
        m_openRecord = false;
        return;
    }
    m_gameRecord.Clear();
    m_openRecord = true;
    m_gameRecord.set_game_type(m_pHostRoom->GetGameType());
    m_gameRecord.set_play_type(GetPlayType());
    m_gameRecord.set_start_time(time::getSysTime());
    //桌子信息
    net::cli::msg_table_info_rep tblinfo_msg;
    net::table_info *pTableInfo = tblinfo_msg.mutable_table_info();
    GetTableFaceInfo(pTableInfo);
    PushRecordGameMsg(&tblinfo_msg, net::S2C_MSG_TABLE_INFO);
    //座位信息
    net::cli::msg_seat_info_rep seat_msg;
    GetSeatInfo(seat_msg);
    PushRecordGameMsg(&seat_msg, net::S2C_MSG_SEATS_INFO);
    //准备信息
    net::cli::msg_table_ready_rep ready_msg;
    GetReadyInfo(ready_msg);
    PushRecordGameMsg(&ready_msg, net::S2C_MSG_TABLE_READY_REP);
    LOG_TAB_DEBUG("初始化牌局记录");
}

void CGameTable::PushRecordGameMsg(const google::protobuf::Message *msg, uint16_t msg_type, uint32_t uid) {
    net::record_game_msg *record = m_gameRecord.add_msgs();
    record->set_msg_type(msg_type);
    record->set_uid(uid);
    record->set_send_time(time::getSysTime());
    string sstr;
    msg->SerializeToString(&sstr);
    record->set_msg(sstr);

}

void CGameTable::SaveRecordGameMsg() {
    if (!m_needOpenRecord)
    {
        m_openRecord = false;
        return;
    }
    string sstr, outStr;
    m_gameRecord.SerializeToString(&sstr);
    LOG_TAB_DEBUG("保存录像的大小:{}", sstr.length());
    //压缩 toney
    snappy::Compress(sstr.c_str(), sstr.length(), &outStr);
    LOG_TAB_DEBUG("压缩后的录像长度:{}", outStr.length());

    //CRedisMgr::Instance().SetRecordGameMsg(m_chessid, outStr);

}












