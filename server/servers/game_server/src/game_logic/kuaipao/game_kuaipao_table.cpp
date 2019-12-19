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
// �ܵÿ���Ϸ����
    CGameKuaipaoTable::CGameKuaipaoTable(std::shared_ptr<CGameRoom> pRoom, int64_t tableID)
            : CGameCoinTable(pRoom, tableID) {
        m_vecPlayers.clear();

        m_fourTakeThree = false;
        m_fristOutType = emFristOutType_BankerRandAll;
        m_playCardNum = 16;
        m_hong10FanBei = false;

        m_hong10User = INVALID_CHAIR;

        //ը������
        m_bankerUser = INVALID_CHAIR;
        m_curUser = INVALID_CHAIR;
        memset(m_outCardCount, 0, sizeof(m_outCardCount));
        memset(m_operCount, 0, sizeof(m_operCount));
        m_outCardTotal = 0;
        //��Ϸ����
        memset(m_eachBombCount, 0, sizeof(m_eachBombCount));

        //������Ϣ
        m_turnCardCount = 0;
        m_turnWiner = INVALID_CHAIR;
        memset(m_turnCardData, 0, sizeof(m_turnCardData));

        //�˿���Ϣ
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


//��������
    bool CGameKuaipaoTable::Init() {
        SetGameState(net::TABLE_STATE_FREE);

        ResetInitSeat(GAME_KUAIPAO_PLAYER);
        m_playerCount = GAME_KUAIPAO_PLAYER;
        m_coolLogic.clearCool();

        //��ʼ���淨����


        m_conf.seatNum = 3;//3���淨
        ResetInitSeat(m_conf.seatNum);
        m_playerCount = m_conf.seatNum;
        if (m_conf.playType == KUAIPAO_TYPE_fifteen)
        {// �淨1��15���淨
            m_playCardNum = 15;
        }
        else
        {// �淨2��16���淨
            m_playCardNum = 16;
        }

        m_fristOutType = emFristOutType_BankerRandAll;
        m_fristHei3 = 0;
        //ը�����ɲ�1���� 0����
        m_splitBomb = true;
        //�����Ĵ�����1���� 0����
        m_fourTakeTwo = 1;
        //�����Ĵ�����1���� 0����
        m_fourTakeThree = 1;
        //����10������1���� 0����
        m_hong10FanBei = 0;

        // �����ٴ����꣬1���� 0����
        m_threeOutLess = true;
        // �����ٴ����꣬1���� 0����
        m_threePassLess = true;
        // �ɻ��ٴ����꣬1���� 0����
        m_feijiOutLess = true;
        //�ɻ��ٴ����꣬1���� 0����
        m_feijiPassLess = true;

        return true;
    }

//��λ����
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
                        {// Ʈ��
                            StartPiaoScore();
                        }
                        else
                        {// ֱ�ӿ�ʼ
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

// ��Ϸ��Ϣ
    int CGameKuaipaoTable::OnGameMessage(std::shared_ptr<CGamePlayer> pPlayer, uint16_t cmdID, const uint8_t *pkt_buf, uint16_t buf_len) {
        uint16_t chairID = GetChairID(pPlayer);
        CHECK_LOG_RET(chairID != 0xFF, 0, "��Ҳ�����λ��:{}", pPlayer->GetUID());
        LOG_TAB_DEBUG("message {}--{}", chairID, cmdID);
        switch (cmdID)
        {
            case net::C2S_MSG_KUAIPAO_OUT_CARD_REQ:// �û�����
            {
                net::msg_kuaipao_out_card_req msg;
                PARSE_MSG_FROM_ARRAY(msg);
                //״̬Ч��
                if (GetGameState() != TABLE_STATE_PLAY)
                {
                    LOG_TAB_DEBUG("not play state");
                    return 0;
                }
                //��Ϣ����
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
            case net::C2S_MSG_KUAIPAO_PASS_CARD_REQ:// �û�����
            {
                net::msg_kuaipao_pass_card_req msg;
                PARSE_MSG_FROM_ARRAY(msg);
                //״̬Ч��
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
                //��Ϣ����
                OnUserAutoCard();
                return 0;
                break;
            }
            case net::C2S_MSG_KUAIPAO_PIAO_SCORE_REQ://Ʈ��
            {
                if (GetGameState() != TABLE_STATE_CALL)return 0;

                net::msg_kuaipao_piao_score_req msg;
                PARSE_MSG_FROM_ARRAY(msg);
                LOG_TAB_DEBUG("���Ʈ��:{}--{}", chairID, msg.score());
                return OnUserPiaoScore(chairID, msg.score());
                break;
            }
            default:
                return 0;
        }
        return 0;
    }

// ��Ϸ��ʼ
    bool CGameKuaipaoTable::OnGameStart() {
        LOG_TAB_DEBUG("game start");

        CheckUserPiaoScore();

        // ������Ϣ
        m_turnCardCount = 0;
        m_turnWiner = INVALID_CHAIR;
        memset(m_turnCardData, 0, sizeof(m_turnCardData));

        SetGameState(net::TABLE_STATE_PLAY);

        DispatchCard(m_playCardNum);

        //�����û�
        for (uint16_t i = 0; i < m_playerCount; i++)
        {
            for (uint8_t j = 0; j < m_handCardCount[i]; j++)
            {
                if (m_handCardData[i][j] == 0x2A)
                {// ����10
                    m_hong10User = i;
                }
            }
        }

        if (m_bankerUser == INVALID_CHAIR)
        {
            uint16_t chairID = SetBankerBy3();
            //�����û�
            m_bankerUser = chairID;
            m_curUser = chairID;
        }
        else
        {
            m_curUser = m_bankerUser;
        }

        OnGameRoundStart();
        WriteBankerLog(m_curUser);
        //��������
        for (uint16_t i = 0; i < m_playerCount; i++)
        {
            //������Ϣ
            net::msg_kuaipao_start_rep gameStart;
            gameStart.set_start_user(m_bankerUser);
            gameStart.set_cur_user(m_curUser);

            for (uint8_t j = 0; j < m_handCardCount[i]; ++j)
            {
                gameStart.add_card_data(m_handCardData[i][j]);
            }
            //��������
            TableMsgToClient(i, &gameStart, net::S2C_MSG_KUAIPAO_START);
        }

        // �����зֶ�ʱ��
        m_coolLogic.beginCooling(s_DispatchTime + GetOutCardTime());

        return true;
    }

//��Ϸ����
    bool CGameKuaipaoTable::OnGameEnd(uint16_t chairID, uint8_t reason) {
        LOG_TAB_DEBUG("game end:{}--{}", chairID, reason);
        m_coolLogic.clearCool();
        switch (reason)
        {
            case GER_NORMAL:        //�������
            {
                net::msg_kuaipao_game_over_rep gameOver;
                gameOver.set_hong10_user(0);
                m_operLog["h10"] = -1;
                if (m_hong10FanBei)
                {
                    gameOver.set_hong10_user(m_hong10User);
                    m_operLog["h10"] = m_hong10User;
                }

                //�û��˿�
                for (uint16_t i = 0; i < GAME_KUAIPAO_PLAYER; i++)
                {//�����ܵÿ�ҲҪȫ������
                    //�����˿�
                    gameOver.add_card_counts(m_handCardCount[i]);
                    for (uint8_t j = 0; j < m_handCardCount[i]; ++j)
                    {
                        gameOver.add_hand_card_data(m_handCardData[i][j]);
                    }
                    WriteOutCardLog(i, m_handCardData[i], m_handCardCount[i], 1);
                }


                //ͳ�ƻ���
                int64_t calcScore[GAME_KUAIPAO_PLAYER];
                memset(calcScore, 0, sizeof(calcScore));

                //����ͳ��
                for (uint16_t i = 0; i < m_playerCount; i++)
                {
                    //��Ϸ����
                    int32_t lScoreTimes = 0;
                    int32_t lBaseScore = 0;
                    if (m_handCardCount[i] > 1)
                    {
                        lScoreTimes = (m_handCardCount[i] == m_playCardNum) ? 2L : 1L;
                        lBaseScore = m_handCardCount[i] * GetBaseScore() * lScoreTimes;
                        if (m_hong10FanBei)
                        {//����10����
                            if (i == m_hong10User || chairID == m_hong10User)
                            {
                                lBaseScore = lBaseScore * 2;
                            }
                        }
                        lBaseScore += (m_piaoScore[i] + m_piaoScore[chairID]);//Ʈ��
                        LOG_TAB_DEBUG("�������Ʈ��:{}--{}", m_piaoScore[i], m_piaoScore[chairID]);
                    }
                    calcScore[i] -= lBaseScore;
                    calcScore[chairID] += lBaseScore;
                    gameOver.add_guanmens((m_handCardCount[i] == m_playCardNum) ? 1 : 0);

                }

                if (!CanMinus())
                {
                    LOG_TAB_DEBUG("���ֲ���Ϊ�������������");
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

                //�������
                for (uint16_t i = 0; i < m_playerCount; i++)
                {
                    LOG_TAB_DEBUG("��ҽ���:{}", calcScore[i]);
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
                //��������
                TableMsgToAll(&gameOver, net::S2C_MSG_KUAIPAO_GAME_OVER);
                //������Ϸ
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
            case GER_DISMISS:        //��Ϸ��ɢ
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
            case GER_USER_LEAVE:        //�û�ǿ��
            case GER_NETWORK_ERROR:     //�����ж�
            default:
                return true;
        }
        return false;
    }

// �û�Ʈ��
    bool CGameKuaipaoTable::OnUserPiaoScore(uint16_t chairID, uint8_t score) {
        if (!IsOpenPiaoScore())
        {
            LOG_TAB_DEBUG("Ʈ��δ����,����Ʈ��");
            return false;
        }
        if (m_piaoScore[chairID] != INVALID_CHAIR)
        {
            LOG_TAB_DEBUG("�Ѿ�Ʈ�ֹ���");
            return false;
        }
        m_piaoScore[chairID] = std::min(score, (uint8_t) 10);//�������Ư10��
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
        LOG_TAB_DEBUG("֪ͨƮ��:{}--{}", chairID, m_piaoScore[chairID]);
    }

    void CGameKuaipaoTable::CheckUserPiaoScore() {
        LOG_TAB_DEBUG("������Ʈ����ֵ");
        for (uint8_t i = 0; i < m_playerCount; ++i)
        {
            if (m_piaoScore[i] == INVALID_CHAIR)
            {
                m_piaoScore[i] = 0;
                if (IsOpenPiaoScore())
                {// Ʈ��
                    NotifyPiaoScore(i);
                }
            }
        }
    }

// �û�����
    bool CGameKuaipaoTable::OnUserPassCard(uint16_t chairID) {
        //Ч��״̬
        if (chairID != m_curUser || m_turnCardCount == 0)
        {
            LOG_TAB_ERROR("not you oper:{}", chairID);
            return false;
        }
        //���ñ���
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

// �û�����
    bool CGameKuaipaoTable::OnUserOutCard(uint16_t chairID, uint8_t cardData[], uint8_t cardCount) {
        LOG_TAB_DEBUG("��ҳ���:{}", chairID);
        CCommonLogic::LogCardString(cardData, cardCount);
        //Ч��״̬
        if (chairID != m_curUser)
        {
            LOG_TAB_ERROR("you can't oper outcard:{}", chairID);
            return false;
        }
        //��ȡ����
        m_gameLogic.SortCardListByLogicValue(cardData, cardCount);
        uint8_t cardType = m_gameLogic.GetCardType(cardData, cardCount, IsCanLessOutCard(chairID, cardCount));

        //�����ж�
        if (cardType == CT_ERROR)
        {
            LOG_TAB_ERROR("the card type is error:{}", cardType);
            CCommonLogic::LogCardString(cardData, cardCount);
            return false;
        }
        //�Ƿ�4��3
        if (cardType == CT_FOUR_TAKE_THREE && !m_fourTakeThree)
        {
            LOG_TAB_ERROR("û�п���4��3ѡ��");
            return false;
        }
        //�Ƿ�4��2
        if (cardType == CT_FOUR_TAKE_TWO && !m_fourTakeTwo)
        {
            LOG_TAB_ERROR("û�п���4��2ѡ��");
            return false;
        }
        // ը�����ɲ�
        if (CheckSplitBomb(chairID, cardData, cardCount))
        {
            LOG_TAB_ERROR("ը�������Բ�");
            return false;
        }

        //�׳��ж�
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
//                    LOG_TAB_DEBUG("�׳�Ҫ������3");
//                    return false;
//                }
//            }
//        }
        //�����ض�
        if (true)
        {
            if ((m_handCardCount[chairID] != 0))
            {
                //��ȡ�û�
                uint16_t wNextPlayer = (chairID + 1) % m_playerCount;

                //�����ж�
                if ((cardCount == 1) && (m_handCardCount[wNextPlayer] == 1))
                {
                    if (m_gameLogic.CompareCard(m_handCardData[chairID], cardData, 1, 1, false, false) != FALSE)
                    {
                        LOG_TAB_DEBUG("�����ض�,���ܷ���");
                        return false;
                    }
                }
            }
        }
        //�����ж�
        if (m_turnCardCount != 0)
        {
            //�Ա��˿�
            if (m_gameLogic.CompareCard(cardData, m_turnCardData, cardCount, m_turnCardCount,
                                        IsCanLessOutCard(chairID, cardCount), false) != TRUE)
            {
                LOG_TAB_ERROR("the outcard is not big than last card:{}", chairID);
                CCommonLogic::LogCardString(cardData, cardCount);
                CCommonLogic::LogCardString(m_turnCardData, m_turnCardCount);
                return false;
            }
        }

        //ɾ���˿�
        if (m_gameLogic.RemoveCardList(cardData, cardCount, m_handCardData[chairID], m_handCardCount[chairID]) ==
            false)
        {
            LOG_TAB_ERROR("removecard error:{}", chairID);
            return false;
        }

        //���Ʊ���
        m_outCardCount[chairID]++;
        m_outCardTotal++;
        //���ñ���
        m_turnCardCount = cardCount;
        m_handCardCount[chairID] -= cardCount;
        memcpy(m_turnCardData, cardData, sizeof(uint8_t) * cardCount);
        m_turnCardType = cardType;

        //�л��û�
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

        // ��������
        net::msg_kuaipao_out_card_rep outCard;
        outCard.set_out_card_user(chairID);
        outCard.set_cur_user(m_curUser);
        outCard.set_card_type(m_turnCardType);
        outCard.set_is_pass(isPass ? 1 : 0);

        // ����
        uint8_t tmpCardData[MAX_KUAIPAO_COUNT];   // ��������
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

        //д�������־
        WriteOutCardLog(chairID, tmpCardData, m_turnCardCount, 0);

        //�����ж�
        if (m_curUser == INVALID_CHAIR)
        {
            if (CheckBombCalc())
            {
                LOG_TAB_DEBUG("ը��Ʈ�֣��ӳ�2��");
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

// ����ʧ��
    void CGameKuaipaoTable::NotifyOperFail(uint16_t chairID, uint8_t reason) {
        net::msg_kuaipao_oper_fail_rep rep;
        rep.set_reason(reason);

        TableMsgToClient(chairID, &rep, net::S2C_MSG_KUAIPAO_OPER_FAIL, false);
        LOG_TAB_ERROR("�ظ�����ʧ��:{}", chairID);
    }

    void CGameKuaipaoTable::OnOutCardTimeOut() {
        m_vecPlayers[m_curUser].overTimes++;
        OnUserAutoCard();
    }

// �йܳ���
    void CGameKuaipaoTable::OnUserAutoCard() {
        if (m_turnCardCount == 0)// �Զ�����
        {
            uint8_t outCard[MAX_KUAIPAO_COUNT];
            memset(outCard, 0, sizeof(outCard));
            //�׳��ж�
            if (m_fristHei3 && (m_curUser == m_bankerUser) && (m_handCardCount[m_curUser] == m_playCardNum))
            {
                for (uint8_t i = 0; i < MAX_KUAIPAO_COUNT; i++)
                {
                    if (m_handCardData[m_curUser][i] == 0x33)
                    {
                        outCard[0] = 0x33;
                        OnUserOutCard(m_curUser, outCard, 1);
                        LOG_TAB_DEBUG("�Զ�����1");
                        return;
                    }
                }
            }

            //�����˿�
            tagOutCardResult OutCardResult;
            if (m_gameLogic.SearchActionOutCard(m_handCardData[m_curUser],
                                                m_handCardCount[m_curUser],
                                                GetNextPlayerCardNum(m_curUser),
                                                OutCardResult))
            {
                LOG_TAB_DEBUG("�Զ�����--{}", m_curUser);
                if (OnUserOutCard(m_curUser, OutCardResult.cbResultCard, OutCardResult.cbCardCount) == false)
                {
                    OnUserPassCard(m_curUser);
                }
            }
        }
        else
        {
            //�����˿�
            tagOutCardResult OutCardResult;
            bool lastHand = IsCanLessOutCard(m_curUser, m_handCardCount[m_curUser]);
            if (m_gameLogic.SearchOutCard(m_handCardData[m_curUser], m_handCardCount[m_curUser], m_turnCardData,
                                          m_turnCardCount, OutCardResult, lastHand))
            {
                LOG_TAB_DEBUG("�Զ�����--{}", m_curUser);
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

// ����Ƿ�Ҫ����
    bool CGameKuaipaoTable::CheckUserPass() {
        if (m_curUser == INVALID_CHAIR)return false;
        if (m_turnCardCount == 0)return false;

        tagOutCardResult OutCardResult;
        bool lastHand = IsCanLessOutCard(m_curUser, m_handCardCount[m_curUser]);
        //�����˿�
        if (m_gameLogic.SearchOutCard(m_handCardData[m_curUser], m_handCardCount[m_curUser], m_turnCardData,
                                      m_turnCardCount, OutCardResult, lastHand) == false)
        {
            return true;
        }
        return false;
    }

// һ�ֽ����Զ���(û��ը��)
    void CGameKuaipaoTable::CheckLastHandAutoCard() {
        if (m_curUser == INVALID_CHAIR)return;
        //���߿�ס
        //if (IsDisconnect(m_curUser))return;

        if (m_outCardTotal == 0 && m_coolLogic.getPassTick() < 4000)return;
        if (m_autoOutCard)return;
        m_autoOutCard = true;
        if (m_turnCardCount > 0)
        {// ���һ�ֽ���
            LOG_TAB_DEBUG("������һ�ֽ���");
            tagOutCardResult OutCardResult;

            //����ը��
            bool bNeedAuto = true;
            if (m_gameLogic.SearchBombOutCard(m_handCardData[m_curUser], m_handCardCount[m_curUser], 4,
                                              OutCardResult) && m_handCardCount[m_curUser] > 4)
            {//��ը��������4�Ų��Զ���
                LOG_TAB_DEBUG("��ը�����Զ�����");
                bNeedAuto = false;
            }
            //�����˿�
            bool lastHand = IsCanLessOutCard(m_curUser, m_handCardCount[m_curUser]);
            uint8_t cardType = m_gameLogic.GetCardType(m_handCardData[m_curUser], m_handCardCount[m_curUser], lastHand);
            //�����ж�
            if (cardType != CT_ERROR && bNeedAuto)
            {
                //�Ա��˿�
                if (m_gameLogic.CompareCard(m_handCardData[m_curUser], m_turnCardData, m_handCardCount[m_curUser],
                                            m_turnCardCount, lastHand, false) == TRUE)
                {
                    LOG_TAB_DEBUG("���һ���Զ�---����--{} ���һ�� {}", m_curUser, lastHand);
                    OnUserOutCard(m_curUser, m_handCardData[m_curUser], m_handCardCount[m_curUser]);
                    return;
                }
            }
            //���Ҫ�����Զ���
            //�����˿�
            if (m_gameLogic.SearchOutCard(m_handCardData[m_curUser], m_handCardCount[m_curUser], m_turnCardData,
                                          m_turnCardCount, OutCardResult, lastHand) == false)
            {
                OnUserPassCard(m_curUser);
                return;
            }
        }
        else
        {// ���һ�ֳ���
            tagOutCardResult OutCardResult;
            LOG_TAB_DEBUG("������һ�ֳ���");
            //����ը��
            bool bNeedAuto = true;
            if (m_gameLogic.SearchBombOutCard(m_handCardData[m_curUser], m_handCardCount[m_curUser],4,
                                              OutCardResult) && m_handCardCount[m_curUser] > 4)
            {//��ը��������4�Ų��Զ���
                LOG_TAB_DEBUG("��ը�����Զ�����");
                bNeedAuto = false;
            }
            //�����˿�
            uint8_t cardType = m_gameLogic.GetCardType(m_handCardData[m_curUser], m_handCardCount[m_curUser],
                                                       IsCanLessOutCard(m_curUser, m_handCardCount[m_curUser]));
            //�����ж�
            if (cardType != CT_ERROR && bNeedAuto)
            {//һ�ֳ���
                LOG_TAB_DEBUG("���һ���Զ�---����--{}", m_curUser);
                OnUserOutCard(m_curUser, m_handCardData[m_curUser], m_handCardCount[m_curUser]);
                return;
            }
        }
    }

// ����ʱ����е���ʱ��
    uint32_t CGameKuaipaoTable::GetOutCardTime() {
        return 20 * 1000;
    }

// ���ͳ�����Ϣ(��������)
    void CGameKuaipaoTable::SendGameScene(std::shared_ptr<CGamePlayer> pPlayer) {
        LOG_TAB_DEBUG("send game scene:{}", pPlayer->GetUID());
        uint16_t chairID = GetChairID(pPlayer);
        CHECK_LOG_VOID(chairID != 0xFF, "��Ҳ�����λ��:{}", pPlayer->GetUID());

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

            // ����
            uint8_t tmpCardData[MAX_KUAIPAO_COUNT];   // ��������
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
                m_coolLogic.beginCooling(2000);//�ӳ�2s
            }
        }
    }

// ������Ϸ����
    void CGameKuaipaoTable::ResetGameData() {
        //��Ϸ����
        m_bankerUser = INVALID_CHAIR;
        m_curUser = INVALID_CHAIR;
        memset(m_outCardCount, 0, sizeof(m_outCardCount));
        memset(m_operCount, 0, sizeof(m_operCount));
        m_outCardTotal = 0;
        memset(m_eachBombCount, 0, sizeof(m_eachBombCount));
        //������Ϣ
        m_turnCardCount = 0;
        m_turnWiner = INVALID_CHAIR;
        m_turnCardType = 0;
        memset(m_turnCardData, 0, sizeof(m_turnCardData));
        //�˿���Ϣ
        memset(m_handCardData, 0, sizeof(m_handCardData));
        memset(m_handCardCount, 0, sizeof(m_handCardCount));

        memset(m_gameScore, 0, sizeof(m_gameScore));
        memset(m_piaoScore, INVALID_CHAIR, sizeof(m_piaoScore));

        m_hong10User = INVALID_CHAIR;
        m_autoOutCard = false;

    }

// ϴ�Ʒ���
    void CGameKuaipaoTable::DispatchCard(uint8_t cardNum) {
        vector<uint8_t> poolCards;
        m_gameLogic.GetInitCardList(cardNum, poolCards);
        for (uint32_t j = 0; j < cardNum; ++j)
        {// �ַ��˿�(���ŷ�)
            for (uint16_t i = 0; i < GAME_KUAIPAO_PLAYER; i++)
            {
                m_handCardData[i][j] = poolCards[0];
                poolCards.erase(poolCards.begin());
            }
        }
        for (uint16_t i = 0; i < GAME_KUAIPAO_PLAYER; i++)
        {//�����ܵÿ�ҲҪ����
            m_handCardCount[i] = cardNum;
        }
        FlushSortHandCard();
        CountDispBomb();


    }

// ��������
    void CGameKuaipaoTable::FlushSortHandCard() {
        for (uint16_t i = 0; i < GAME_KUAIPAO_PLAYER; i++)
        {
            m_gameLogic.SortCardListByLogicValue(m_handCardData[i], m_handCardCount[i]);
        }
    }

// ͳ�Ʒ���ը����������
    void CGameKuaipaoTable::CountDispBomb() {
        //70%����ը��
        for (uint16_t i = 0; i < m_playerCount; ++i)
        {
            //�����˿�
            tagAnalyseResult AnalyseResult;
            m_gameLogic.AnalysebCardData(m_handCardData[i], m_handCardCount[i], AnalyseResult);
            for (uint8_t j = 0; j < AnalyseResult.bFourCount; ++j)
            {
                if (rand_range(0, PRO_DENO_100) < 70)
                {
                    uint8_t card = AnalyseResult.bFourCardData[j * 4];
                    ChangeUserHandCard(i, card);
                    LOG_TAB_DEBUG("ը������{}", card);
                }
            }
        }
        FlushSortHandCard();
        //70%�����ɻ�
        for (uint16_t i = 0; i < m_playerCount; ++i)
        {
            //�����˿�
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
                            LOG_TAB_DEBUG("�ɻ�����{}", card);
                        }
                    }
                }
            }
        }
        FlushSortHandCard();
        //50%����3��
/*	for (uint16_t i = 0; i < m_playerCount; ++i)
	{
		//�����˿�
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
						LOG_TAB_DEBUG("��������{}", card);
					}
				}
			}
		}
	}

	FlushSortHandCard();*/
    }

// ����
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

// ����3ׯ��
    uint16_t CGameKuaipaoTable::SetBankerBy3() {
        if (m_fristOutType == emFristOutType_BankerRandAll)
        {
            return svrlib::rand() % m_playerCount;
        }
        if (m_playerCount == 2)
        {//���˳�
            if (m_fristOutType == emFristOutType_BankerMin)
            {//��С�׳�
                return SetBankerMinCard();
            }
            //����׳�
            LOG_TAB_DEBUG("��������ȳ�");
            return svrlib::rand() % m_playerCount;
        }
        else
        {
            for (uint16_t i = 0; i < m_playerCount; i++)
            {
                if (IsHaveHei3(i))
                    return i;
            }
            //û���к���3,�����
            return svrlib::rand() % m_playerCount;
        }
    }

// ��С��ׯ��
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
        LOG_TAB_DEBUG("��С�ȳ�:{}", chairID);
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
        LOG_TAB_DEBUG("��С��:{:#x}", minCard);
        return minCard;
    }

// �Ƿ��к���3
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

// �¼���������
    uint16_t CGameKuaipaoTable::GetNextPlayerCardNum(uint16_t chairID) {
        //��ȡ�û�
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

// ����Ʈ��
    void CGameKuaipaoTable::StartPiaoScore() {
        LOG_TAB_DEBUG("����Ʈ��״̬");
        net::msg_kuaipao_notify_piao_score msg;
        msg.set_piao_time(10 * 1000);
        TableMsgToAll(&msg, S2C_MSG_KUAIPAO_NOTIFY_PIAO_SCORE);

        SetGameState(TABLE_STATE_CALL);
        m_coolLogic.beginCooling(10 * 1000);
    }

// �ܷ��ٴ�
    bool CGameKuaipaoTable::IsCanLessOutCard(uint16_t chairID, uint8_t cardNum) {
        if (cardNum != m_handCardCount[chairID])return false;
        if (cardNum <= 5)
        {
            if (m_threeOutLess && m_turnCardCount == 0)
            {
                return true;//�����ٴ�����
            }
            if (m_threePassLess && m_turnCardCount != 0)
            {
                return true;//�����ٴ�����
            }
        }
        else
        {
            if (m_feijiOutLess && m_turnCardCount == 0)
            {
                return true;//�ɻ��ٴ�����
            }
            if (m_feijiPassLess && m_turnCardCount != 0)
            {
                return true;//�ɻ��ٴ�����
            }
        }
        return false;
    }

// �Ĵ�
    bool CGameKuaipaoTable::IsCanFourTakeThree() {
        return m_fourTakeThree;
    }

    bool CGameKuaipaoTable::IsCanFourTakeTwo() {
        return m_fourTakeTwo;
    }

// ը��
    bool CGameKuaipaoTable::IsCanSpiltBomb() {
        return m_splitBomb;
    }

// �Ƿ�Ʈ��
    bool CGameKuaipaoTable::IsOpenPiaoScore() {
        return m_openPiaoScore;
    }

// ���ը��Ʈ��
    bool CGameKuaipaoTable::CheckBombCalc() {
        if (m_turnCardType != CT_BOMB)return false;

        //ը���ж�
        int64_t bombScore[GAME_KUAIPAO_PLAYER];
        memset(bombScore, 0, sizeof(bombScore));

        m_eachBombCount[m_turnWiner]++;
        int64_t bScore = 10 * GetBaseScore();

        for (uint16_t i = 0; i < m_playerCount; i++)
        {
            //��Ϸ����
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

// ���ը���Ƿ����
    bool CGameKuaipaoTable::CheckSplitBomb(uint16_t chairID, uint8_t cardData[], uint8_t cardCount) {
        if (m_splitBomb)return false;

        for (uint8_t j = 0; j < cardCount; ++j)
        {
            uint8_t card = cardData[j];

            if (m_gameLogic.CountCardNum(m_handCardData[chairID], m_handCardCount[chairID], card) == 4)
            {
                if (m_gameLogic.CountCardNum(cardData, cardCount, card) < 4)
                {
                    LOG_TAB_DEBUG("ը������:{}", card);
                    return true;
                }
            }
        }

        return false;
    }
};











