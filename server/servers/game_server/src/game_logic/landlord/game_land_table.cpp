//
// Created by toney on 16/4/6.
//
#include "game_define.h"
#include "data_cfg_mgr.h"
#include "game_land_table.h"
#include "game_table/game_room.h"
#include "nlohmann/json_wrap.h"
#include "land_logic_msg.pb.h"
#include "error_code.pb.h"

using namespace std;
using namespace svrlib;
using namespace net;

namespace game_land {
    namespace {
        const static int32_t s_ReadyTimeOut = 20;

    };

// 斗地主游戏桌子
    CGameLandTable::CGameLandTable(std::shared_ptr<CGameRoom> pRoom, int64_t tableID)
            : CGameCoinTable(pRoom, tableID) {
        m_vecPlayers.clear();

        //炸弹变量
        m_firstUser = INVALID_CHAIR;
        m_bankerUser = INVALID_CHAIR;
        m_curUser = INVALID_CHAIR;
        memset(m_outCardCount, 0, sizeof(m_outCardCount));
        m_firstUserType = 1;
        m_maxBeiShu = 32;
        //游戏变量
        m_bombCount = 0;
        memset(m_eachBombCount, 0, sizeof(m_eachBombCount));

        //叫分信息
        memset(m_callScore, 0, sizeof(m_callScore));
        m_callCount = 0;

        //出牌信息
        m_turnCardCount = 0;
        m_turnWiner = INVALID_CHAIR;
        memset(m_turnCardData, 0, sizeof(m_turnCardData));

        //扑克信息
        memset(m_bankerCard, 0, sizeof(m_bankerCard));
        memset(m_handCardData, 0, sizeof(m_handCardData));
        memset(m_handCardCount, 0, sizeof(m_handCardCount));

    }

    CGameLandTable::~CGameLandTable() {

    }

    void CGameLandTable::GetTableFaceInfo(net::table_info *pInfo) {
        GetTableFaceBaseInfo(pInfo);
        //附加信息
        pInfo->set_show_hand_num(1);
        pInfo->set_call_time(GetCallScoreTime());
        pInfo->set_card_time(GetOutCardTime());
    }

//配置桌子
    bool CGameLandTable::Init() {
        SetGameState(net::TABLE_STATE_FREE);
        ResetInitSeat(GAME_LAND_PLAYER);
        return true;
    }

//复位桌子
    void CGameLandTable::ResetTable() {
        CGameCoinTable::ResetTable();
        ResetGameData();
        SetGameState(TABLE_STATE_FREE);
        ResetPlayerReady();
        ReInitPoker();
    }

