//
// Created by toney on 16/4/6.
//
#include <data_cfg_mgr.h>
#include "game_kuaipao_table.h"
#include "game_table/game_room.h"
#include "nlohmann/json_wrap.h"
#include "common_logic.h"
#include "error_code.pb.h"

using namespace std;
using namespace svrlib;
using namespace net;

namespace {
    const static int32_t s_ReadyTimeOut = 20;
    const static int32_t s_DispatchTime = 5 * 1000;
};
namespace game_kuaipao {
// 跑得快游戏桌子
    CGameKuaipaoTable::CGameKuaipaoTable(std::shared_ptr<CGameRoom> pRoom, int64_t tableID)
            : CGameCoinTable(pRoom, tableID) {
        m_vecPlayers.clear();

        m_fourTakeThree = false;
        m_fristOutType = emFristOutType_BankerRandAll;
        m_playCardNum = 16;
        m_hong10FanBei = false;

        m_hong10User = INVALID_CHAIR;

        //炸弹变量
        m_bankerUser = INVALID_CHAIR;
        m_curUser = INVALID_CHAIR;
        memset(m_outCardCount, 0, sizeof(m_outCardCount));
        memset(m_operCount, 0, sizeof(m_operCount));
        m_outCardTotal = 0;
        //游戏变量
        memset(m_eachBombCount, 0, sizeof(m_eachBombCount));

        //出牌信息
        m_turnCardCount = 0;
        m_turnWiner = INVALID_CHAIR;
        memset(m_turnCardData, 0, sizeof(m_turnCardData));

        //扑克信息
        memset(m_handCardData, 0, sizeof(m_handCardData));
        memset(m_handCardCount, 0, sizeof(m_handCardCount));
        memset(m_gameScore, 0, sizeof(m_gameScore));
        m_gameLogic.SetKuaiPaoTable(this);
        m_openPiaoScore = false;
        memset(m_piaoScore, INVALID_CHAIR, sizeof(m_piaoScore));
    }

    CGameKuaipaoTable::~CGameKuaipaoTable() {

    }

    void CGameKuaipaoTable::GetTableFaceInfo(net::table_info *pInfo) {
        GetTableFaceBaseInfo(pInfo);
        pInfo->set_show_hand_num(1);
        pInfo->set_call_time(s_DispatchTime);
        pInfo->set_card_time(GetOutCardTime());

    }


//配置桌子
    bool CGameKuaipaoTable::Init() {
        SetGameState(net::TABLE_STATE_FREE);

        ResetInitSeat(GAME_KUAIPAO_PLAYER);
        m_playerCount = GAME_KUAIPAO_PLAYER;
        m_coolLogic.clearCool();

        //初始化玩法参数


        m_conf.seatNum = 3;//3人玩法
        ResetInitSeat(m_conf.seatNum);
        m_playerCount = m_conf.seatNum;
        if (m_conf.playType == KUAIPAO_TYPE_fifteen)
        {// 玩法1：15张玩法
            m_playCardNum = 15;
        }
        else
        {// 玩法2：16张玩法
            m_playCardNum = 16;
        }

        m_fristOutType = emFristOutType_BankerRandAll;
        m_fristHei3 = 0;
        //炸弹不可拆，1：是 0：否
        m_splitBomb = true;
        //允许四带二，1：是 0：否
        m_fourTakeTwo = 1;
        //允许四带三，1：是 0：否
        m_fourTakeThree = 1;
        //红桃10翻倍，1：是 0：否
        m_hong10FanBei = 0;

        // 三张少带出完，1：是 0：否
        m_threeOutLess = true;
        // 三张少带接完，1：是 0：否
        m_threePassLess = true;
        // 飞机少带出完，1：是 0：否
        m_feijiOutLess = true;
        //飞机少带接完，1：是 0：否
        m_feijiPassLess = true;

        return true;
    }

//复位桌子
    void CGameKuaipaoTable::ResetTable() {
        CGameCoinTable::ResetTable();
        ResetGameData();

        SetGameState(TABLE_STATE_FREE);
        ResetPlayerReady();
    }

