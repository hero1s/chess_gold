

#include "kuaipao_logic.h"
#include "math/random.hpp"
#include "game_kuaipao_table.h"

using namespace game_kuaipao;
//��̬����
namespace game_kuaipao {

//���캯��
    CKuaipaoLogic::CKuaipaoLogic() {
    }

//��������
    CKuaipaoLogic::~CKuaipaoLogic() {
    }

    void CKuaipaoLogic::SetKuaiPaoTable(CGameKuaipaoTable *pTable) {
        m_pHostTable = pTable;
    }

//��ȡ����
    uint8_t CKuaipaoLogic::GetCardType(uint8_t bCardData[], uint8_t bCardCount, bool lastHand) {
        //������
        switch (bCardCount)
        {
            case 1: //����
            {
                return CT_SINGLE;
            }
            case 2: //����
            {
                return (GetCardLogicValue(bCardData[0]) == GetCardLogicValue(bCardData[1])) ? CT_DOUBLE_LINE : CT_ERROR;
            }
        }
        LOG_DEBUG("GetCardType,lastHand: {}", lastHand);

        //�����˿�
        tagAnalyseResult AnalyseResult;
        AnalysebCardData(bCardData, bCardCount, AnalyseResult);

        //ը���ж�
        if ((AnalyseResult.bFourCount == 1) && (bCardCount == 4)) return CT_BOMB;

        //�����ж�
        if (m_pHostTable->IsCanFourTakeThree())
        {
            if ((AnalyseResult.bFourCount == 1) && (bCardCount == 7)) return CT_FOUR_TAKE_THREE;
        }
        if (m_pHostTable->IsCanFourTakeTwo())
        {
            if ((AnalyseResult.bFourCount == 1) && (bCardCount == 6)) return CT_FOUR_TAKE_TWO;
        }
        //ը�����ɲ�
        if (AnalyseResult.bFourCount > 0 && bCardCount > 4 && !m_pHostTable->IsCanSpiltBomb())
        {
            LOG_ERROR("ը�����ɲ�");
            return CT_ERROR;
        }

        //ը���濪
        SpiltBombToThree(AnalyseResult);

        //�����ж�
        if (AnalyseResult.bThreeCount > 0)
        {
            //�����ж�
            uint8_t lines = 0, value = 0;
            GetThreeLine(AnalyseResult, lines, value);

            //�����ж�
            LOG_DEBUG("{}��˳--cardvalue:{}", lines, value);
            uint8_t bSignedCount = bCardCount - lines * 3;
            //���һ��
            if (lastHand && (lines * 5) >= bCardCount)
            {
                LOG_DEBUG("���һ�ַ���:{}--{}", lines, bSignedCount);
                return CT_THREE_LINE_TAKE_DOUBLE;
            }
            for (uint8_t i = 0; i < lines; ++i)
            {
                if ((lines - i) * 5 == bCardCount)
                {
                    return CT_THREE_LINE_TAKE_DOUBLE;
                }
            }

        }

        //�����ж�
        if (AnalyseResult.bDoubleCount > 0)
        {
            //�����ж�
            bool bSeriesCard = false;
            if ((AnalyseResult.bDoubleCount == 1) || (AnalyseResult.bDoubleLogicVolue[0] != 15))
            {
                uint8_t i = 1;
                for (i = 1; i < AnalyseResult.bDoubleCount; i++)
                {
                    if (AnalyseResult.bDoubleLogicVolue[i] != (AnalyseResult.bDoubleLogicVolue[0] - i)) break;
                }
                if (i == AnalyseResult.bDoubleCount) bSeriesCard = true;
            }

            //�����ж�
            if ((bSeriesCard == true) && (AnalyseResult.bDoubleCount * 2 == bCardCount)) return CT_DOUBLE_LINE;
        }

        //�����ж�
        if ((AnalyseResult.bSignedCount >= 5) && (AnalyseResult.bSignedCount == bCardCount))
        {
            //��������
            bool bSeriesCard = false;
            uint8_t bLogicValue = GetCardLogicValue(bCardData[0]);

            //�����ж�
            if (bLogicValue != 15)
            {
                uint8_t i = 1;
                for (i = 1; i < AnalyseResult.bSignedCount; i++)
                {
                    if (GetCardLogicValue(bCardData[i]) != (bLogicValue - i)) break;
                }
                if (i == AnalyseResult.bSignedCount) bSeriesCard = true;
            }

            //�����ж�
            if (bSeriesCard == true) return CT_SINGLE_LINE;
        }
        LOG_DEBUG("��������");
        return CT_ERROR;
    }

//��ó�ʼ�˿�
    void CKuaipaoLogic::GetInitCardList(uint8_t cardNum, vector<uint8_t> &poolCards) {
        poolCards.clear();
        AddFullPokerCard(poolCards);
        if (cardNum == 16)
        {//16���淨//ȥ��2�� 1��A 3��2
            vector<uint8_t> move_cards = {0x4E, 0x4F, 0x01, 0x02, 0x12, 0x22};
            RemoveCardList(move_cards, poolCards);
        }
        else
        {//15���淨//ȥ��3��2,3��A,1��K
            vector<uint8_t> move_cards = {0x4E, 0x4F, 0x01, 0x11, 0x21, 0x02, 0x12, 0x22, 0x3D};
            RemoveCardList(move_cards, poolCards);
        }
        std::random_shuffle(poolCards.begin(), poolCards.end());
        std::random_shuffle(poolCards.begin(), poolCards.end());
    }

//�Ա��˿�
    int CKuaipaoLogic::CompareCard(uint8_t bFirstList[], uint8_t bNextList[], uint8_t bFirstCount, uint8_t bNextCount,
                                   bool firstLastHand, bool nextlastHand) {
        //��ȡ����
        SortCardListByLogicValue(bNextList, bNextCount);
        SortCardListByLogicValue(bFirstList, bFirstCount);

        uint8_t bNextType = GetCardType(bNextList, bNextCount, nextlastHand);
        uint8_t bFirstType = GetCardType(bFirstList, bFirstCount, firstLastHand);

        LOG_DEBUG("CompareCard:{}--{}--{}--{}", bFirstType, bNextType, firstLastHand, nextlastHand);
        //�����ж�
        if (bFirstType == CT_ERROR) return -1;

        //ը���ж�
        if ((bFirstType == CT_BOMB) && (bNextType != CT_BOMB)) return TRUE;
        if ((bFirstType != CT_BOMB) && (bNextType == CT_BOMB)) return FALSE;

        //�����ж�
        if (bFirstType != bNextType)return -1;
        if (bFirstCount != bNextCount)
        {//ֻ�����һ�����������ٴ�
            if (bFirstType != CT_THREE_LINE && bFirstType != CT_THREE_LINE_TAKE_DOUBLE
                && bFirstType != CT_THREE_LINE_TAKE_SINGLE)
            {
                return -1;
            }
            if (!firstLastHand && !nextlastHand)
                return -1;
        }

        //��ʼ�Ա�
        switch (bNextType)
        {
            case CT_BOMB:
            case CT_SINGLE:
            case CT_SINGLE_LINE:
            case CT_DOUBLE_LINE:
            {
                uint8_t bNextLogicValue = GetCardLogicValue(bNextList[0]);
                uint8_t bFirstLogicValue = GetCardLogicValue(bFirstList[0]);
                return bFirstLogicValue > bNextLogicValue ? TRUE : FALSE;
            }
            case CT_THREE_LINE:
            case CT_THREE_LINE_TAKE_SINGLE:
            case CT_THREE_LINE_TAKE_DOUBLE:
            {
                tagAnalyseResult NextResult;
                tagAnalyseResult FirstResult;
                AnalysebCardData(bNextList, bNextCount, NextResult);
                AnalysebCardData(bFirstList, bFirstCount, FirstResult);

                //ը���濪
                SpiltBombToThree(FirstResult);
                SpiltBombToThree(NextResult);

                uint8_t lines1 = 0, value1 = 0;
                GetThreeLine(FirstResult, lines1, value1);
                uint8_t lines2 = 0, value2 = 0;
                GetThreeLine(NextResult, lines2, value2);

                return (value1 > value2 && lines1 == lines2) ? TRUE : FALSE;

                //return FirstResult.bThreeLogicVolue[0]>NextResult.bThreeLogicVolue[0] ? TRUE : FALSE;
            }
            case CT_FOUR_TAKE_THREE:
            {
                tagAnalyseResult NextResult;
                tagAnalyseResult FirstResult;
                AnalysebCardData(bNextList, bNextCount, NextResult);
                AnalysebCardData(bFirstList, bFirstCount, FirstResult);
                return FirstResult.bFourLogicVolue[0] > NextResult.bFourLogicVolue[0] ? TRUE : FALSE;
            }
        }

        return FALSE;
    }

//�����ж�
    bool CKuaipaoLogic::SearchOutCard(uint8_t bCardData[], uint8_t bCardCount, uint8_t bTurnCardData[], uint8_t bTurnCardCount,
                                      tagOutCardResult &OutCardResult, bool lastHand) {
        //��������
        uint8_t bTurnOutType = GetCardType(bTurnCardData, bTurnCardCount, false);

        //���ý��
        OutCardResult.cbCardCount = 0;

        //��������
        switch (bTurnOutType)
        {
            case CT_ERROR:                    //��������
            {
                //��ȡ��ֵ
                uint8_t bLogicValue = GetCardLogicValue(bCardData[bCardCount - 1]);

                //�����ж�
                uint8_t cbSameCount = 1;
                for (uint8_t i = 1; i < bCardCount; i++)
                {
                    if (GetCardLogicValue(bCardData[bCardCount - i - 1]) == bLogicValue)
                        cbSameCount++;
                    else break;
                }

                //��ɴ���
                if (cbSameCount > 1)
                {
                    OutCardResult.cbCardCount = cbSameCount;
                    for (uint8_t j = 0; j < cbSameCount; j++)
                    {
                        OutCardResult.cbResultCard[j] = bCardData[bCardCount - 1 - j];
                    }
                    return true;
                }

                //���ƴ���
                OutCardResult.cbCardCount = 1;
                OutCardResult.cbResultCard[0] = bCardData[bCardCount - 1];

                return true;
            }
            case CT_SINGLE:                    //��������
            {
                //��ȡ��ֵ
                uint8_t bLogicValue = GetCardLogicValue(bTurnCardData[0]);

                //�Աȴ�С
                for (uint8_t i = 0; i < bCardCount; i++)
                {
                    if (GetCardLogicValue(bCardData[bCardCount - i - 1]) > bLogicValue)
                    {
                        OutCardResult.cbCardCount = 1;
                        OutCardResult.cbResultCard[0] = bCardData[bCardCount - i - 1];
                        return true;
                    }
                }

                break;
            }
            case CT_SINGLE_LINE:            //��������
            {
                //�����ж�
                if (bCardCount < bTurnCardCount) break;

                //��ȡ��ֵ
                uint8_t bLogicValue = GetCardLogicValue(bTurnCardData[0]);

                //��������
                for (uint8_t i = (bTurnCardCount - 1); i < bCardCount; i++)
                {
                    //��ȡ��ֵ
                    uint8_t bHandLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);

                    //�����ж�
                    if (bHandLogicValue >= 15) break;
                    if (bHandLogicValue <= bLogicValue) continue;

                    //��������
                    uint8_t bLineCount = 0;
                    for (uint8_t j = (bCardCount - i - 1); j < bCardCount; j++)
                    {
                        if ((GetCardLogicValue(bCardData[j]) + bLineCount) == bHandLogicValue)
                        {
                            //��������
                            OutCardResult.cbResultCard[bLineCount++] = bCardData[j];

                            //����ж�
                            if (bLineCount == bTurnCardCount)
                            {
                                OutCardResult.cbCardCount = bTurnCardCount;
                                return true;
                            }
                        }
                    }
                }

                break;
            }
            case CT_DOUBLE_LINE:    //��������
            {
                //�����ж�
                if (bCardCount < bTurnCardCount) break;

                //��ȡ��ֵ
                uint8_t bLogicValue = GetCardLogicValue(bTurnCardData[0]);

                //��������
                for (uint8_t i = (bTurnCardCount - 1); i < bCardCount; i++)
                {
                    //��ȡ��ֵ
                    uint8_t bHandLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);

                    //�����ж�
                    if (bHandLogicValue <= bLogicValue) continue;
                    if ((bTurnCardCount > 2) && (bHandLogicValue >= 15)) break;

                    //��������
                    uint8_t bLineCount = 0;
                    for (uint8_t j = (bCardCount - i - 1); j < (bCardCount - 1); j++)
                    {
                        if (((GetCardLogicValue(bCardData[j]) + bLineCount) == bHandLogicValue)
                            && ((GetCardLogicValue(bCardData[j + 1]) + bLineCount) == bHandLogicValue))
                        {
                            //��������
                            OutCardResult.cbResultCard[bLineCount * 2] = bCardData[j];
                            OutCardResult.cbResultCard[(bLineCount++) * 2 + 1] = bCardData[j + 1];

                            //����ж�
                            if (bLineCount * 2 == bTurnCardCount)
                            {
                                OutCardResult.cbCardCount = bTurnCardCount;
                                return true;
                            }
                        }
                    }
                }

                break;
            }
            case CT_THREE_LINE:                //��������
            case CT_THREE_LINE_TAKE_SINGLE:    //����һ��
            case CT_THREE_LINE_TAKE_DOUBLE:    //����һ��
            {
                //�����ж�
                if (bCardCount < bTurnCardCount) break;

                //������ֵ
                tagAnalyseResult turnCardResult;
                AnalysebCardData(bTurnCardData, bTurnCardCount, turnCardResult);
                SpiltBombToThree(turnCardResult);
                uint8_t lines = 0, value = 0;
                GetThreeLine(turnCardResult, lines, value);
                LOG_DEBUG("������������:lines:{}--value:{}", lines, value);
                uint8_t bTurnLineCount = lines;
                uint8_t bLogicValue = value;

                //��������
                for (uint8_t i = bTurnLineCount * 3 - 1; i < bCardCount; i++)
                {
                    //��ȡ��ֵ
                    uint8_t bHandLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);

                    //�����ж�
                    if (bHandLogicValue <= bLogicValue) continue;
                    if ((bTurnLineCount > 1) && (bHandLogicValue >= 15)) break;

                    //��������
                    uint8_t bLineCount = 0;
                    for (uint8_t j = (bCardCount - i - 1); j < (bCardCount - 2); j++)
                    {
                        //�����ж�
                        if ((GetCardLogicValue(bCardData[j]) + bLineCount) != bHandLogicValue) continue;
                        if ((GetCardLogicValue(bCardData[j + 1]) + bLineCount) != bHandLogicValue) continue;
                        if ((GetCardLogicValue(bCardData[j + 2]) + bLineCount) != bHandLogicValue) continue;

                        //��������
                        OutCardResult.cbResultCard[bLineCount * 3] = bCardData[j];
                        OutCardResult.cbResultCard[bLineCount * 3 + 1] = bCardData[j + 1];
                        OutCardResult.cbResultCard[(bLineCount++) * 3 + 2] = bCardData[j + 2];

                        //����ж�
                        if (bLineCount == bTurnLineCount)
                        {
                            //��������
                            OutCardResult.cbCardCount = bLineCount * 3;

                            //�����˿�
                            uint8_t bLeftCardData[MAX_COUNT];
                            uint8_t bLeftCount = bCardCount - OutCardResult.cbCardCount;
                            memcpy(bLeftCardData, bCardData, sizeof(uint8_t) * bCardCount);
                            RemoveCardList(OutCardResult.cbResultCard, OutCardResult.cbCardCount, bLeftCardData,
                                           bCardCount);//�Ƴ���������������

                            //�����˿�
                            tagAnalyseResult AnalyseResultLeft;
                            AnalysebCardData(bLeftCardData, bLeftCount, AnalyseResultLeft);

                            //���ƴ���
                            if (bTurnOutType == CT_THREE_LINE_TAKE_SINGLE || bTurnOutType == CT_THREE_LINE_TAKE_DOUBLE)
                            {
                                //��ȡ����
                                for (uint8_t k = 0; k < AnalyseResultLeft.bSignedCount; k++)
                                {
                                    //��ֹ�ж�
                                    if (OutCardResult.cbCardCount == bTurnCardCount) break;

                                    //�����˿�
                                    uint8_t bIndex = k;//AnalyseResultLeft.bSignedCount-k-1;
                                    uint8_t bSignedCard = AnalyseResultLeft.bSignedCardData[bIndex];
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = bSignedCard;
                                }

                                //��ȡ����
                                for (uint8_t k = 0; k < AnalyseResultLeft.bDoubleCount * 2; k++)
                                {
                                    //��ֹ�ж�
                                    if (OutCardResult.cbCardCount == bTurnCardCount) break;

                                    //�����˿�
                                    uint8_t bIndex = k;//(AnalyseResultLeft.bDoubleCount*2-k-1);
                                    uint8_t bSignedCard = AnalyseResultLeft.bDoubleCardData[bIndex];
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = bSignedCard;
                                }

                                //��ȡ����
                                for (uint8_t k = 0; k < AnalyseResultLeft.bThreeCount * 3; k++)
                                {
                                    //��ֹ�ж�
                                    if (OutCardResult.cbCardCount == bTurnCardCount) break;

                                    //�����˿�
                                    uint8_t bIndex = k;//(AnalyseResultLeft.bThreeCount*3-k-1);
                                    uint8_t bSignedCard = AnalyseResultLeft.bThreeCardData[bIndex];
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = bSignedCard;
                                }

                                //��ȡ����
                                for (uint8_t k = 0; k < AnalyseResultLeft.bFourCount * 4; k++)
                                {
                                    //��ֹ�ж�
                                    if (OutCardResult.cbCardCount == bTurnCardCount) break;

                                    //�����˿�
                                    uint8_t bIndex = k;//(AnalyseResultLeft.bFourCount*4-k-1);
                                    uint8_t bSignedCard = AnalyseResultLeft.bFourCardData[bIndex];
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = bSignedCard;
                                }
                            }


                            //����ж�
                            if (OutCardResult.cbCardCount != bTurnCardCount && !lastHand)
                            {
                                OutCardResult.cbCardCount = 0;
                                return false;
                            }

                            return true;
                        }
                    }
                }