    void CGameLandTable::OnTimeTick() {
        if (m_coolLogic.isTimeOut())
        {
            uint8_t tableState = GetGameState();
            switch (tableState)
            {
                case TABLE_STATE_FREE:
                {
                    if (IsAllReady())
                    {
                        ResetPlayStatus(1);
                        OnGameStart();
                    }
                }
                    break;
                case TABLE_STATE_CALL:
                {
                    OnCallScoreTimeOut();
                }
                    break;
                case TABLE_STATE_PLAY:
                {
                    OnOutCardTimeOut();
                }
                    break;
                case TABLE_STATE_GAME_END:
                {
                    m_coolLogic.beginCooling(4000);
                    SetGameState(TABLE_STATE_FREE);
                    OnGameRoundFlush();
                }
                    break;
                default:
                    break;
            }
        }
        if (GetGameState() == TABLE_STATE_CALL)
        {
            auto pPlayer = m_vecPlayers[m_curUser].pPlayer;
            if (pPlayer != nullptr && pPlayer->IsRobot() && m_coolRobot.isTimeOut())
            {
                OnRobotCallScore();
            }
        }
        if (GetGameState() == TABLE_STATE_PLAY)
        {
            if (m_vecPlayers[m_curUser].overTimes == 2)
            {
                m_vecPlayers[m_curUser].autoStatus = 1;
                SendReadyStateToClient();
            }
            auto pPlayer = m_vecPlayers[m_curUser].pPlayer;
            if (pPlayer != nullptr && pPlayer->IsRobot() && m_coolRobot.isTimeOut())
            {
                OnRobotOutCard();
                return;
            }
            if (m_vecPlayers[m_curUser].autoStatus == 1 && m_coolLogic.getPassTick() > 1000)
            {
                OnUserAutoCard();
            }
        }
    }

// 游戏消息
    int CGameLandTable::OnGameMessage(std::shared_ptr<CGamePlayer> pPlayer, uint16_t cmdID, const uint8_t *pkt_buf, uint16_t buf_len) {
        uint16_t chairID = GetChairID(pPlayer);
        CHECK_LOG_RET(chairID != 0xFF, 0, "玩家不在座位上:{}", pPlayer->GetUID());
        switch (cmdID)
        {
            case net::C2S_MSG_LAND_CALL_SCORE_REQ:// 用户叫分
            {
                net::msg_land_call_score_req msg;
                PARSE_MSG_FROM_ARRAY(msg);
                //状态效验
                if (GetGameState() != TABLE_STATE_CALL)
                {
                    LOG_TAB_DEBUG("not call state");
                    return 0;
                }
                //消息处理
                return OnUserCallScore(chairID, msg.call_score());
            }
                break;
            case net::C2S_MSG_LAND_OUT_CARD_REQ:// 用户出牌
            {
                net::msg_land_out_card_req msg;
                PARSE_MSG_FROM_ARRAY(msg);
                //状态效验
                if (GetGameState() != TABLE_STATE_PLAY)
                {
                    LOG_TAB_DEBUG("not play state");
                    return 0;
                }
                //消息处理
                if (msg.card_data_size() > MAX_LAND_COUNT)
                {
                    LOG_TAB_ERROR("the card is more 20:{}", chairID);
                    return 0;
                }
                uint8_t cardCount = 0;
                uint8_t cardData[MAX_LAND_COUNT];
                memset(cardData, 0, sizeof(cardData));
                for (uint8_t i = 0; i < msg.card_data_size(); ++i)
                {
                    cardData[i] = msg.card_data(i);
                    cardCount++;
                }
                return OnUserOutCard(chairID, cardData, cardCount);
            }
                break;
            case net::C2S_MSG_LAND_PASS_CARD_REQ:// 用户放弃
            {
                net::msg_land_pass_card_req msg;
                PARSE_MSG_FROM_ARRAY(msg);
                //状态效验
                if (GetGameState() != TABLE_STATE_PLAY)
                {
                    LOG_TAB_DEBUG("not play state");
                    return 0;
                }
                //消息处理
                return OnUserPassCard(chairID);
            }
                break;
            case net::C2S_MSG_LAND_REQ_HAND_CARD:// 请求手牌
            {
                net::msg_land_req_hand_card_req msg;
                PARSE_MSG_FROM_ARRAY(msg);
                SendHandleCard(chairID);
                return 0;
            }
                break;
            default:
                return 0;
        }
        return 0;
    }

// 游戏开始
    bool CGameLandTable::OnGameStart() {
        LOG_TAB_DEBUG("game start");
        // 出牌信息
        m_turnCardCount = 0;
        m_turnWiner = INVALID_CHAIR;
        memset(m_turnCardData, 0, sizeof(m_turnCardData));

        SetGameState(net::TABLE_STATE_CALL);

        InitBlingLog(true);

        // 混淆扑克
        ShuffleTableCard();

        // 抽取明牌
        uint8_t validCardData = 0;
        uint8_t validCardIndex = 0;
        uint16_t startUser = m_firstUser;
        m_curUser = m_firstUser;

        //抽取扑克
        validCardIndex = rand_range(0, DISPATCH_COUNT);
        validCardData = m_randCard[validCardIndex];

        //设置用户
        startUser = m_gameLogic.GetCardValue(validCardData) % GAME_LAND_PLAYER;
        m_curUser = DispatchUserCard(startUser, validCardIndex);
        //排列扑克
        for (uint16_t i = 0; i < GAME_LAND_PLAYER; i++)
        {
            m_gameLogic.SortCardList(m_handCardData[i], m_handCardCount[i], ST_ORDER);
        }
        //设置底牌
        memcpy(m_bankerCard, &m_randCard[DISPATCH_COUNT], sizeof(m_bankerCard));

        //设置用户
        if (m_callCount > 0 && m_firstUser != INVALID_CHAIR)
        {
            m_curUser = m_firstUser;
            LOG_TAB_DEBUG("重新叫分:{}--{}", m_curUser, m_callCount);
        }
        else
        {
            if (m_firstUserType == 2 || m_firstUser == INVALID_CHAIR)
            {
                LOG_TAB_DEBUG("随机先叫:{}--{}", m_curUser, m_firstUser);
                m_firstUser = m_curUser;
            }
            else
            {
                LOG_TAB_DEBUG("先出先叫:{}", m_firstUser);
                m_curUser = m_firstUser;
            }
        }
        LOG_TAB_DEBUG("callCount:{} curuser:{}--firstuser:{}", m_callCount, m_curUser, m_firstUser);
        //发送数据
        for (uint16_t i = 0; i < GAME_LAND_PLAYER; i++)
        {
            //构造消息
            net::msg_land_start_rep gameStart;
            gameStart.set_start_user(startUser);
            gameStart.set_cur_user(m_curUser);
            gameStart.set_valid_card_data(validCardData);
            gameStart.set_valid_card_index(validCardIndex);

            for (uint8_t j = 0; j < m_handCardCount[i]; ++j)
            {
                gameStart.add_card_data(m_handCardData[i][j]);
            }
            //发送数据
            TableMsgToClient(i, &gameStart, net::S2C_MSG_LAND_START);
        }

        // 启动叫分定时器
        m_coolLogic.beginCooling(GetCallScoreTime());

        //OnSubGameStart();
        return true;
    }

//游戏结束
    bool CGameLandTable::OnGameEnd(uint16_t chairID, uint8_t reason) {
        LOG_TAB_DEBUG("game end:{}--{},banker {}", chairID, reason, m_bankerUser);
        m_coolLogic.clearCool();
        switch (reason)
        {
            case GER_NORMAL:        //常规结束
            {
                net::msg_land_game_over_rep gameOver;
                // 游戏积分计算

                // 炸弹信息
                gameOver.set_bomb_count(m_bombCount);
                for (uint8_t i = 0; i < GAME_LAND_PLAYER; ++i)
                {
                    gameOver.add_each_bomb_counts(m_eachBombCount[i]);
                }
                //用户扑克
                for (uint16_t i = 0; i < GAME_LAND_PLAYER; i++)
                {
                    //拷贝扑克
                    gameOver.add_card_counts(m_handCardCount[i]);
                    for (uint8_t j = 0; j < m_handCardCount[i]; ++j)
                    {
                        gameOver.add_hand_card_data(m_handCardData[i][j]);
                    }
                    WriteOutCardLog(i, m_handCardData[i], m_handCardCount[i], 1);
                }
                //炸弹统计
                int64_t scoreTimes = 1;
                for (uint8_t i = 0; i < m_bombCount; i++)
                {
                    scoreTimes *= 2;
                }
                //春天判断
                if (chairID == m_bankerUser)
                {
                    //用户定义
                    uint16_t user1 = (m_bankerUser + 1) % GAME_LAND_PLAYER;
                    uint16_t user2 = (m_bankerUser + 2) % GAME_LAND_PLAYER;
                    gameOver.set_chun_tian(0);

                    //用户判断
                    if ((m_outCardCount[user1] == 0) && (m_outCardCount[user2] == 0))
                    {
                        scoreTimes *= 2;
                        gameOver.set_chun_tian(1);
                    }
                }
                //反春天判断
                if (chairID != m_bankerUser)
                {
                    gameOver.set_fan_chun_tian(0);
                    if (m_outCardCount[m_bankerUser] == 1)
                    {
                        scoreTimes *= 2;
                        gameOver.set_fan_chun_tian(1);
                    }
                }
                //叫分倍数
                gameOver.set_banker_score(m_callScore[m_bankerUser]);
                scoreTimes *= m_callScore[m_bankerUser];

                //调整倍数
                scoreTimes = std::min(scoreTimes, (int64_t) m_maxBeiShu);
                LOG_TAB_DEBUG("结算翻倍数:{}", scoreTimes);

                //统计积分
                int64_t calcScore[GAME_LAND_PLAYER];
                int64_t loseNum = 0;
                memset(calcScore, 0, sizeof(calcScore));
                // 计算输的分数
                for (uint16_t i = 0; i < GAME_LAND_PLAYER; i++)
                {
                    //变量定义
                    int64_t userScore = 0;
                    uint8_t isLand = (i == m_bankerUser) ? 1 : 0;
                    //积分基数
                    if (i == m_bankerUser && m_handCardCount[m_bankerUser] != 0)
                    {
                        userScore = 2;
                    }
                    else if (i != m_bankerUser && m_handCardCount[m_bankerUser] == 0)
                    {
                        userScore = 1;
                    }
                    else
                    {
                        continue;
                    }
                    //计算积分
                    userScore = GetBaseScore() * userScore * scoreTimes;
                    calcScore[i] = -userScore;
                    loseNum += abs(calcScore[i]);
                }
                // 计算赢的分数
                for (uint16_t i = 0; i < GAME_LAND_PLAYER; ++i)
                {
                    if (calcScore[i] == 0)
                    {
                        if (i == m_bankerUser)
                        {
                            calcScore[i] = loseNum;
                        }
                        else
                        {
                            calcScore[i] = loseNum / 2;
                        }
                    }
                }
                if (!CanMinus())
                {
                    LOG_TAB_DEBUG("积分不能为负数，分配分数");
                    vector<stDevide> devides;
                    for (uint16_t i = 0; i < GAME_LAND_PLAYER; ++i)
                    {
                        devides.push_back(GetDevide(i, calcScore[i], i == m_bankerUser));
                    }
                    DevideByWeight(devides, true);
                    for (uint16_t i = 0; i < GAME_LAND_PLAYER; ++i)
                    {
                        calcScore[i] = devides[i].realWin;
                    }
                }
                for (uint16_t i = 0; i < GAME_LAND_PLAYER; ++i)
                {
                    gameOver.add_scores(calcScore[i]);
                    std::shared_ptr<CGamePlayer> pGamePlayer = GetPlayer(i);
                    CalcPlayerGameInfo(pGamePlayer->GetUID(), calcScore[i]);
                }
                //发送数据
                TableMsgToAll(&gameOver, net::S2C_MSG_LAND_GAME_OVER);

                //切换用户
                if (m_firstUserType == 1)
                {
                    LOG_TAB_DEBUG("切换叫分用户:{}", chairID);
                    m_firstUser = chairID;
                }
                //结束游戏
                ResetGameData();

                SetGameState(TABLE_STATE_GAME_END);
                m_coolLogic.beginCooling(1000);

                OnGameRoundOver();
                return true;
            }
                break;
            case GER_DISMISS:        //游戏解散
            {
                LOG_TAB_ERROR("froce dis game");
                for (uint8_t i = 0; i < GAME_LAND_PLAYER; ++i)
                {
                    if (m_vecPlayers[i].pPlayer != nullptr)
                    {
                        LeaveTable(m_vecPlayers[i].pPlayer);
                    }
                }
                ResetTable();
                return true;
            }
                break;
            case GER_USER_LEAVE:        //用户强退
            case GER_NETWORK_ERROR:    //网络中断
            default:
                return true;
        }
        return false;
    }

// 发送手中扑克牌
    void CGameLandTable::SendHandleCard(uint16_t chairID) {
        if (chairID >= GAME_LAND_PLAYER)
            return;
        LOG_TAB_DEBUG("发送手牌:{}", chairID);
        //发送数据
        net::msg_land_hand_card_rep msg;
        msg.set_chair_id(chairID);
        for (uint8_t j = 0; j < m_handCardCount[chairID]; ++j)
        {
            msg.add_card_data(m_handCardData[chairID][j]);
        }

        //发送数据
        TableMsgToClient(chairID, &msg, net::S2C_MSG_LAND_HAND_CARD);
    }

// 用户放弃
    bool CGameLandTable::OnUserPassCard(uint16_t chairID) {
        //效验状态
        if (chairID != m_curUser || m_turnCardCount == 0)
        {
            LOG_TAB_ERROR("not you oper:{}", chairID);
            return false;
        }
        LOG_TAB_DEBUG("玩家pass：{}", chairID);
        //设置变量
        m_curUser = (m_curUser + 1) % GAME_LAND_PLAYER;
        if (m_curUser == m_turnWiner)
        {
            m_turnCardCount = 0;
        }
        uint8_t isOver = m_turnCardCount == 0 ? 1 : 0;

        net::msg_land_pass_card_rep passCard;
        passCard.set_pass_card_user(chairID);
        passCard.set_cur_user(m_curUser);
        passCard.set_turn_over(isOver);

        TableMsgToAll(&passCard, net::S2C_MSG_LAND_PASS_CARD);
        m_coolLogic.beginCooling(GetOutCardTime());

        return true;
    }

// 用户叫分
    bool CGameLandTable::OnUserCallScore(uint16_t chairID, uint8_t score) {
        //效验状态
        if (chairID != m_curUser)
        {
            LOG_TAB_DEBUG("you can't oper call:{}--{}", m_curUser, chairID);
            return false;
        }
        LOG_TAB_DEBUG("玩家叫分:{}--{}", chairID, score);
        //效验参数
        if (score > 3)
            score = 0;

        //设置叫分
        m_callScore[chairID] = score;
        //设置用户
        if (score == 3 || m_firstUser == (chairID + 1) % GAME_LAND_PLAYER)
        {
            if (score != 0)
            {
                m_bankerUser = m_curUser;
            }
            else
            {
                int32_t maxScore = 0;
                for (uint16_t i = 0; i < GAME_LAND_PLAYER; ++i)
                {
                    if (m_callScore[i] > maxScore)
                    {
                        maxScore = m_callScore[i];
                        m_bankerUser = i;
                    }
                }
                LOG_TAB_DEBUG("最大分数玩家:{}--{}", m_bankerUser, maxScore);
            }
            m_curUser = INVALID_CHAIR;
        }
        else
        {
            m_curUser = (chairID + 1) % GAME_LAND_PLAYER;
        }

        net::msg_land_call_score_rep callScore;
        callScore.set_call_user(chairID);
        callScore.set_cur_user(m_curUser);
        callScore.set_call_score(score);
        TableMsgToAll(&callScore, net::S2C_MSG_LAND_CALL_SCORE);

        //无人叫分重新开始
        if (m_firstUser == (chairID + 1) % GAME_LAND_PLAYER)
        {
            LOG_TAB_DEBUG("无人叫分重新开始:firstUser {},bankerUser {}", m_firstUser, m_bankerUser);
            if (m_bankerUser == INVALID_CHAIR)
            {
                if (m_callCount <= 3)
                {
                    ReGameStart();
                    return true;
                }
                else
                {
                    m_bankerUser = m_firstUser;
                    m_callScore[m_bankerUser] = 1;
                    LOG_TAB_DEBUG("第四局第一个玩家坐庄:{}", m_firstUser);
                }
            }
        }
        if (m_bankerUser != INVALID_CHAIR)
        {// 叫地主
            // 设置状态
            SetGameState(TABLE_STATE_PLAY);
            // 开启超时定时器
            m_coolLogic.beginCooling(GetOutCardTime());
            //设置变量
            if (m_bankerUser == INVALID_CHAIR)
                m_bankerUser = m_firstUser;

            //发送底牌
            m_handCardCount[m_bankerUser] += getArrayLen(m_bankerCard);
            memcpy(&m_handCardData[m_bankerUser][NORMAL_COUNT], m_bankerCard, sizeof(m_bankerCard));

            //排列扑克
            m_gameLogic.SortCardList(m_handCardData[m_bankerUser], m_handCardCount[m_bankerUser], ST_ORDER);

            //设置用户
            m_turnWiner = m_bankerUser;
            m_curUser = m_bankerUser;

            OnGameRoundStart();
            WriteBankerLog(m_bankerUser);

            //发送消息
            net::msg_land_banker_info_rep bankerInfo;
            bankerInfo.set_banker_user(m_bankerUser);
            bankerInfo.set_cur_user(m_curUser);
            bankerInfo.set_call_score(m_callScore[m_bankerUser]);

            uint8_t iSize = getArrayLen(m_bankerCard);
            for (uint8_t i = 0; i < iSize; ++i)
            {
                bankerInfo.add_banker_card(m_bankerCard[i]);
            }
            TableMsgToAll(&bankerInfo, net::S2C_MSG_LAND_BANKER_INFO);

            //OnSubBankerInfo();

            return true;
        }
        m_coolLogic.beginCooling(GetCallScoreTime());

        return true;
    }

