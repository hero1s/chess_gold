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

// ��������Ϸ����
    CGameLandTable::CGameLandTable(std::shared_ptr<CGameRoom> pRoom, int64_t tableID)
            : CGameCoinTable(pRoom, tableID) {
        m_vecPlayers.clear();

        //ը������
        m_firstUser = INVALID_CHAIR;
        m_bankerUser = INVALID_CHAIR;
        m_curUser = INVALID_CHAIR;
        memset(m_outCardCount, 0, sizeof(m_outCardCount));
        m_firstUserType = 1;
        m_maxBeiShu = 32;
        //��Ϸ����
        m_bombCount = 0;
        memset(m_eachBombCount, 0, sizeof(m_eachBombCount));

        //�з���Ϣ
        memset(m_callScore, 0, sizeof(m_callScore));
        m_callCount = 0;

        //������Ϣ
        m_turnCardCount = 0;
        m_turnWiner = INVALID_CHAIR;
        memset(m_turnCardData, 0, sizeof(m_turnCardData));

        //�˿���Ϣ
        memset(m_bankerCard, 0, sizeof(m_bankerCard));
        memset(m_handCardData, 0, sizeof(m_handCardData));
        memset(m_handCardCount, 0, sizeof(m_handCardCount));

    }

    CGameLandTable::~CGameLandTable() {

    }

    void CGameLandTable::GetTableFaceInfo(net::table_info *pInfo) {
        GetTableFaceBaseInfo(pInfo);
        //������Ϣ
        pInfo->set_show_hand_num(1);
        pInfo->set_call_time(GetCallScoreTime());
        pInfo->set_card_time(GetOutCardTime());
    }

//��������
    bool CGameLandTable::Init() {
        SetGameState(net::TABLE_STATE_FREE);
        ResetInitSeat(GAME_LAND_PLAYER);
        return true;
    }

//��λ����
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