                break;
            }
            case CT_BOMB:                        //ը������
            {
                //�����ж�
                if (bCardCount < bTurnCardCount) break;

                //��ȡ��ֵ
                uint8_t bLogicValue = GetCardLogicValue(bTurnCardData[0]);

                //����ը��
                for (uint8_t i = 3; i < bCardCount; i++)
                {
                    //��ȡ��ֵ
                    uint8_t bHandLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);

                    //�����ж�
                    if (bHandLogicValue <= bLogicValue) continue;

                    //ը���ж�
                    uint8_t bTempLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);
                    uint8_t j = 1;
                    for (j = 1; j < 4; j++)
                    {
                        if (GetCardLogicValue(bCardData[bCardCount + j - i - 1]) != bTempLogicValue) break;
                    }
                    if (j != 4) continue;

                    //��ɴ���
                    OutCardResult.cbCardCount = bTurnCardCount;
                    OutCardResult.cbResultCard[0] = bCardData[bCardCount - i - 1];
                    OutCardResult.cbResultCard[1] = bCardData[bCardCount - i];
                    OutCardResult.cbResultCard[2] = bCardData[bCardCount - i + 1];
                    OutCardResult.cbResultCard[3] = bCardData[bCardCount - i + 2];

                    return true;
                }

                return false;
            }
        }

        //ը������
        for (uint8_t i = 3; i < bCardCount; i++)
        {
            //��ȡ��ֵ
            uint8_t bHandLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);

            //ը���ж�
            uint8_t bTempLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);
            uint8_t j = 1;
            for (j = 1; j < 4; j++)
            {
                if (GetCardLogicValue(bCardData[bCardCount + j - i - 1]) != bTempLogicValue) break;
            }
            if (j != 4) continue;

            //��ɴ���
            OutCardResult.cbCardCount = 4;
            OutCardResult.cbResultCard[0] = bCardData[bCardCount - i - 1];
            OutCardResult.cbResultCard[1] = bCardData[bCardCount - i];
            OutCardResult.cbResultCard[2] = bCardData[bCardCount - i + 1];
            OutCardResult.cbResultCard[3] = bCardData[bCardCount - i + 2];

            return true;
        }

        return false;
    }