    void CGameLandTable::OnCallScoreTimeOut() {
        LOG_TAB_DEBUG("time out call score");
        if (m_curUser == INVALID_CHAIR)
        {
            ReGameStart();
            return;
        }
        OnUserCallScore(m_curUser, 0);
    }

// 用户出牌
    bool CGameLandTable::OnUserOutCard(uint16_t chairID, uint8_t cardData[], uint8_t cardCount) {
        //效验状态
        if (chairID != m_curUser)
        {
            LOG_TAB_ERROR("you can't oper outcard:{}", chairID);
            return false;
        }
        //获取类型
        m_gameLogic.SortCardList(cardData, cardCount, ST_ORDER);
        uint8_t cardType = m_gameLogic.GetCardType(cardData, cardCount);
        LOG_TAB_DEBUG("玩家出牌:{}", chairID);
        //类型判断
        if (cardType == CT_ERROR)
        {
            LOG_TAB_ERROR("the card type is error:{}", cardType);
            return false;
        }
        //出牌判断
        if (m_turnCardCount != 0)
        {
            //对比扑克
            if (m_gameLogic.CompareCard(m_turnCardData, cardData, m_turnCardCount, cardCount) == false)
            {
                LOG_TAB_ERROR("the outcard is not handcard:{}", chairID);
                return false;
            }
        }

        //删除扑克
        if (m_gameLogic.RemoveCardList(cardData, cardCount, m_handCardData[chairID], m_handCardCount[chairID]) == false)
        {
            LOG_TAB_ERROR("removecard error:{}", chairID);
            return false;
        }

        //出牌变量
        m_outCardCount[chairID]++;

        //设置变量
        m_turnCardCount = cardCount;
        m_handCardCount[chairID] -= cardCount;
        memcpy(m_turnCardData, cardData, sizeof(uint8_t) * cardCount);

        //炸弹判断
        if ((cardType == CT_BOMB_CARD) || (cardType == CT_MISSILE_CARD))
        {
            m_bombCount++;
            m_eachBombCount[chairID]++;
        }

        //切换用户
        m_turnWiner = chairID;
        if (m_handCardCount[chairID] != 0)
        {
            m_curUser = (m_curUser + 1) % GAME_LAND_PLAYER;
        }
        else
        {
            m_curUser = INVALID_CHAIR;
        }

        // 发送数据
        net::msg_land_out_card_rep outCard;
        outCard.set_out_card_user(chairID);
        outCard.set_cur_user(m_curUser);
        for (uint8_t i = 0; i < m_turnCardCount; ++i)
        {
            outCard.add_card_data(m_turnCardData[i]);
        }
        TableMsgToAll(&outCard, net::S2C_MSG_LAND_OUT_CARD);

        WriteOutCardLog(chairID, m_turnCardData, m_turnCardCount, 0);

        //结束判断
        if (m_curUser == INVALID_CHAIR)
        {
            OnGameEnd(chairID, GER_NORMAL);
        }
        else
        {
            m_coolLogic.beginCooling(GetOutCardTime());
        }

        //OnSubOutCard();
        return true;
    }