    void CGameKuaipaoTable::OnTimeTick() {
        if (m_coolLogic.isTimeOut())
        {
            uint8_t tableState = GetGameState();
            switch (tableState)
            {
                case TABLE_STATE_FREE:
                {
                    if (IsCanStart())
                    {
                        InitBlingLog(true);
                        ResetPlayStatus(1);
                        if (IsOpenPiaoScore())
                        {// 飘分
                            StartPiaoScore();
                        }
                        else
                        {// 直接开始
                            OnGameStart();
                        }
                    }
                    break;
                }
                case TABLE_STATE_CALL:
                {
                    OnGameStart();
                    break;
                }
                case TABLE_STATE_PLAY:
                {
                    OnOutCardTimeOut();
                    break;
                }
                case TABLE_STATE_WAIT:
                {
                    OnGameEnd(m_turnWiner, GER_NORMAL);
                    break;
                }
                case TABLE_STATE_GAME_END:
                {
                    m_coolLogic.beginCooling(3000);
                    SetGameState(TABLE_STATE_FREE);
                    OnGameRoundFlush();
                    break;
                }
                default:
                    break;
            }
        }
        if (GetGameState() == TABLE_STATE_PLAY)
        {
            if (m_coolLogic.getPassTick() > 500)
            {
                CheckLastHandAutoCard();
            }
            if (m_vecPlayers[m_curUser].overTimes == 2)
            {
                m_vecPlayers[m_curUser].autoStatus = 1;
                SendReadyStateToClient();
            }
            if (m_vecPlayers[m_curUser].autoStatus == 1 && m_coolLogic.getPassTick() > 1000)
            {
                OnUserAutoCard();
            }
        }
    }

// 游戏消息
    int CGameKuaipaoTable::OnGameMessage(std::shared_ptr<CGamePlayer> pPlayer, uint16_t cmdID, const uint8_t *pkt_buf, uint16_t buf_len) {
        uint16_t chairID = GetChairID(pPlayer);
        CHECK_LOG_RET(chairID != 0xFF, 0, "玩家不在座位上:{}", pPlayer->GetUID());
        LOG_TAB_DEBUG("message {}--{}", chairID, cmdID);
        switch (cmdID)
        {
            case net::C2S_MSG_KUAIPAO_OUT_CARD_REQ:// 用户出牌
            {
                net::msg_kuaipao_out_card_req msg;
                PARSE_MSG_FROM_ARRAY(msg);
                //状态效验
                if (GetGameState() != TABLE_STATE_PLAY)
                {
                    LOG_TAB_DEBUG("not play state");
                    return 0;
                }
                //消息处理
                if (msg.card_data_size() > MAX_KUAIPAO_COUNT)
                {
                    LOG_TAB_ERROR("the card is more 20:{}", chairID);
                    return 0;
                }
                uint8_t cardCount = 0;
                uint8_t cardData[MAX_KUAIPAO_COUNT];
                memset(cardData, 0, sizeof(cardData));
                for (uint8_t i = 0; i < msg.card_data_size(); ++i)
                {
                    cardData[i] = msg.card_data(i);
                    cardCount++;
                }
                if (OnUserOutCard(chairID, cardData, cardCount) == false)
                {
                    NotifyOperFail(chairID, 1);
                }
                break;
            }
            case net::C2S_MSG_KUAIPAO_PASS_CARD_REQ:// 用户放弃
            {
                net::msg_kuaipao_pass_card_req msg;
                PARSE_MSG_FROM_ARRAY(msg);
                //状态效验
                if (GetGameState() != TABLE_STATE_PLAY)
                {
                    LOG_TAB_DEBUG("not play state");
                    return 0;
                }
                if (chairID != m_curUser)
                {
                    LOG_TAB_ERROR("you can't oper outcard:{}", chairID);
                    return 0;
                }
                //消息处理
                OnUserAutoCard();
                return 0;
                break;
            }
            case net::C2S_MSG_KUAIPAO_PIAO_SCORE_REQ://飘分
            {
                if (GetGameState() != TABLE_STATE_CALL)return 0;

                net::msg_kuaipao_piao_score_req msg;
                PARSE_MSG_FROM_ARRAY(msg);
                LOG_TAB_DEBUG("玩家飘分:{}--{}", chairID, msg.score());
                return OnUserPiaoScore(chairID, msg.score());
                break;
            }
            default:
                return 0;
        }
        return 0;
    }

// 游戏开始
    bool CGameKuaipaoTable::OnGameStart() {
        LOG_TAB_DEBUG("game start");

        CheckUserPiaoScore();

        // 出牌信息
        m_turnCardCount = 0;
        m_turnWiner = INVALID_CHAIR;
        memset(m_turnCardData, 0, sizeof(m_turnCardData));

        SetGameState(net::TABLE_STATE_PLAY);

        DispatchCard(m_playCardNum);

        //设置用户
        for (uint16_t i = 0; i < m_playerCount; i++)
        {
            for (uint8_t j = 0; j < m_handCardCount[i]; j++)
            {
                if (m_handCardData[i][j] == 0x2A)
                {// 红桃10
                    m_hong10User = i;
                }
            }
        }

        if (m_bankerUser == INVALID_CHAIR)
        {
            uint16_t chairID = SetBankerBy3();
            //设置用户
            m_bankerUser = chairID;
            m_curUser = chairID;
        }
        else
        {
            m_curUser = m_bankerUser;
        }

        OnGameRoundStart();
        WriteBankerLog(m_curUser);
        //发送数据
        for (uint16_t i = 0; i < m_playerCount; i++)
        {
            //构造消息
            net::msg_kuaipao_start_rep gameStart;
            gameStart.set_start_user(m_bankerUser);
            gameStart.set_cur_user(m_curUser);

            for (uint8_t j = 0; j < m_handCardCount[i]; ++j)
            {
                gameStart.add_card_data(m_handCardData[i][j]);
            }
            //发送数据
            TableMsgToClient(i, &gameStart, net::S2C_MSG_KUAIPAO_START);
        }

        // 启动叫分定时器
        m_coolLogic.beginCooling(s_DispatchTime + GetOutCardTime());

        return true;
    }

//游戏结束
    bool CGameKuaipaoTable::OnGameEnd(uint16_t chairID, uint8_t reason) {
        LOG_TAB_DEBUG("game end:{}--{}", chairID, reason);
        m_coolLogic.clearCool();
        switch (reason)
        {
            case GER_NORMAL:        //常规结束
            {
                net::msg_kuaipao_game_over_rep gameOver;
                gameOver.set_hong10_user(0);
                m_operLog["h10"] = -1;
                if (m_hong10FanBei)
                {
                    gameOver.set_hong10_user(m_hong10User);
                    m_operLog["h10"] = m_hong10User;
                }

                //用户扑克
                for (uint16_t i = 0; i < GAME_KUAIPAO_PLAYER; i++)
                {//二人跑得快也要全部亮牌
                    //拷贝扑克
                    gameOver.add_card_counts(m_handCardCount[i]);
                    for (uint8_t j = 0; j < m_handCardCount[i]; ++j)
                    {
                        gameOver.add_hand_card_data(m_handCardData[i][j]);
                    }
                    WriteOutCardLog(i, m_handCardData[i], m_handCardCount[i], 1);
                }


                //统计积分
                int64_t calcScore[GAME_KUAIPAO_PLAYER];
                memset(calcScore, 0, sizeof(calcScore));

                //积分统计
                for (uint16_t i = 0; i < m_playerCount; i++)
                {
                    //游戏积分
                    int32_t lScoreTimes = 0;
                    int32_t lBaseScore = 0;
                    if (m_handCardCount[i] > 1)
                    {
                        lScoreTimes = (m_handCardCount[i] == m_playCardNum) ? 2L : 1L;
                        lBaseScore = m_handCardCount[i] * GetBaseScore() * lScoreTimes;
                        if (m_hong10FanBei)
                        {//红桃10翻倍
                            if (i == m_hong10User || chairID == m_hong10User)
                            {
                                lBaseScore = lBaseScore * 2;
                            }
                        }
                        lBaseScore += (m_piaoScore[i] + m_piaoScore[chairID]);//飘分
                        LOG_TAB_DEBUG("增加玩家飘分:{}--{}", m_piaoScore[i], m_piaoScore[chairID]);
                    }
                    calcScore[i] -= lBaseScore;
                    calcScore[chairID] += lBaseScore;
                    gameOver.add_guanmens((m_handCardCount[i] == m_playCardNum) ? 1 : 0);

                }

                if (!CanMinus())
                {
                    LOG_TAB_DEBUG("积分不能为负数，分配分数");
                    vector<stDevide> devides;
                    for (uint16_t i = 0; i < GAME_KUAIPAO_PLAYER; ++i)
                    {
                        devides.push_back(GetDevide(i, calcScore[i] + m_gameScore[i], false));
                    }
                    DevideByWeight(devides, true);
                    for (uint16_t i = 0; i < GAME_KUAIPAO_PLAYER; ++i)
                    {
                        calcScore[i] = devides[i].realWin;
                    }
                }

                //结算积分
                for (uint16_t i = 0; i < m_playerCount; i++)
                {
                    LOG_TAB_DEBUG("玩家结算:{}", calcScore[i]);
                    auto player = GetPlayer(i);
                    if (player == nullptr)continue;

                    CalcPlayerGameInfo(player->GetUID(), calcScore[i]);
                }

                for (uint16_t i = 0; i < m_playerCount; ++i)
                {
                    gameOver.add_scores(calcScore[i]);
                    gameOver.add_bomb_scores(m_gameScore[i]);
                    gameOver.add_bomb_counts(m_eachBombCount[i]);

                }
                //发送数据
                TableMsgToAll(&gameOver, net::S2C_MSG_KUAIPAO_GAME_OVER);
                //结束游戏
                ResetGameData();
                if (m_fristOutType != emFristOutType_BankerAll)
                {
                    m_bankerUser = chairID;
                }
                SetGameState(TABLE_STATE_GAME_END);
                m_coolLogic.beginCooling(1000);

                OnGameRoundOver();
                return true;
                break;
            }
            case GER_DISMISS:        //游戏解散
            {
                LOG_TAB_ERROR("froce dis game");
                for (uint8_t i = 0; i < m_playerCount; ++i)
                {
                    if (m_vecPlayers[i].pPlayer != nullptr)
                    {
                        LeaveTable(m_vecPlayers[i].pPlayer);
                    }
                }
                ResetTable();
                return true;
                break;
            }
            case GER_USER_LEAVE:        //用户强退
            case GER_NETWORK_ERROR:     //网络中断
            default:
                return true;
        }
        return false;
    }

// 用户飘分
    bool CGameKuaipaoTable::OnUserPiaoScore(uint16_t chairID, uint8_t score) {
        if (!IsOpenPiaoScore())
        {
            LOG_TAB_DEBUG("飘分未开启,不能飘分");
            return false;
        }
        if (m_piaoScore[chairID] != INVALID_CHAIR)
        {
            LOG_TAB_DEBUG("已经飘分过了");
            return false;
        }
        m_piaoScore[chairID] = std::min(score, (uint8_t) 10);//限制最大漂10分
        NotifyPiaoScore(chairID);

        for (uint8_t i = 0; i < m_playerCount; ++i)
        {
            if (m_piaoScore[i] == INVALID_CHAIR)
                return true;
        }
        m_coolLogic.clearCool();

        return true;
    }