//���������ж�
    bool CKuaipaoLogic::SearchActionOutCard(uint8_t bCardData[], uint8_t bCardCount, uint16_t nextPlayerCardNum,
                                            tagOutCardResult &OutCardResult) {

        //�ɻ�
        if (SearchThreeTakeTwoOutCard(bCardData, bCardCount, 3, 2, OutCardResult))return true;//3�ɻ�
        if (SearchThreeTakeTwoOutCard(bCardData, bCardCount, 2, 2, OutCardResult))return true;//2�ɻ�
        //������
        if (SearchThreeTakeTwoOutCard(bCardData, bCardCount, 1, 2, OutCardResult))return true;//3��2
        //˳��
        if (SearchDoubleLineOutCard(bCardData, bCardCount, 7, OutCardResult))return true;//7����
        if (SearchDoubleLineOutCard(bCardData, bCardCount, 6, OutCardResult))return true;//6����
        if (SearchDoubleLineOutCard(bCardData, bCardCount, 5, OutCardResult))return true;//5����
        if (SearchDoubleLineOutCard(bCardData, bCardCount, 4, OutCardResult))return true;//4����
        if (SearchDoubleLineOutCard(bCardData, bCardCount, 3, OutCardResult))return true;//3����
        if (SearchDoubleLineOutCard(bCardData, bCardCount, 2, OutCardResult))return true;//2����
        if (SearchSingleLineOutCard(bCardData, bCardCount, OutCardResult))return true;//����˳

        //ը��
        if (SearchBombOutCard(bCardData, bCardCount, 4, OutCardResult))return true;
        //����
        if (SearchBombOutCard(bCardData, bCardCount, 2, OutCardResult))return true;
        //����(�Ƿ񶥴�)
        if (nextPlayerCardNum == 1)
        {//���һ��
            OutCardResult.cbResultCard[0] = bCardData[0];
            OutCardResult.cbCardCount = 1;
        }
        else
        {//��С����
            OutCardResult.cbResultCard[0] = bCardData[bCardCount - 1];
            OutCardResult.cbCardCount = 1;
        }

        return true;
    }