    void CGameLandTable::OnOutCardTimeOut() {
        LOG_DEBUG("outcard time out");
        m_vecPlayers[m_curUser].overTimes++;
        if (m_turnCardCount == 0)// 主动出牌
        {
            uint8_t outCard[MAX_LAND_COUNT];
            memset(outCard, 0, sizeof(outCard));
            outCard[0] = m_handCardData[m_curUser][m_handCardCount[m_curUser] - 1];
            OnUserOutCard(m_curUser, outCard, 1);
        }
        else
        {
            OnUserPassCard(m_curUser);
        }
    }

// 托管出牌
    void CGameLandTable::OnUserAutoCard() {
        if (m_turnCardCount == 0)// 自动出牌
        {
            uint8_t outCard[MAX_LAND_COUNT];
            memset(outCard, 0, sizeof(outCard));
            outCard[0] = m_handCardData[m_curUser][m_handCardCount[m_curUser] - 1];// 出最小一张
            OnUserOutCard(m_curUser, outCard, 1);
        }
        else
        {
            //搜索扑克
            tagOutCardResult OutCardResult;
            if (m_gameLogic.SearchOutCard(m_handCardData[m_curUser], m_handCardCount[m_curUser], m_turnCardData,
                                          m_turnCardCount, OutCardResult))
            {
                if (OnUserOutCard(m_curUser, OutCardResult.cbResultCard, OutCardResult.cbCardCount) == false)
                {
                    OnUserPassCard(m_curUser);
                }
            }
            else
            {
                OnUserPassCard(m_curUser);
            }
        }
    }

// 出牌时间跟叫地主时间
    uint32_t CGameLandTable::GetCallScoreTime() {
        return 25 * 1000;
    }