    void CGameKuaipaoTable::NotifyPiaoScore(uint16_t chairID) {
        net::msg_kuaipao_piao_score_rep msg;
        msg.set_score(m_piaoScore[chairID]);
        msg.set_user(chairID);
        TableMsgToAll(&msg, S2C_MSG_KUAIPAO_PIAO_SCORE_REP);
        LOG_TAB_DEBUG("通知飘分:{}--{}", chairID, m_piaoScore[chairID]);
    }

    void CGameKuaipaoTable::CheckUserPiaoScore() {
        LOG_TAB_DEBUG("检测玩家飘分数值");
        for (uint8_t i = 0; i < m_playerCount; ++i)
        {
            if (m_piaoScore[i] == INVALID_CHAIR)
            {
                m_piaoScore[i] = 0;
                if (IsOpenPiaoScore())
                {// 飘分
                    NotifyPiaoScore(i);
                }
            }
        }
    }

// 用户放弃
    bool CGameKuaipaoTable::OnUserPassCard(uint16_t chairID) {
        //效验状态
        if (chairID != m_curUser || m_turnCardCount == 0)
        {
            LOG_TAB_ERROR("not you oper:{}", chairID);
            return false;
        }
        //设置变量
        m_curUser = (m_curUser + 1) % m_playerCount;
        if (m_curUser == m_turnWiner)
        {
            CheckBombCalc();
            m_turnCardCount = 0;
        }
        uint8_t isOver = m_turnCardCount == 0 ? 1 : 0;
        bool isPass = CheckUserPass();

        net::msg_kuaipao_pass_card_rep passCard;
        passCard.set_pass_card_user(chairID);
        passCard.set_cur_user(m_curUser);
        passCard.set_turn_over(isOver);
        passCard.set_is_pass(isPass);

        TableMsgToAll(&passCard, net::S2C_MSG_KUAIPAO_PASS_CARD);
        m_coolLogic.beginCooling(GetOutCardTime());
        m_autoOutCard = false;

        m_operCount[chairID]++;
        return true;
    }

// 用户出牌
    bool CGameKuaipaoTable::OnUserOutCard(uint16_t chairID, uint8_t cardData[], uint8_t cardCount) {
        LOG_TAB_DEBUG("玩家出牌:{}", chairID);
        CCommonLogic::LogCardString(cardData, cardCount);
        //效验状态
        if (chairID != m_curUser)
        {
            LOG_TAB_ERROR("you can't oper outcard:{}", chairID);
            return false;
        }
        //获取类型
        m_gameLogic.SortCardListByLogicValue(cardData, cardCount);
        uint8_t cardType = m_gameLogic.GetCardType(cardData, cardCount, IsCanLessOutCard(chairID, cardCount));

        //类型判断
        if (cardType == CT_ERROR)
        {
            LOG_TAB_ERROR("the card type is error:{}", cardType);
            CCommonLogic::LogCardString(cardData, cardCount);
            return false;
        }
        //是否4带3
        if (cardType == CT_FOUR_TAKE_THREE && !m_fourTakeThree)
        {
            LOG_TAB_ERROR("没有开启4带3选项");
            return false;
        }
        //是否4带2
        if (cardType == CT_FOUR_TAKE_TWO && !m_fourTakeTwo)
        {
            LOG_TAB_ERROR("没有开启4带2选项");
            return false;
        }
        // 炸弹不可拆
        if (CheckSplitBomb(chairID, cardData, cardCount))
        {
            LOG_TAB_ERROR("炸弹不可以拆开");
            return false;
        }

        //首出判断
//        if (m_fristHei3 && m_round == 0 && (chairID == m_bankerUser)
//            && (m_outCardTotal == 0))
//        {
//            if (IsHaveHei3(chairID))
//            {
//                uint8_t i = 0;
//                for (; i < cardCount; i++)
//                {
//                    if (cardData[i] == 0x33)break;
//                }
//                if (i == cardCount)
//                {
//                    LOG_TAB_DEBUG("首出要出黑桃3");
//                    return false;
//                }
//            }
//        }
        //报单必顶
        if (true)
        {
            if ((m_handCardCount[chairID] != 0))
            {
                //获取用户
                uint16_t wNextPlayer = (chairID + 1) % m_playerCount;

                //包赔判断
                if ((cardCount == 1) && (m_handCardCount[wNextPlayer] == 1))
                {
                    if (m_gameLogic.CompareCard(m_handCardData[chairID], cardData, 1, 1, false, false) != FALSE)
                    {
                        LOG_TAB_DEBUG("报单必顶,不能放走");
                        return false;
                    }
                }
            }
        }
        //出牌判断
        if (m_turnCardCount != 0)
        {
            //对比扑克
            if (m_gameLogic.CompareCard(cardData, m_turnCardData, cardCount, m_turnCardCount,
                                        IsCanLessOutCard(chairID, cardCount), false) != TRUE)
            {
                LOG_TAB_ERROR("the outcard is not big than last card:{}", chairID);
                CCommonLogic::LogCardString(cardData, cardCount);
                CCommonLogic::LogCardString(m_turnCardData, m_turnCardCount);
                return false;
            }
        }

        //删除扑克
        if (m_gameLogic.RemoveCardList(cardData, cardCount, m_handCardData[chairID], m_handCardCount[chairID]) ==
            false)
        {
            LOG_TAB_ERROR("removecard error:{}", chairID);
            return false;
        }

        //出牌变量
        m_outCardCount[chairID]++;
        m_outCardTotal++;
        //设置变量
        m_turnCardCount = cardCount;
        m_handCardCount[chairID] -= cardCount;
        memcpy(m_turnCardData, cardData, sizeof(uint8_t) * cardCount);
        m_turnCardType = cardType;

        //切换用户
        m_turnWiner = chairID;
        if (m_handCardCount[chairID] != 0)
        {
            m_curUser = (m_curUser + 1) % m_playerCount;
        }
        else
        {
            m_curUser = INVALID_CHAIR;
        }
        bool isPass = CheckUserPass();

        // 发送数据
        net::msg_kuaipao_out_card_rep outCard;
        outCard.set_out_card_user(chairID);
        outCard.set_cur_user(m_curUser);
        outCard.set_card_type(m_turnCardType);
        outCard.set_is_pass(isPass ? 1 : 0);

        // 排序
        uint8_t tmpCardData[MAX_KUAIPAO_COUNT];   // 出牌数据
        memset(tmpCardData, 0, sizeof(tmpCardData));
        memcpy(tmpCardData, m_turnCardData, m_turnCardCount);
        if (cardType >= CT_THREE_LINE)
        {
            m_gameLogic.SortCardListByCardNum(tmpCardData, m_turnCardCount);
        }
        for (uint8_t i = 0; i < m_turnCardCount; ++i)
        {
            outCard.add_card_data(tmpCardData[i]);
        }
        TableMsgToAll(&outCard, net::S2C_MSG_KUAIPAO_OUT_CARD);

        //写入出牌日志
        WriteOutCardLog(chairID, tmpCardData, m_turnCardCount, 0);

        //结束判断
        if (m_curUser == INVALID_CHAIR)
        {
            if (CheckBombCalc())
            {
                LOG_TAB_DEBUG("炸弹飘分，延迟2秒");
                SetGameState(TABLE_STATE_WAIT);
                m_coolLogic.beginCooling(2000);
            }
            else
            {
                OnGameEnd(chairID, GER_NORMAL);
            }
        }
        else
        {
            m_coolLogic.beginCooling(GetOutCardTime());
            m_autoOutCard = false;
        }
        m_operCount[chairID]++;
        return true;
    }

// 操作失败
    void CGameKuaipaoTable::NotifyOperFail(uint16_t chairID, uint8_t reason) {
        net::msg_kuaipao_oper_fail_rep rep;
        rep.set_reason(reason);

        TableMsgToClient(chairID, &rep, net::S2C_MSG_KUAIPAO_OPER_FAIL, false);
        LOG_TAB_ERROR("回复出牌失败:{}", chairID);
    }