// ��Ϸ��Ϣ
    int CGameLandTable::OnGameMessage(std::shared_ptr<CGamePlayer> pPlayer, uint16_t cmdID, const uint8_t *pkt_buf, uint16_t buf_len) {
        uint16_t chairID = GetChairID(pPlayer);
        CHECK_LOG_RET(chairID != 0xFF, 0, "��Ҳ�����λ��:{}", pPlayer->GetUID());
        switch (cmdID)
        {
            case net::C2S_MSG_LAND_CALL_SCORE_REQ:// �û��з�
            {
                net::msg_land_call_score_req msg;
                PARSE_MSG_FROM_ARRAY(msg);
                //״̬Ч��
                if (GetGameState() != TABLE_STATE_CALL)
                {
                    LOG_TAB_DEBUG("not call state");
                    return 0;
                }
                //��Ϣ����
                return OnUserCallScore(chairID, msg.call_score());
            }
                break;
            case net::C2S_MSG_LAND_OUT_CARD_REQ:// �û�����
            {
                net::msg_land_out_card_req msg;
                PARSE_MSG_FROM_ARRAY(msg);
                //״̬Ч��
                if (GetGameState() != TABLE_STATE_PLAY)
                {
                    LOG_TAB_DEBUG("not play state");
                    return 0;
                }
                //��Ϣ����
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
            case net::C2S_MSG_LAND_PASS_CARD_REQ:// �û�����
            {
                net::msg_land_pass_card_req msg;
                PARSE_MSG_FROM_ARRAY(msg);
                //״̬Ч��
                if (GetGameState() != TABLE_STATE_PLAY)
                {
                    LOG_TAB_DEBUG("not play state");
                    return 0;
                }
                //��Ϣ����
                return OnUserPassCard(chairID);
            }
                break;
            case net::C2S_MSG_LAND_REQ_HAND_CARD:// ��������
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

// ��Ϸ��ʼ
    bool CGameLandTable::OnGameStart() {
        LOG_TAB_DEBUG("game start");
        // ������Ϣ
        m_turnCardCount = 0;
        m_turnWiner = INVALID_CHAIR;
        memset(m_turnCardData, 0, sizeof(m_turnCardData));

        SetGameState(net::TABLE_STATE_CALL);

        InitBlingLog(true);

        // �����˿�
        ShuffleTableCard();

        // ��ȡ����
        uint8_t validCardData = 0;
        uint8_t validCardIndex = 0;
        uint16_t startUser = m_firstUser;
        m_curUser = m_firstUser;

        //��ȡ�˿�
        validCardIndex = rand_range(0, DISPATCH_COUNT);
        validCardData = m_randCard[validCardIndex];

        //�����û�
        startUser = m_gameLogic.GetCardValue(validCardData) % GAME_LAND_PLAYER;
        m_curUser = DispatchUserCard(startUser, validCardIndex);
        //�����˿�
        for (uint16_t i = 0; i < GAME_LAND_PLAYER; i++)
        {
            m_gameLogic.SortCardList(m_handCardData[i], m_handCardCount[i], ST_ORDER);
        }
        //���õ���
        memcpy(m_bankerCard, &m_randCard[DISPATCH_COUNT], sizeof(m_bankerCard));

        //�����û�
        if (m_callCount > 0 && m_firstUser != INVALID_CHAIR)
        {
            m_curUser = m_firstUser;
            LOG_TAB_DEBUG("���½з�:{}--{}", m_curUser, m_callCount);
        }
        else
        {
            if (m_firstUserType == 2 || m_firstUser == INVALID_CHAIR)
            {
                LOG_TAB_DEBUG("����Ƚ�:{}--{}", m_curUser, m_firstUser);
                m_firstUser = m_curUser;
            }
            else
            {
                LOG_TAB_DEBUG("�ȳ��Ƚ�:{}", m_firstUser);
                m_curUser = m_firstUser;
            }
        }
        LOG_TAB_DEBUG("callCount:{} curuser:{}--firstuser:{}", m_callCount, m_curUser, m_firstUser);
        //��������
        for (uint16_t i = 0; i < GAME_LAND_PLAYER; i++)
        {
            //������Ϣ
            net::msg_land_start_rep gameStart;
            gameStart.set_start_user(startUser);
            gameStart.set_cur_user(m_curUser);
            gameStart.set_valid_card_data(validCardData);
            gameStart.set_valid_card_index(validCardIndex);

            for (uint8_t j = 0; j < m_handCardCount[i]; ++j)
            {
                gameStart.add_card_data(m_handCardData[i][j]);
            }
            //��������
            TableMsgToClient(i, &gameStart, net::S2C_MSG_LAND_START);
        }

        // �����зֶ�ʱ��
        m_coolLogic.beginCooling(GetCallScoreTime());

        //OnSubGameStart();
        return true;
    }

//��Ϸ����
    bool CGameLandTable::OnGameEnd(uint16_t chairID, uint8_t reason) {
        LOG_TAB_DEBUG("game end:{}--{},banker {}", chairID, reason, m_bankerUser);
        m_coolLogic.clearCool();
        switch (reason)
        {
            case GER_NORMAL:        //�������
            {
                net::msg_land_game_over_rep gameOver;
                // ��Ϸ���ּ���

                // ը����Ϣ
                gameOver.set_bomb_count(m_bombCount);
                for (uint8_t i = 0; i < GAME_LAND_PLAYER; ++i)
                {
                    gameOver.add_each_bomb_counts(m_eachBombCount[i]);
                }
                //�û��˿�
                for (uint16_t i = 0; i < GAME_LAND_PLAYER; i++)
                {
                    //�����˿�
                    gameOver.add_card_counts(m_handCardCount[i]);
                    for (uint8_t j = 0; j < m_handCardCount[i]; ++j)
                    {
                        gameOver.add_hand_card_data(m_handCardData[i][j]);
                    }
                    WriteOutCardLog(i, m_handCardData[i], m_handCardCount[i], 1);
                }
                //ը��ͳ��
                int64_t scoreTimes = 1;
                for (uint8_t i = 0; i < m_bombCount; i++)
                {
                    scoreTimes *= 2;
                }
                //�����ж�
                if (chairID == m_bankerUser)
                {
                    //�û�����
                    uint16_t user1 = (m_bankerUser + 1) % GAME_LAND_PLAYER;
                    uint16_t user2 = (m_bankerUser + 2) % GAME_LAND_PLAYER;
                    gameOver.set_chun_tian(0);

                    //�û��ж�
                    if ((m_outCardCount[user1] == 0) && (m_outCardCount[user2] == 0))
                    {
                        scoreTimes *= 2;
                        gameOver.set_chun_tian(1);
                    }
                }
                //�������ж�
                if (chairID != m_bankerUser)
                {
                    gameOver.set_fan_chun_tian(0);
                    if (m_outCardCount[m_bankerUser] == 1)
                    {
                        scoreTimes *= 2;
                        gameOver.set_fan_chun_tian(1);
                    }
                }
                //�зֱ���
                gameOver.set_banker_score(m_callScore[m_bankerUser]);
                scoreTimes *= m_callScore[m_bankerUser];

                //��������
                scoreTimes = std::min(scoreTimes, (int64_t) m_maxBeiShu);
                LOG_TAB_DEBUG("���㷭����:{}", scoreTimes);

                //ͳ�ƻ���
                int64_t calcScore[GAME_LAND_PLAYER];
                int64_t loseNum = 0;
                memset(calcScore, 0, sizeof(calcScore));
                // ������ķ���
                for (uint16_t i = 0; i < GAME_LAND_PLAYER; i++)
                {
                    //��������
                    int64_t userScore = 0;
                    uint8_t isLand = (i == m_bankerUser) ? 1 : 0;
                    //���ֻ���
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
                    //�������
                    userScore = GetBaseScore() * userScore * scoreTimes;
                    calcScore[i] = -userScore;
                    loseNum += abs(calcScore[i]);
                }
                // ����Ӯ�ķ���
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
                    LOG_TAB_DEBUG("���ֲ���Ϊ�������������");
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
                //��������
                TableMsgToAll(&gameOver, net::S2C_MSG_LAND_GAME_OVER);

                //�л��û�
                if (m_firstUserType == 1)
                {
                    LOG_TAB_DEBUG("�л��з��û�:{}", chairID);
                    m_firstUser = chairID;
                }
                //������Ϸ
                ResetGameData();

                SetGameState(TABLE_STATE_GAME_END);
                m_coolLogic.beginCooling(1000);

                OnGameRoundOver();
                return true;
            }
                break;
            case GER_DISMISS:        //��Ϸ��ɢ
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
            case GER_USER_LEAVE:        //�û�ǿ��
            case GER_NETWORK_ERROR:    //�����ж�
            default:
                return true;
        }
        return false;
    }

// ���������˿���
    void CGameLandTable::SendHandleCard(uint16_t chairID) {
        if (chairID >= GAME_LAND_PLAYER)
            return;
        LOG_TAB_DEBUG("��������:{}", chairID);
        //��������
        net::msg_land_hand_card_rep msg;
        msg.set_chair_id(chairID);
        for (uint8_t j = 0; j < m_handCardCount[chairID]; ++j)
        {
            msg.add_card_data(m_handCardData[chairID][j]);
        }

        //��������
        TableMsgToClient(chairID, &msg, net::S2C_MSG_LAND_HAND_CARD);
    }

// �û�����
    bool CGameLandTable::OnUserPassCard(uint16_t chairID) {
        //Ч��״̬
        if (chairID != m_curUser || m_turnCardCount == 0)
        {
            LOG_TAB_ERROR("not you oper:{}", chairID);
            return false;
        }
        LOG_TAB_DEBUG("���pass��{}", chairID);
        //���ñ���
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

// �û��з�
    bool CGameLandTable::OnUserCallScore(uint16_t chairID, uint8_t score) {
        //Ч��״̬
        if (chairID != m_curUser)
        {
            LOG_TAB_DEBUG("you can't oper call:{}--{}", m_curUser, chairID);
            return false;
        }
        LOG_TAB_DEBUG("��ҽз�:{}--{}", chairID, score);
        //Ч�����
        if (score > 3)
            score = 0;

        //���ýз�
        m_callScore[chairID] = score;
        //�����û�
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
                LOG_TAB_DEBUG("���������:{}--{}", m_bankerUser, maxScore);
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

        //���˽з����¿�ʼ
        if (m_firstUser == (chairID + 1) % GAME_LAND_PLAYER)
        {
            LOG_TAB_DEBUG("���˽з����¿�ʼ:firstUser {},bankerUser {}", m_firstUser, m_bankerUser);
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
                    LOG_TAB_DEBUG("���ľֵ�һ�������ׯ:{}", m_firstUser);
                }
            }
        }
        if (m_bankerUser != INVALID_CHAIR)
        {// �е���
            // ����״̬
            SetGameState(TABLE_STATE_PLAY);
            // ������ʱ��ʱ��
            m_coolLogic.beginCooling(GetOutCardTime());
            //���ñ���
            if (m_bankerUser == INVALID_CHAIR)
                m_bankerUser = m_firstUser;

            //���͵���
            m_handCardCount[m_bankerUser] += getArrayLen(m_bankerCard);
            memcpy(&m_handCardData[m_bankerUser][NORMAL_COUNT], m_bankerCard, sizeof(m_bankerCard));

            //�����˿�
            m_gameLogic.SortCardList(m_handCardData[m_bankerUser], m_handCardCount[m_bankerUser], ST_ORDER);

            //�����û�
            m_turnWiner = m_bankerUser;
            m_curUser = m_bankerUser;

            OnGameRoundStart();
            WriteBankerLog(m_bankerUser);

            //������Ϣ
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

// �û�����
    bool CGameLandTable::OnUserOutCard(uint16_t chairID, uint8_t cardData[], uint8_t cardCount) {
        //Ч��״̬
        if (chairID != m_curUser)
        {
            LOG_TAB_ERROR("you can't oper outcard:{}", chairID);
            return false;
        }
        //��ȡ����
        m_gameLogic.SortCardList(cardData, cardCount, ST_ORDER);
        uint8_t cardType = m_gameLogic.GetCardType(cardData, cardCount);
        LOG_TAB_DEBUG("��ҳ���:{}", chairID);
        //�����ж�
        if (cardType == CT_ERROR)
        {
            LOG_TAB_ERROR("the card type is error:{}", cardType);
            return false;
        }
        //�����ж�
        if (m_turnCardCount != 0)
        {
            //�Ա��˿�
            if (m_gameLogic.CompareCard(m_turnCardData, cardData, m_turnCardCount, cardCount) == false)
            {
                LOG_TAB_ERROR("the outcard is not handcard:{}", chairID);
                return false;
            }
        }

        //ɾ���˿�
        if (m_gameLogic.RemoveCardList(cardData, cardCount, m_handCardData[chairID], m_handCardCount[chairID]) == false)
        {
            LOG_TAB_ERROR("removecard error:{}", chairID);
            return false;
        }

        //���Ʊ���
        m_outCardCount[chairID]++;

        //���ñ���
        m_turnCardCount = cardCount;
        m_handCardCount[chairID] -= cardCount;
        memcpy(m_turnCardData, cardData, sizeof(uint8_t) * cardCount);

        //ը���ж�
        if ((cardType == CT_BOMB_CARD) || (cardType == CT_MISSILE_CARD))
        {
            m_bombCount++;
            m_eachBombCount[chairID]++;
        }

        //�л��û�
        m_turnWiner = chairID;
        if (m_handCardCount[chairID] != 0)
        {
            m_curUser = (m_curUser + 1) % GAME_LAND_PLAYER;
        }
        else
        {
            m_curUser = INVALID_CHAIR;
        }

        // ��������
        net::msg_land_out_card_rep outCard;
        outCard.set_out_card_user(chairID);
        outCard.set_cur_user(m_curUser);
        for (uint8_t i = 0; i < m_turnCardCount; ++i)
        {
            outCard.add_card_data(m_turnCardData[i]);
        }
        TableMsgToAll(&outCard, net::S2C_MSG_LAND_OUT_CARD);

        WriteOutCardLog(chairID, m_turnCardData, m_turnCardCount, 0);

        //�����ж�
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
        if (m_turnCardCount == 0)// ��������
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

// �йܳ���
    void CGameLandTable::OnUserAutoCard() {
        if (m_turnCardCount == 0)// �Զ�����
        {
            uint8_t outCard[MAX_LAND_COUNT];
            memset(outCard, 0, sizeof(outCard));
            outCard[0] = m_handCardData[m_curUser][m_handCardCount[m_curUser] - 1];// ����Сһ��
            OnUserOutCard(m_curUser, outCard, 1);
        }
        else
        {
            //�����˿�
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

// ����ʱ����е���ʱ��
    uint32_t CGameLandTable::GetCallScoreTime() {
        return 25 * 1000;
    }

    uint32_t CGameLandTable::GetOutCardTime() {
        return 20 * 1000;
    }

// ���ͳ�����Ϣ(��������)
    void CGameLandTable::SendGameScene(std::shared_ptr<CGamePlayer> pPlayer) {
        LOG_TAB_DEBUG("send game scene:{}", pPlayer->GetUID());
        uint16_t chairID = GetChairID(pPlayer);
        CHECK_LOG_VOID(chairID != 0xFF, "��Ҳ�����λ��:{}", pPlayer->GetUID());
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
                LOG_TAB_DEBUG("��������{}--������{}", m_handCardCount[i], m_callScore[i]);
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

// ��Ϸ���¿�ʼ
    void CGameLandTable::ReGameStart() {
        LOG_TAB_DEBUG("restart game {}", m_callCount);
        m_callCount++;
        m_firstUser = (m_firstUser + 1) % GAME_LAND_PLAYER;
        ResetGameData();
        SetGameState(TABLE_STATE_FREE);
        OnGameStart();
    }

// ������Ϸ����
    void CGameLandTable::ResetGameData() {
        //��Ϸ����
        m_bombCount = 0;
        m_bankerUser = INVALID_CHAIR;
        m_curUser = INVALID_CHAIR;
        memset(m_outCardCount, 0, sizeof(m_outCardCount));
        memset(m_eachBombCount, 0, sizeof(m_eachBombCount));
        //�з���Ϣ
        memset(m_callScore, 0, sizeof(m_callScore));
        //������Ϣ
        m_turnCardCount = 0;
        m_turnWiner = INVALID_CHAIR;
        memset(m_turnCardData, 0, sizeof(m_turnCardData));
        //�˿���Ϣ
        memset(m_bankerCard, 0, sizeof(m_bankerCard));
        memset(m_handCardData, 0, sizeof(m_handCardData));
        memset(m_handCardCount, 0, sizeof(m_handCardCount));

    }

// �����˿�
    void CGameLandTable::ReInitPoker() {
        m_gameLogic.RandCardList(m_randCard, getArrayLen(m_randCard));
    }

// ϴ��
    void CGameLandTable::ShuffleTableCard() {
        ReInitPoker();
    }

// ���Ʋ����ص�ǰ���λ��
    uint16_t CGameLandTable::DispatchUserCard(uint16_t startUser, uint32_t cardIndex) {
        LOG_TAB_DEBUG("���ƣ�{}--{}", startUser, cardIndex);
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

// AI ����
//��Ϸ��ʼ
    bool CGameLandTable::OnSubGameStart() {
        for (uint16_t wChairID = 0; wChairID < GAME_LAND_PLAYER; ++wChairID)
        {
            m_gameLogic.SetUserCard(wChairID, m_handCardData[wChairID], NORMAL_COUNT);
            //�����˿�
            m_gameLogic.SetLandScoreCardData(wChairID, m_handCardData[wChairID], MAX_LAND_COUNT);

        }

        return true;
    }

//ׯ����Ϣ
    bool CGameLandTable::OnSubBankerInfo() {
        //���õ���
        m_gameLogic.SetBackCard(m_bankerUser, m_bankerCard, 3);
        m_gameLogic.SetBanker(m_bankerUser);
        m_callCount = 0;
        return true;
    }

//�û�����
    bool CGameLandTable::OnSubOutCard() {
        //���ñ���
        m_gameLogic.RemoveUserCardData(m_turnWiner, m_turnCardData, m_turnCardCount);

        return true;
    }

    bool CGameLandTable::OnRobotCallScore() {
        uint8_t score = m_gameLogic.LandScore(m_curUser, 0);

//        int bidNumber = m_ddzAIRobot[m_curUser].GetBidNumber();
//        LOG_DEBUG("��????��?��??????��???:%d",bidNumber);
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