    uint32_t CGameLandTable::GetOutCardTime() {
        return 20 * 1000;
    }

// 发送场景信息(断线重连)
    void CGameLandTable::SendGameScene(std::shared_ptr<CGamePlayer> pPlayer) {
        LOG_TAB_DEBUG("send game scene:{}", pPlayer->GetUID());
        uint16_t chairID = GetChairID(pPlayer);
        CHECK_LOG_VOID(chairID != 0xFF, "玩家不在座位上:{}", pPlayer->GetUID());
        if (GetGameState() == net::TABLE_STATE_FREE)
        {
            return;
        }
        else if (GetGameState() == net::TABLE_STATE_PLAY || GetGameState() == net::TABLE_STATE_CALL)
        {
            net::msg_land_game_info_rep msg;
            msg.set_bomb_count(m_bombCount);
            msg.set_banker_user(m_bankerUser);
            msg.set_cur_user(m_curUser);
            msg.set_turn_winer(m_turnWiner);
            msg.set_first_user(m_firstUser);

            for (uint8_t i = 0; i < m_turnCardCount; ++i)
            {
                msg.add_turn_card_data(m_turnCardData[i]);
            }
            for (uint8_t i = 0; i < getArrayLen(m_bankerCard); ++i)
            {
                if (GetGameState() == net::TABLE_STATE_CALL)
                {
                    msg.add_banker_card(0);
                }
                else
                {
                    msg.add_banker_card(m_bankerCard[i]);
                }
            }
            for (uint8_t i = 0; i < GAME_LAND_PLAYER; ++i)
            {
                msg.add_hand_card_count(m_handCardCount[i]);
                msg.add_call_score(m_callScore[i]);
                LOG_TAB_DEBUG("手牌数：{}--分数：{}", m_handCardCount[i], m_callScore[i]);
            }
            for (uint8_t i = 0; i < m_handCardCount[chairID]; ++i)
            {
                msg.add_hand_card_data(m_handCardData[chairID][i]);
            }
            msg.set_game_state(GetGameState());
            msg.set_wait_time(m_coolLogic.getCoolTick());

            pPlayer->SendMsgToClient(&msg, net::S2C_MSG_LAND_GAME_INFO);
        }
    }

// 游戏重新开始
    void CGameLandTable::ReGameStart() {
        LOG_TAB_DEBUG("restart game {}", m_callCount);
        m_callCount++;
        m_firstUser = (m_firstUser + 1) % GAME_LAND_PLAYER;
        ResetGameData();
        SetGameState(TABLE_STATE_FREE);
        OnGameStart();
    }

// 重置游戏数据
    void CGameLandTable::ResetGameData() {
        //游戏变量
        m_bombCount = 0;
        m_bankerUser = INVALID_CHAIR;
        m_curUser = INVALID_CHAIR;
        memset(m_outCardCount, 0, sizeof(m_outCardCount));
        memset(m_eachBombCount, 0, sizeof(m_eachBombCount));
        //叫分信息
        memset(m_callScore, 0, sizeof(m_callScore));
        //出牌信息
        m_turnCardCount = 0;
        m_turnWiner = INVALID_CHAIR;
        memset(m_turnCardData, 0, sizeof(m_turnCardData));
        //扑克信息
        memset(m_bankerCard, 0, sizeof(m_bankerCard));
        memset(m_handCardData, 0, sizeof(m_handCardData));
        memset(m_handCardCount, 0, sizeof(m_handCardCount));

    }

// 重置扑克
    void CGameLandTable::ReInitPoker() {
        m_gameLogic.RandCardList(m_randCard, getArrayLen(m_randCard));
    }

// 洗牌
    void CGameLandTable::ShuffleTableCard() {
        ReInitPoker();
    }

// 发牌并返回当前玩家位置
    uint16_t CGameLandTable::DispatchUserCard(uint16_t startUser, uint32_t cardIndex) {
        LOG_TAB_DEBUG("发牌：{}--{}", startUser, cardIndex);
        uint16_t curUser = 0;
        memset(m_handCardCount, 0, sizeof(m_handCardCount));
        memset(m_handCardData, 0, sizeof(m_handCardData));

        for (uint16_t i = 0; i < GAME_LAND_PLAYER; ++i)
        {
            memcpy(&m_handCardData[i], &m_randCard[NORMAL_COUNT * i], NORMAL_COUNT);
            m_handCardCount[i] = NORMAL_COUNT;
        }
        curUser = (cardIndex + startUser) % GAME_LAND_PLAYER;

        return curUser;
    }

// AI 函数
//游戏开始
    bool CGameLandTable::OnSubGameStart() {
        for (uint16_t wChairID = 0; wChairID < GAME_LAND_PLAYER; ++wChairID)
        {
            m_gameLogic.SetUserCard(wChairID, m_handCardData[wChairID], NORMAL_COUNT);
            //叫牌扑克
            m_gameLogic.SetLandScoreCardData(wChairID, m_handCardData[wChairID], MAX_LAND_COUNT);

        }

        return true;
    }

//庄家信息
    bool CGameLandTable::OnSubBankerInfo() {
        //设置底牌
        m_gameLogic.SetBackCard(m_bankerUser, m_bankerCard, 3);
        m_gameLogic.SetBanker(m_bankerUser);
        m_callCount = 0;
        return true;
    }

//用户出牌
    bool CGameLandTable::OnSubOutCard() {
        //设置变量
        m_gameLogic.RemoveUserCardData(m_turnWiner, m_turnCardData, m_turnCardCount);

        return true;
    }