//������������
    bool CKuaipaoLogic::SearchSingleLineOutCard(uint8_t bCardData[], uint8_t bCardCount, tagOutCardResult &OutCardResult) {
        //�����ж�
        if (bCardCount < 5) return false;
        //��ȡ��ֵ
        uint8_t bLogicValue = GetCardLogicValue(bCardData[bCardCount - 1]);

        //��������
        for (uint8_t lineCount = bCardCount; lineCount > 5; --lineCount)
        {
            OutCardResult.cbCardCount = 0;

            for (uint8_t i = (lineCount - 1); i < bCardCount; i++)
            {
                //��ȡ��ֵ
                uint8_t bHandLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);

                //�����ж�
                if (bHandLogicValue >= 15) return false;
                if (bHandLogicValue <= bLogicValue) continue;

                //��������
                uint8_t bLineCount = 0;
                for (uint8_t j = (bCardCount - i - 1); j < bCardCount; j++)
                {
                    if ((GetCardLogicValue(bCardData[j]) + bLineCount) == bHandLogicValue)
                    {
                        //��������
                        OutCardResult.cbResultCard[bLineCount++] = bCardData[j];

                        //����ж�
                        if (bLineCount == lineCount)
                        {
                            OutCardResult.cbCardCount = lineCount;
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    bool CKuaipaoLogic::SearchDoubleLineOutCard(uint8_t bCardData[], uint8_t bCardCount, uint8_t lineNum, tagOutCardResult &OutCardResult) {
        //�����ж�
        if (bCardCount < lineNum * 2) return false;

        //��ȡ��ֵ
        uint8_t bLogicValue = GetCardLogicValue(bCardData[bCardCount - 1]);

        //����2����
        OutCardResult.cbCardCount = 0;
        for (uint8_t i = (lineNum * 2 - 1); i < bCardCount; i++)
        {
            //��ȡ��ֵ
            uint8_t bHandLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);

            //�����ж�
            if (bHandLogicValue <= bLogicValue) continue;
            if (bHandLogicValue >= 15) return false;

            //��������
            uint8_t bLineCount = 0;
            for (uint8_t j = (bCardCount - i - 1); j < (bCardCount - 1); j++)
            {
                if (((GetCardLogicValue(bCardData[j]) + bLineCount) == bHandLogicValue)
                    && ((GetCardLogicValue(bCardData[j + 1]) + bLineCount) == bHandLogicValue))
                {
                    //��������
                    OutCardResult.cbResultCard[bLineCount * 2] = bCardData[j];
                    OutCardResult.cbResultCard[(bLineCount++) * 2 + 1] = bCardData[j + 1];

                    //����ж�
                    if (bLineCount * 2 == lineNum * 2)
                    {
                        OutCardResult.cbCardCount = lineNum * 2;
                        return true;
                    }
                }
            }
        }

        return false;
    }

    //������������ɻ�
    bool CKuaipaoLogic::SearchThreeTakeTwoOutCard(uint8_t bCardData[], uint8_t bCardCount, uint8_t lineNum, uint8_t takeNum, tagOutCardResult &OutCardResult) {
        //�����ж�
        uint8_t bTurnCardCount = lineNum * (3 + takeNum);
        if (bCardCount < bTurnCardCount) return false;

        //������ֵ
        uint8_t bTurnLineCount = lineNum;
        LOG_DEBUG("������������:lines:{}", lineNum);
        //��������
        for (uint8_t i = bTurnLineCount * 3 - 1; i < bCardCount; i++)
        {
            //��ȡ��ֵ
            uint8_t bHandLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);
            //�����ж�
            if ((bTurnLineCount > 1) && (bHandLogicValue >= 15)) break;

            //��������
            uint8_t bLineCount = 0;
            for (uint8_t j = (bCardCount - i - 1); j < (bCardCount - 2); j++)
            {
                //�����ж�
                if ((GetCardLogicValue(bCardData[j]) + bLineCount) != bHandLogicValue) continue;
                if ((GetCardLogicValue(bCardData[j + 1]) + bLineCount) != bHandLogicValue) continue;
                if ((GetCardLogicValue(bCardData[j + 2]) + bLineCount) != bHandLogicValue) continue;

                //��������
                OutCardResult.cbResultCard[bLineCount * 3] = bCardData[j];
                OutCardResult.cbResultCard[bLineCount * 3 + 1] = bCardData[j + 1];
                OutCardResult.cbResultCard[(bLineCount++) * 3 + 2] = bCardData[j + 2];

                //����ж�
                if (bLineCount == bTurnLineCount)
                {
                    //��������
                    OutCardResult.cbCardCount = bLineCount * 3;

                    //�����˿�
                    uint8_t bLeftCardData[MAX_COUNT];
                    uint8_t bLeftCount = bCardCount - OutCardResult.cbCardCount;
                    memcpy(bLeftCardData, bCardData, sizeof(uint8_t) * bCardCount);
                    RemoveCardList(OutCardResult.cbResultCard, OutCardResult.cbCardCount, bLeftCardData,
                                   bCardCount);//�Ƴ���������������

                    //�����˿�
                    tagAnalyseResult AnalyseResultLeft;
                    AnalysebCardData(bLeftCardData, bLeftCount, AnalyseResultLeft);

                    //���ƴ���
                    if (takeNum > 0)
                    {
                        //��ȡ����
                        for (uint8_t k = 0; k < AnalyseResultLeft.bSignedCount; k++)
                        {
                            //��ֹ�ж�
                            if (OutCardResult.cbCardCount == bTurnCardCount) break;

                            //�����˿�
                            uint8_t bIndex = k;//AnalyseResultLeft.bSignedCount-k-1;
                            uint8_t bSignedCard = AnalyseResultLeft.bSignedCardData[bIndex];
                            OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = bSignedCard;
                        }

                        //��ȡ����
                        for (uint8_t k = 0; k < AnalyseResultLeft.bDoubleCount * 2; k++)
                        {
                            //��ֹ�ж�
                            if (OutCardResult.cbCardCount == bTurnCardCount) break;

                            //�����˿�
                            uint8_t bIndex = k;//(AnalyseResultLeft.bDoubleCount*2-k-1);
                            uint8_t bSignedCard = AnalyseResultLeft.bDoubleCardData[bIndex];
                            OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = bSignedCard;
                        }

                        //��ȡ����
                        for (uint8_t k = 0; k < AnalyseResultLeft.bThreeCount * 3; k++)
                        {
                            //��ֹ�ж�
                            if (OutCardResult.cbCardCount == bTurnCardCount) break;

                            //�����˿�
                            uint8_t bIndex = k;//(AnalyseResultLeft.bThreeCount*3-k-1);
                            uint8_t bSignedCard = AnalyseResultLeft.bThreeCardData[bIndex];
                            OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = bSignedCard;
                        }

                        //��ȡ����
                        for (uint8_t k = 0; k < AnalyseResultLeft.bFourCount * 4; k++)
                        {
                            //��ֹ�ж�
                            if (OutCardResult.cbCardCount == bTurnCardCount) break;

                            //�����˿�
                            uint8_t bIndex = k;//(AnalyseResultLeft.bFourCount*4-k-1);
                            uint8_t bSignedCard = AnalyseResultLeft.bFourCardData[bIndex];
                            OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = bSignedCard;
                        }
                    }

                    //����ж�
                    if (OutCardResult.cbCardCount != bTurnCardCount && OutCardResult.cbCardCount != bCardCount)
                    {//�����Ի����һ��
                        OutCardResult.cbCardCount = 0;
                        return false;
                    }

                    return true;
                }
            }
        }
        return false;
    }

//����ը�������
    bool CKuaipaoLogic::SearchBombOutCard(uint8_t bCardData[], uint8_t bCardCount, uint8_t bSameNum, tagOutCardResult &OutCardResult) {
        //�����ж�
        if (bCardCount < bSameNum) return false;

        //����ը��
        for (uint8_t i = bSameNum - 1; i < bCardCount; i++)
        {
            //ը���ж�
            uint8_t bTempLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);
            uint8_t j = 1;
            for (j = 1; j < bSameNum; j++)
            {
                if (GetCardLogicValue(bCardData[bCardCount + j - i - 1]) != bTempLogicValue) break;
            }
            if (j != bSameNum) continue;

            //��ɴ���
            OutCardResult.cbCardCount = bSameNum;
            for (uint8_t k = 0; k < bSameNum; ++k)
            {
                OutCardResult.cbResultCard[k] = bCardData[bCardCount - i - 1 + k];
            }
            return true;
        }

        return false;
    }

//�����˿�
    void CKuaipaoLogic::AnalysebCardData(const uint8_t bCardData[], uint8_t bCardCount, tagAnalyseResult &AnalyseResult) {
        //���ý��
        ZeroMemory(&AnalyseResult, sizeof(AnalyseResult));

        //�˿˷���
        for (uint8_t i = 0; i < bCardCount; i++)
        {
            //��������
            uint8_t bSameCount = 1;
            uint8_t bSameCardData[4] = {bCardData[i], 0, 0, 0};
            uint8_t bLogicValue = GetCardLogicValue(bCardData[i]);

            //��ȡͬ��
            for (int j = i + 1; j < bCardCount; j++)
            {
                //�߼��Ա�
                if (GetCardLogicValue(bCardData[j]) != bLogicValue) break;

                //�����˿�
                bSameCardData[bSameCount++] = bCardData[j];
            }

            //������
            switch (bSameCount)
            {
                case 1:        //����
                {
                    AnalyseResult.bSignedLogicVolue[AnalyseResult.bSignedCount] = bLogicValue;
                    memcpy(&AnalyseResult.bSignedCardData[(AnalyseResult.bSignedCount++) * bSameCount], bSameCardData,
                           bSameCount);
                    break;
                }
                case 2:        //����
                {
                    AnalyseResult.bDoubleLogicVolue[AnalyseResult.bDoubleCount] = bLogicValue;
                    memcpy(&AnalyseResult.bDoubleCardData[(AnalyseResult.bDoubleCount++) * bSameCount], bSameCardData,
                           bSameCount);
                    break;
                }
                case 3:        //����
                {
                    AnalyseResult.bThreeLogicVolue[AnalyseResult.bThreeCount] = bLogicValue;
                    memcpy(&AnalyseResult.bThreeCardData[(AnalyseResult.bThreeCount++) * bSameCount], bSameCardData,
                           bSameCount);
                    break;
                }
                case 4:        //����
                {
                    AnalyseResult.bFourLogicVolue[AnalyseResult.bFourCount] = bLogicValue;
                    memcpy(&AnalyseResult.bFourCardData[(AnalyseResult.bFourCount++) * bSameCount], bSameCardData,
                           bSameCount);
                    break;
                }
            }

            //���õ���
            i += bSameCount - 1;
        }

        return;
    }

//ը��������
    void CKuaipaoLogic::SpiltBombToThree(tagAnalyseResult &AnalyseResult) {
        //ը���濪
        for (uint8_t i = 0; i < AnalyseResult.bFourCount; ++i)
        {
            AnalyseResult.bThreeLogicVolue[AnalyseResult.bThreeCount] = AnalyseResult.bFourLogicVolue[i];
            AnalyseResult.bThreeCount++;
            LOG_DEBUG("��ը��:{}", AnalyseResult.bThreeCount);
            std::sort(AnalyseResult.bThreeLogicVolue, AnalyseResult.bThreeLogicVolue + AnalyseResult.bThreeCount,
                      [&](uint8_t a, uint8_t b)
                      {
                          return a > b;
                      });
        }
    }

//��ȡ��������������ʼֵ
    void CKuaipaoLogic::GetThreeLine(tagAnalyseResult &AnalyseResult, uint8_t &lines, uint8_t &value) {
        lines = 0;
        value = 0;
        if (AnalyseResult.bThreeCount > 0)
        {
            //�����ж�
            bool bSeriesCard = false;
            if ((AnalyseResult.bThreeCount == 1) || (AnalyseResult.bThreeLogicVolue[0] != 15))
            {
                uint8_t i = 1;
                for (i = 1; i < AnalyseResult.bThreeCount; i++)
                {
                    if (AnalyseResult.bThreeLogicVolue[i] != (AnalyseResult.bThreeLogicVolue[0] - i)) break;
                }
                if (i == AnalyseResult.bThreeCount) bSeriesCard = true;
                if (bSeriesCard)
                {
                    lines = AnalyseResult.bThreeCount;
                    value = AnalyseResult.bThreeLogicVolue[0];
                    LOG_DEBUG("{}��˳ value {}", lines, value);
                    return;
                }
                else
                {
                    //���⴦��(����������)
                    if (AnalyseResult.bThreeCount == 3)
                    {
                        if (AnalyseResult.bThreeLogicVolue[0] == (AnalyseResult.bThreeLogicVolue[1] + 1))
                        {
                            lines = 2;
                            value = AnalyseResult.bThreeLogicVolue[0];
                            return;
                        }
                        if (AnalyseResult.bThreeLogicVolue[1] == (AnalyseResult.bThreeLogicVolue[2] + 1))
                        {
                            lines = 2;
                            value = AnalyseResult.bThreeLogicVolue[1];
                            return;
                        }
                    }
                    if (AnalyseResult.bThreeCount == 4)
                    {
                        if (AnalyseResult.bThreeLogicVolue[0] == (AnalyseResult.bThreeLogicVolue[2] + 2))
                        {
                            lines = 3;
                            value = AnalyseResult.bThreeLogicVolue[0];
                            return;
                        }
                        if (AnalyseResult.bThreeLogicVolue[1] == (AnalyseResult.bThreeLogicVolue[3] + 2))
                        {
                            lines = 3;
                            value = AnalyseResult.bThreeLogicVolue[1];
                            return;
                        }
                    }
                    if (AnalyseResult.bThreeCount == 5)
                    {
                        if (AnalyseResult.bThreeLogicVolue[0] == (AnalyseResult.bThreeLogicVolue[2] + 2))
                        {
                            lines = 3;
                            value = AnalyseResult.bThreeLogicVolue[0];
                            return;
                        }
                        if (AnalyseResult.bThreeLogicVolue[1] == (AnalyseResult.bThreeLogicVolue[3] + 2))
                        {
                            lines = 3;
                            value = AnalyseResult.bThreeLogicVolue[1];
                            return;
                        }
                        if (AnalyseResult.bThreeLogicVolue[2] == (AnalyseResult.bThreeLogicVolue[4] + 2))
                        {
                            lines = 3;
                            value = AnalyseResult.bThreeLogicVolue[2];
                            return;
                        }
                    }
                }
            }
            lines = 1;
            value = AnalyseResult.bThreeLogicVolue[0];
            return;
        }
    }

};