    void CGameKuaipaoTable::OnOutCardTimeOut() {
        m_vecPlayers[m_curUser].overTimes++;
        OnUserAutoCard();
    }

// 托管出牌
    void CGameKuaipaoTable::OnUserAutoCard() {
        if (m_turnCardCount == 0)// 自动出牌
        {
            uint8_t outCard[MAX_KUAIPAO_COUNT];
            memset(outCard, 0, sizeof(outCard));
            //首出判断
            if (m_fristHei3 && (m_curUser == m_bankerUser) && (m_handCardCount[m_curUser] == m_playCardNum))
            {
                for (uint8_t i = 0; i < MAX_KUAIPAO_COUNT; i++)
                {
                    if (m_handCardData[m_curUser][i] == 0x33)
                    {
                        outCard[0] = 0x33;
                        OnUserOutCard(m_curUser, outCard, 1);
                        LOG_TAB_DEBUG("自动出牌1");
                        return;
                    }
                }
            }

            //搜索扑克
            tagOutCardResult OutCardResult;
            if (m_gameLogic.SearchActionOutCard(m_handCardData[m_curUser],
                                                m_handCardCount[m_curUser],
                                                GetNextPlayerCardNum(m_curUser),
                                                OutCardResult))
            {
                LOG_TAB_DEBUG("自动出牌--{}", m_curUser);
                if (OnUserOutCard(m_curUser, OutCardResult.cbResultCard, OutCardResult.cbCardCount) == false)
                {
                    OnUserPassCard(m_curUser);
                }
            }
        }
        else
        {
            //搜索扑克
            tagOutCardResult OutCardResult;
            bool lastHand = IsCanLessOutCard(m_curUser, m_handCardCount[m_curUser]);
            if (m_gameLogic.SearchOutCard(m_handCardData[m_curUser], m_handCardCount[m_curUser], m_turnCardData,
                                          m_turnCardCount, OutCardResult, lastHand))
            {
                LOG_TAB_DEBUG("自动接牌--{}", m_curUser);
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

// 检测是否要不起
    bool CGameKuaipaoTable::CheckUserPass() {
        if (m_curUser == INVALID_CHAIR)return false;
        if (m_turnCardCount == 0)return false;

        tagOutCardResult OutCardResult;
        bool lastHand = IsCanLessOutCard(m_curUser, m_handCardCount[m_curUser]);
        //搜索扑克
        if (m_gameLogic.SearchOutCard(m_handCardData[m_curUser], m_handCardCount[m_curUser], m_turnCardData,
                                      m_turnCardCount, OutCardResult, lastHand) == false)
        {
            return true;
        }
        return false;
    }

// 一手接完自动打(没有炸弹)
    void CGameKuaipaoTable::CheckLastHandAutoCard() {
        if (m_curUser == INVALID_CHAIR)return;
        //断线卡住
        //if (IsDisconnect(m_curUser))return;

        if (m_outCardTotal == 0 && m_coolLogic.getPassTick() < 4000)return;
        if (m_autoOutCard)return;
        m_autoOutCard = true;
        if (m_turnCardCount > 0)
        {// 最后一手接完
            LOG_TAB_DEBUG("检测最后一手接完");
            tagOutCardResult OutCardResult;

            //搜索炸弹
            bool bNeedAuto = true;
            if (m_gameLogic.SearchBombOutCard(m_handCardData[m_curUser], m_handCardCount[m_curUser], 4,
                                              OutCardResult) && m_handCardCount[m_curUser] > 4)
            {//有炸弹，多余4张不自动出
                LOG_TAB_DEBUG("有炸弹不自动接完");
                bNeedAuto = false;
            }
            //搜索扑克
            bool lastHand = IsCanLessOutCard(m_curUser, m_handCardCount[m_curUser]);
            uint8_t cardType = m_gameLogic.GetCardType(m_handCardData[m_curUser], m_handCardCount[m_curUser], lastHand);
            //类型判断
            if (cardType != CT_ERROR && bNeedAuto)
            {
                //对比扑克
                if (m_gameLogic.CompareCard(m_handCardData[m_curUser], m_turnCardData, m_handCardCount[m_curUser],
                                            m_turnCardCount, lastHand, false) == TRUE)
                {
                    LOG_TAB_DEBUG("最后一手自动---接完--{} 最后一手 {}", m_curUser, lastHand);
                    OnUserOutCard(m_curUser, m_handCardData[m_curUser], m_handCardCount[m_curUser]);
                    return;
                }
            }
            //检测要不起自动过
            //搜索扑克
            if (m_gameLogic.SearchOutCard(m_handCardData[m_curUser], m_handCardCount[m_curUser], m_turnCardData,
                                          m_turnCardCount, OutCardResult, lastHand) == false)
            {
                OnUserPassCard(m_curUser);
                return;
            }
        }
        else
        {// 最后一手出完
            tagOutCardResult OutCardResult;
            LOG_TAB_DEBUG("检测最后一手出完");
            //搜索炸弹
            bool bNeedAuto = true;
            if (m_gameLogic.SearchBombOutCard(m_handCardData[m_curUser], m_handCardCount[m_curUser],4,
                                              OutCardResult) && m_handCardCount[m_curUser] > 4)
            {//有炸弹，多余4张不自动出
                LOG_TAB_DEBUG("有炸弹不自动接完");
                bNeedAuto = false;
            }
            //搜索扑克
            uint8_t cardType = m_gameLogic.GetCardType(m_handCardData[m_curUser], m_handCardCount[m_curUser],
                                                       IsCanLessOutCard(m_curUser, m_handCardCount[m_curUser]));
            //类型判断
            if (cardType != CT_ERROR && bNeedAuto)
            {//一手出完
                LOG_TAB_DEBUG("最后一手自动---出完--{}", m_curUser);
                OnUserOutCard(m_curUser, m_handCardData[m_curUser], m_handCardCount[m_curUser]);
                return;
            }
        }
    }

// 出牌时间跟叫地主时间
    uint32_t CGameKuaipaoTable::GetOutCardTime() {
        return 20 * 1000;
    }

// 发送场景信息(断线重连)
    void CGameKuaipaoTable::SendGameScene(std::shared_ptr<CGamePlayer> pPlayer) {
        LOG_TAB_DEBUG("send game scene:{}", pPlayer->GetUID());
        uint16_t chairID = GetChairID(pPlayer);
        CHECK_LOG_VOID(chairID != 0xFF, "玩家不在座位上:{}", pPlayer->GetUID());

        if (GetGameState() == net::TABLE_STATE_FREE)
        {

            return;
        }
        else if (GetGameState() == net::TABLE_STATE_PLAY || GetGameState() == net::TABLE_STATE_CALL)
        {
            net::msg_kuaipao_game_info_rep msg;

            msg.set_banker_user(m_bankerUser);
            msg.set_cur_user(m_curUser);
            msg.set_turn_winer(m_turnWiner);
            msg.set_oper_count(m_operCount[chairID]);

            // 排序
            uint8_t tmpCardData[MAX_KUAIPAO_COUNT];   // 出牌数据
            memset(tmpCardData, 0, sizeof(tmpCardData));
            memcpy(tmpCardData, m_turnCardData, m_turnCardCount);
            if (m_turnCardType >= CT_THREE_LINE)
            {
                m_gameLogic.SortCardListByCardNum(tmpCardData, m_turnCardCount);
            }
            for (uint8_t i = 0; i < m_turnCardCount; ++i)
            {
                msg.add_turn_card_data(tmpCardData[i]);
            }
            for (uint8_t i = 0; i < m_playerCount; ++i)
            {
                msg.add_hand_card_count(m_handCardCount[i]);
                msg.add_bomb_scores(m_gameScore[i]);
                msg.add_piao_scores(m_piaoScore[i]);
            }
            for (uint8_t i = 0; i < m_handCardCount[chairID]; ++i)
            {
                msg.add_hand_card_data(m_handCardData[chairID][i]);
            }
            msg.set_game_state(GetGameState());
            msg.set_wait_time(m_coolLogic.getCoolTick());

            pPlayer->SendMsgToClient(&msg, net::S2C_MSG_KUAIPAO_GAME_INFO);

            if (m_curUser == chairID && m_coolLogic.isTimeOut())
            {
                m_coolLogic.beginCooling(2000);//延迟2s
            }
        }
    }

// 重置游戏数据
    void CGameKuaipaoTable::ResetGameData() {
        //游戏变量
        m_bankerUser = INVALID_CHAIR;
        m_curUser = INVALID_CHAIR;
        memset(m_outCardCount, 0, sizeof(m_outCardCount));
        memset(m_operCount, 0, sizeof(m_operCount));
        m_outCardTotal = 0;
        memset(m_eachBombCount, 0, sizeof(m_eachBombCount));
        //出牌信息
        m_turnCardCount = 0;
        m_turnWiner = INVALID_CHAIR;
        m_turnCardType = 0;
        memset(m_turnCardData, 0, sizeof(m_turnCardData));
        //扑克信息
        memset(m_handCardData, 0, sizeof(m_handCardData));
        memset(m_handCardCount, 0, sizeof(m_handCardCount));

        memset(m_gameScore, 0, sizeof(m_gameScore));
        memset(m_piaoScore, INVALID_CHAIR, sizeof(m_piaoScore));

        m_hong10User = INVALID_CHAIR;
        m_autoOutCard = false;

    }

// 洗牌发牌
    void CGameKuaipaoTable::DispatchCard(uint8_t cardNum) {
        vector<uint8_t> poolCards;
        m_gameLogic.GetInitCardList(cardNum, poolCards);
        for (uint32_t j = 0; j < cardNum; ++j)
        {// 分发扑克(单张发)
            for (uint16_t i = 0; i < GAME_KUAIPAO_PLAYER; i++)
            {
                m_handCardData[i][j] = poolCards[0];
                poolCards.erase(poolCards.begin());
            }
        }
        for (uint16_t i = 0; i < GAME_KUAIPAO_PLAYER; i++)
        {//二人跑得快也要发牌
            m_handCardCount[i] = cardNum;
        }
        FlushSortHandCard();
        CountDispBomb();


    }

// 排序手牌
    void CGameKuaipaoTable::FlushSortHandCard() {
        for (uint16_t i = 0; i < GAME_KUAIPAO_PLAYER; i++)
        {
            m_gameLogic.SortCardListByLogicValue(m_handCardData[i], m_handCardCount[i]);
        }
    }

// 统计发牌炸弹数并换牌
    void CGameKuaipaoTable::CountDispBomb() {
        //70%换掉炸弹
        for (uint16_t i = 0; i < m_playerCount; ++i)
        {
            //分析扑克
            tagAnalyseResult AnalyseResult;
            m_gameLogic.AnalysebCardData(m_handCardData[i], m_handCardCount[i], AnalyseResult);
            for (uint8_t j = 0; j < AnalyseResult.bFourCount; ++j)
            {
                if (rand_range(0, PRO_DENO_100) < 70)
                {
                    uint8_t card = AnalyseResult.bFourCardData[j * 4];
                    ChangeUserHandCard(i, card);
                    LOG_TAB_DEBUG("炸弹换牌{}", card);
                }
            }
        }
        FlushSortHandCard();
        //70%换掉飞机
        for (uint16_t i = 0; i < m_playerCount; ++i)
        {
            //分析扑克
            tagAnalyseResult AnalyseResult;
            m_gameLogic.AnalysebCardData(m_handCardData[i], m_handCardCount[i], AnalyseResult);
            if (AnalyseResult.bThreeCount >= 2)
            {
                for (uint8_t j = 0; j < AnalyseResult.bThreeCount - 1; ++j)
                {
                    if (AnalyseResult.bThreeLogicVolue[j] == AnalyseResult.bThreeLogicVolue[j + 1])
                    {
                        if (rand_range(0, PRO_DENO_100) < 70)
                        {
                            uint8_t card = AnalyseResult.bThreeCardData[j * 3];
                            ChangeUserHandCard(i, card);
                            LOG_TAB_DEBUG("飞机换牌{}", card);
                        }
                    }
                }
            }
        }
        FlushSortHandCard();
        //50%换掉3连
/*	for (uint16_t i = 0; i < m_playerCount; ++i)
	{
		//分析扑克
		tagAnalyseResult AnalyseResult;
		m_gameLogic.AnalysebCardData(m_handCardData[i], m_handCardCount[i], AnalyseResult);
		if (AnalyseResult.bDoubleCount >= 3)
		{
			for (uint8 j = 0; j < AnalyseResult.bThreeCount - 2; ++j)
			{
				if (AnalyseResult.bDoubleLogicVolue[j] == AnalyseResult.bDoubleLogicVolue[j + 1]
						&& AnalyseResult.bDoubleLogicVolue[j] == AnalyseResult.bDoubleLogicVolue[j + 2])
				{
					if (g_RandGen.RandRatio(50, PRO_DENO_100))
					{
						uint8 card = AnalyseResult.bDoubleCardData[j*2];
						ChangeUserHandCard(i, card);
						LOG_TAB_DEBUG("三连换牌{}", card);
					}
				}
			}
		}
	}

	FlushSortHandCard();*/
    }

// 换牌
    void CGameKuaipaoTable::ChangeUserHandCard(uint16_t chairID, uint8_t card) {
        uint16_t tarChairID = (chairID + 1) % m_playerCount;
        for (uint8_t i = 0; i < m_handCardCount[chairID]; ++i)
        {
            if (m_handCardData[chairID][i] == card)
            {
                std::swap(m_handCardData[chairID][i], m_handCardData[tarChairID][i]);
            }
        }

    }

// 黑桃3庄家
    uint16_t CGameKuaipaoTable::SetBankerBy3() {
        if (m_fristOutType == emFristOutType_BankerRandAll)
        {
            return svrlib::rand() % m_playerCount;
        }
        if (m_playerCount == 2)
        {//二人场
            if (m_fristOutType == emFristOutType_BankerMin)
            {//最小首出
                return SetBankerMinCard();
            }
            //随机首出
            LOG_TAB_DEBUG("二人随机先出");
            return svrlib::rand() % m_playerCount;
        }
        else
        {
            for (uint16_t i = 0; i < m_playerCount; i++)
            {
                if (IsHaveHei3(i))
                    return i;
            }
            //没人有黑桃3,则随机
            return svrlib::rand() % m_playerCount;
        }
    }

// 最小牌庄家
    uint16_t CGameKuaipaoTable::SetBankerMinCard() {
        uint16_t chairID = 0;
        uint8_t minCard = GetMinCard(0);
        for (uint8_t i = 1; i < m_playerCount; ++i)
        {
            uint8_t tmpCard = GetMinCard(i);
            if (m_gameLogic.GetCardLogicValue(minCard) > m_gameLogic.GetCardLogicValue(tmpCard))
            {
                minCard = tmpCard;
                chairID = i;
            }
            else if (m_gameLogic.GetCardLogicValue(minCard) == m_gameLogic.GetCardLogicValue(tmpCard))
            {
                if (m_gameLogic.GetCardColorValue(minCard) < m_gameLogic.GetCardColorValue(tmpCard))
                {
                    minCard = tmpCard;
                    chairID = i;
                }
            }
        }
        LOG_TAB_DEBUG("最小先出:{}", chairID);
        return chairID;
    }

    uint8_t CGameKuaipaoTable::GetMinCard(uint16_t chairID) {
        uint8_t minCard = m_handCardData[chairID][0];
        for (uint8_t j = 1; j < m_handCardCount[chairID]; j++)
        {
            uint8_t tmpCard = m_handCardData[chairID][j];
            if (m_gameLogic.GetCardLogicValue(minCard) > m_gameLogic.GetCardLogicValue(tmpCard))
            {
                minCard = tmpCard;
            }
            else if (m_gameLogic.GetCardLogicValue(minCard) == m_gameLogic.GetCardLogicValue(tmpCard))
            {
                if (m_gameLogic.GetCardColorValue(minCard) < m_gameLogic.GetCardColorValue(tmpCard))
                {
                    minCard = tmpCard;
                }
            }
        }
        LOG_TAB_DEBUG("最小牌:{:#x}", minCard);
        return minCard;
    }

// 是否有黑桃3
    bool CGameKuaipaoTable::IsHaveHei3(uint16_t chairID) {
        for (uint8_t j = 0; j < m_handCardCount[chairID]; j++)
        {
            if (m_handCardData[chairID][j] == 0x33)
            {
                return true;
            }
        }
        return false;
    }

// 下家手牌数量
    uint16_t CGameKuaipaoTable::GetNextPlayerCardNum(uint16_t chairID) {
        //获取用户
        uint16_t wNextPlayer = (chairID + 1) % m_playerCount;
        return m_handCardCount[wNextPlayer];
    }

    bool CGameKuaipaoTable::IsCanStart() {
        for (uint8_t i = 0; i < m_playerCount; ++i)
        {
            if (m_vecPlayers[i].pPlayer == nullptr || m_vecPlayers[i].readyStatus == 0)
            {
                return false;
            }
        }
        return true;
    }

// 开启飘分
    void CGameKuaipaoTable::StartPiaoScore() {
        LOG_TAB_DEBUG("开启飘分状态");
        net::msg_kuaipao_notify_piao_score msg;
        msg.set_piao_time(10 * 1000);
        TableMsgToAll(&msg, S2C_MSG_KUAIPAO_NOTIFY_PIAO_SCORE);

        SetGameState(TABLE_STATE_CALL);
        m_coolLogic.beginCooling(10 * 1000);
    }

// 能否少带
    bool CGameKuaipaoTable::IsCanLessOutCard(uint16_t chairID, uint8_t cardNum) {
        if (cardNum != m_handCardCount[chairID])return false;
        if (cardNum <= 5)
        {
            if (m_threeOutLess && m_turnCardCount == 0)
            {
                return true;//三张少带出完
            }
            if (m_threePassLess && m_turnCardCount != 0)
            {
                return true;//三张少带接完
            }
        }
        else
        {
            if (m_feijiOutLess && m_turnCardCount == 0)
            {
                return true;//飞机少带出完
            }
            if (m_feijiPassLess && m_turnCardCount != 0)
            {
                return true;//飞机少带接完
            }
        }
        return false;
    }

// 四带
    bool CGameKuaipaoTable::IsCanFourTakeThree() {
        return m_fourTakeThree;
    }

    bool CGameKuaipaoTable::IsCanFourTakeTwo() {
        return m_fourTakeTwo;
    }

// 炸弹
    bool CGameKuaipaoTable::IsCanSpiltBomb() {
        return m_splitBomb;
    }

// 是否飘分
    bool CGameKuaipaoTable::IsOpenPiaoScore() {
        return m_openPiaoScore;
    }

// 检查炸弹飘分
    bool CGameKuaipaoTable::CheckBombCalc() {
        if (m_turnCardType != CT_BOMB)return false;

        //炸弹判断
        int64_t bombScore[GAME_KUAIPAO_PLAYER];
        memset(bombScore, 0, sizeof(bombScore));

        m_eachBombCount[m_turnWiner]++;
        int64_t bScore = 10 * GetBaseScore();

        for (uint16_t i = 0; i < m_playerCount; i++)
        {
            //游戏积分
            if (i != m_turnWiner)
            {
                m_gameScore[m_turnWiner] += bScore;
                bombScore[m_turnWiner] += bScore;
                CalcPlayerGameInfo(GetPlayer(m_turnWiner)->GetUID(), bScore);

                m_gameScore[i] -= bScore;
                bombScore[i] -= bScore;
                //CalcPlayerGameInfo(GetPlayer(i)->GetUID(), -bScore);
            }
        }
        net::msg_kuaipao_bomb_score_rep bombmsg;

        for (uint8_t i = 0; i < m_playerCount; ++i)
        {
            bombmsg.add_bomb_scores(bombScore[i]);
        }
        bombmsg.set_bomb_user(m_turnWiner);
        TableMsgToAll(&bombmsg, net::S2C_MSG_KUAIPAO_BOMB_SCORE);
        FlushSeatValueInfoToClient();
        return true;
    }

// 检查炸弹是否拆了
    bool CGameKuaipaoTable::CheckSplitBomb(uint16_t chairID, uint8_t cardData[], uint8_t cardCount) {
        if (m_splitBomb)return false;

        for (uint8_t j = 0; j < cardCount; ++j)
        {
            uint8_t card = cardData[j];

            if (m_gameLogic.CountCardNum(m_handCardData[chairID], m_handCardCount[chairID], card) == 4)
            {
                if (m_gameLogic.CountCardNum(cardData, cardCount, card) < 4)
                {
                    LOG_TAB_DEBUG("炸弹拆开了:{}", card);
                    return true;
                }
            }
        }

        return false;
    }
};