    bool CGameLandTable::OnRobotCallScore() {
        uint8_t score = m_gameLogic.LandScore(m_curUser, 0);

//        int bidNumber = m_ddzAIRobot[m_curUser].GetBidNumber();
//        LOG_DEBUG("°????ú?÷??????·???:%d",bidNumber);
//        if(bidNumber >= 2)
//            score = 1;
        OnUserCallScore(m_curUser, score);
        return true;
    }

    bool CGameLandTable::OnRobotOutCard() {
        tagOutCardResult OutCardResult;
        try
        {
            uint16_t wMeChairID = m_curUser;
            m_gameLogic.SearchOutCard(m_handCardData[wMeChairID], m_handCardCount[wMeChairID], m_turnCardData, m_turnCardCount, m_turnWiner, wMeChairID, OutCardResult);
        }
        catch (...)
        {
            OnUserAutoCard();
            return true;
        }
        if (OutCardResult.cbCardCount > 0 && CT_ERROR == m_gameLogic.GetCardType(OutCardResult.cbResultCard, OutCardResult.cbCardCount))
        {
            memset(&OutCardResult, 0, sizeof(OutCardResult));
            OnUserAutoCard();
            return true;
        }
        if (m_turnCardCount == 0)
        {
            if (OutCardResult.cbCardCount == 0)
            {
                uint16_t wMeChairID = m_curUser;

                OutCardResult.cbCardCount = 1;
                OutCardResult.cbResultCard[0] = m_handCardData[wMeChairID][m_handCardCount[wMeChairID] - 1];
            }
        }
        else
        {
            if (!m_gameLogic.CompareCard(m_turnCardData, OutCardResult.cbResultCard, m_turnCardCount, OutCardResult.cbCardCount))
            {
                OnUserPassCard(m_curUser);
                return true;
            }
        }
        if (OutCardResult.cbCardCount > 0)
        {
            if (OnUserOutCard(m_curUser, OutCardResult.cbResultCard, OutCardResult.cbCardCount) == false)
            {
                OnUserAutoCard();
            }
        }
        else
        {
            OnUserPassCard(m_curUser);
        }
        return true;
    }


};















