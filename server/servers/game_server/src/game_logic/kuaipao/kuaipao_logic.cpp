

#include "kuaipao_logic.h"
#include "math/random.hpp"
#include "game_kuaipao_table.h"

using namespace game_kuaipao;
//静态变量
namespace game_kuaipao {

//构造函数
    CKuaipaoLogic::CKuaipaoLogic() {
    }

//析构函数
    CKuaipaoLogic::~CKuaipaoLogic() {
    }

    void CKuaipaoLogic::SetKuaiPaoTable(CGameKuaipaoTable *pTable) {
        m_pHostTable = pTable;
    }

//获取类型
    uint8_t CKuaipaoLogic::GetCardType(uint8_t bCardData[], uint8_t bCardCount, bool lastHand) {
        //简单牌形
        switch (bCardCount)
        {
            case 1: //单牌
            {
                return CT_SINGLE;
            }
            case 2: //对牌
            {
                return (GetCardLogicValue(bCardData[0]) == GetCardLogicValue(bCardData[1])) ? CT_DOUBLE_LINE : CT_ERROR;
            }
        }
        LOG_DEBUG("GetCardType,lastHand: {}", lastHand);

        //分析扑克
        tagAnalyseResult AnalyseResult;
        AnalysebCardData(bCardData, bCardCount, AnalyseResult);

        //炸弹判断
        if ((AnalyseResult.bFourCount == 1) && (bCardCount == 4)) return CT_BOMB;

        //四牌判断
        if (m_pHostTable->IsCanFourTakeThree())
        {
            if ((AnalyseResult.bFourCount == 1) && (bCardCount == 7)) return CT_FOUR_TAKE_THREE;
        }
        if (m_pHostTable->IsCanFourTakeTwo())
        {
            if ((AnalyseResult.bFourCount == 1) && (bCardCount == 6)) return CT_FOUR_TAKE_TWO;
        }
        //炸弹不可拆
        if (AnalyseResult.bFourCount > 0 && bCardCount > 4 && !m_pHostTable->IsCanSpiltBomb())
        {
            LOG_ERROR("炸弹不可拆");
            return CT_ERROR;
        }

        //炸弹叉开
        SpiltBombToThree(AnalyseResult);

        //三牌判断
        if (AnalyseResult.bThreeCount > 0)
        {
            //连牌判断
            uint8_t lines = 0, value = 0;
            GetThreeLine(AnalyseResult, lines, value);

            //带牌判断
            LOG_DEBUG("{}连顺--cardvalue:{}", lines, value);
            uint8_t bSignedCount = bCardCount - lines * 3;
            //最后一手
            if (lastHand && (lines * 5) >= bCardCount)
            {
                LOG_DEBUG("最后一手飞完:{}--{}", lines, bSignedCount);
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

        //两连判断
        if (AnalyseResult.bDoubleCount > 0)
        {
            //连牌判断
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

            //两连判断
            if ((bSeriesCard == true) && (AnalyseResult.bDoubleCount * 2 == bCardCount)) return CT_DOUBLE_LINE;
        }

        //单连判断
        if ((AnalyseResult.bSignedCount >= 5) && (AnalyseResult.bSignedCount == bCardCount))
        {
            //变量定义
            bool bSeriesCard = false;
            uint8_t bLogicValue = GetCardLogicValue(bCardData[0]);

            //连牌判断
            if (bLogicValue != 15)
            {
                uint8_t i = 1;
                for (i = 1; i < AnalyseResult.bSignedCount; i++)
                {
                    if (GetCardLogicValue(bCardData[i]) != (bLogicValue - i)) break;
                }
                if (i == AnalyseResult.bSignedCount) bSeriesCard = true;
            }

            //单连判断
            if (bSeriesCard == true) return CT_SINGLE_LINE;
        }
        LOG_DEBUG("错误牌型");
        return CT_ERROR;
    }

//获得初始扑克
    void CKuaipaoLogic::GetInitCardList(uint8_t cardNum, vector<uint8_t> &poolCards) {
        poolCards.clear();
        AddFullPokerCard(poolCards);
        if (cardNum == 16)
        {//16张玩法//去掉2王 1个A 3个2
            vector<uint8_t> move_cards = {0x4E, 0x4F, 0x01, 0x02, 0x12, 0x22};
            RemoveCardList(move_cards, poolCards);
        }
        else
        {//15张玩法//去掉3个2,3个A,1个K
            vector<uint8_t> move_cards = {0x4E, 0x4F, 0x01, 0x11, 0x21, 0x02, 0x12, 0x22, 0x3D};
            RemoveCardList(move_cards, poolCards);
        }
        std::random_shuffle(poolCards.begin(), poolCards.end());
        std::random_shuffle(poolCards.begin(), poolCards.end());
    }

//对比扑克
    int CKuaipaoLogic::CompareCard(uint8_t bFirstList[], uint8_t bNextList[], uint8_t bFirstCount, uint8_t bNextCount,
                                   bool firstLastHand, bool nextlastHand) {
        //获取类型
        SortCardListByLogicValue(bNextList, bNextCount);
        SortCardListByLogicValue(bFirstList, bFirstCount);

        uint8_t bNextType = GetCardType(bNextList, bNextCount, nextlastHand);
        uint8_t bFirstType = GetCardType(bFirstList, bFirstCount, firstLastHand);

        LOG_DEBUG("CompareCard:{}--{}--{}--{}", bFirstType, bNextType, firstLastHand, nextlastHand);
        //类型判断
        if (bFirstType == CT_ERROR) return -1;

        //炸弹判断
        if ((bFirstType == CT_BOMB) && (bNextType != CT_BOMB)) return TRUE;
        if ((bFirstType != CT_BOMB) && (bNextType == CT_BOMB)) return FALSE;

        //规则判断
        if (bFirstType != bNextType)return -1;
        if (bFirstCount != bNextCount)
        {//只有最后一手三连可以少带
            if (bFirstType != CT_THREE_LINE && bFirstType != CT_THREE_LINE_TAKE_DOUBLE
                && bFirstType != CT_THREE_LINE_TAKE_SINGLE)
            {
                return -1;
            }
            if (!firstLastHand && !nextlastHand)
                return -1;
        }

        //开始对比
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

                //炸弹叉开
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

//出牌判断
    bool CKuaipaoLogic::SearchOutCard(uint8_t bCardData[], uint8_t bCardCount, uint8_t bTurnCardData[], uint8_t bTurnCardCount,
                                      tagOutCardResult &OutCardResult, bool lastHand) {
        //出牌类型
        uint8_t bTurnOutType = GetCardType(bTurnCardData, bTurnCardCount, false);

        //设置结果
        OutCardResult.cbCardCount = 0;

        //类型搜索
        switch (bTurnOutType)
        {
            case CT_ERROR:                    //错误类型
            {
                //获取数值
                uint8_t bLogicValue = GetCardLogicValue(bCardData[bCardCount - 1]);

                //多牌判断
                uint8_t cbSameCount = 1;
                for (uint8_t i = 1; i < bCardCount; i++)
                {
                    if (GetCardLogicValue(bCardData[bCardCount - i - 1]) == bLogicValue)
                        cbSameCount++;
                    else break;
                }

                //完成处理
                if (cbSameCount > 1)
                {
                    OutCardResult.cbCardCount = cbSameCount;
                    for (uint8_t j = 0; j < cbSameCount; j++)
                    {
                        OutCardResult.cbResultCard[j] = bCardData[bCardCount - 1 - j];
                    }
                    return true;
                }

                //单牌处理
                OutCardResult.cbCardCount = 1;
                OutCardResult.cbResultCard[0] = bCardData[bCardCount - 1];

                return true;
            }
            case CT_SINGLE:                    //单牌类型
            {
                //获取数值
                uint8_t bLogicValue = GetCardLogicValue(bTurnCardData[0]);

                //对比大小
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
            case CT_SINGLE_LINE:            //单连类型
            {
                //长度判断
                if (bCardCount < bTurnCardCount) break;

                //获取数值
                uint8_t bLogicValue = GetCardLogicValue(bTurnCardData[0]);

                //搜索连牌
                for (uint8_t i = (bTurnCardCount - 1); i < bCardCount; i++)
                {
                    //获取数值
                    uint8_t bHandLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);

                    //构造判断
                    if (bHandLogicValue >= 15) break;
                    if (bHandLogicValue <= bLogicValue) continue;

                    //搜索连牌
                    uint8_t bLineCount = 0;
                    for (uint8_t j = (bCardCount - i - 1); j < bCardCount; j++)
                    {
                        if ((GetCardLogicValue(bCardData[j]) + bLineCount) == bHandLogicValue)
                        {
                            //增加连数
                            OutCardResult.cbResultCard[bLineCount++] = bCardData[j];

                            //完成判断
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
            case CT_DOUBLE_LINE:    //对连类型
            {
                //长度判断
                if (bCardCount < bTurnCardCount) break;

                //获取数值
                uint8_t bLogicValue = GetCardLogicValue(bTurnCardData[0]);

                //搜索连牌
                for (uint8_t i = (bTurnCardCount - 1); i < bCardCount; i++)
                {
                    //获取数值
                    uint8_t bHandLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);

                    //构造判断
                    if (bHandLogicValue <= bLogicValue) continue;
                    if ((bTurnCardCount > 2) && (bHandLogicValue >= 15)) break;

                    //搜索连牌
                    uint8_t bLineCount = 0;
                    for (uint8_t j = (bCardCount - i - 1); j < (bCardCount - 1); j++)
                    {
                        if (((GetCardLogicValue(bCardData[j]) + bLineCount) == bHandLogicValue)
                            && ((GetCardLogicValue(bCardData[j + 1]) + bLineCount) == bHandLogicValue))
                        {
                            //增加连数
                            OutCardResult.cbResultCard[bLineCount * 2] = bCardData[j];
                            OutCardResult.cbResultCard[(bLineCount++) * 2 + 1] = bCardData[j + 1];

                            //完成判断
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
            case CT_THREE_LINE:                //三连类型
            case CT_THREE_LINE_TAKE_SINGLE:    //三带一单
            case CT_THREE_LINE_TAKE_DOUBLE:    //三带一对
            {
                //长度判断
                if (bCardCount < bTurnCardCount) break;

                //属性数值
                tagAnalyseResult turnCardResult;
                AnalysebCardData(bTurnCardData, bTurnCardCount, turnCardResult);
                SpiltBombToThree(turnCardResult);
                uint8_t lines = 0, value = 0;
                GetThreeLine(turnCardResult, lines, value);
                LOG_DEBUG("搜索出牌三连:lines:{}--value:{}", lines, value);
                uint8_t bTurnLineCount = lines;
                uint8_t bLogicValue = value;

                //搜索连牌
                for (uint8_t i = bTurnLineCount * 3 - 1; i < bCardCount; i++)
                {
                    //获取数值
                    uint8_t bHandLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);

                    //构造判断
                    if (bHandLogicValue <= bLogicValue) continue;
                    if ((bTurnLineCount > 1) && (bHandLogicValue >= 15)) break;

                    //搜索连牌
                    uint8_t bLineCount = 0;
                    for (uint8_t j = (bCardCount - i - 1); j < (bCardCount - 2); j++)
                    {
                        //三牌判断
                        if ((GetCardLogicValue(bCardData[j]) + bLineCount) != bHandLogicValue) continue;
                        if ((GetCardLogicValue(bCardData[j + 1]) + bLineCount) != bHandLogicValue) continue;
                        if ((GetCardLogicValue(bCardData[j + 2]) + bLineCount) != bHandLogicValue) continue;

                        //增加连数
                        OutCardResult.cbResultCard[bLineCount * 3] = bCardData[j];
                        OutCardResult.cbResultCard[bLineCount * 3 + 1] = bCardData[j + 1];
                        OutCardResult.cbResultCard[(bLineCount++) * 3 + 2] = bCardData[j + 2];

                        //完成判断
                        if (bLineCount == bTurnLineCount)
                        {
                            //连牌设置
                            OutCardResult.cbCardCount = bLineCount * 3;

                            //构造扑克
                            uint8_t bLeftCardData[MAX_COUNT];
                            uint8_t bLeftCount = bCardCount - OutCardResult.cbCardCount;
                            memcpy(bLeftCardData, bCardData, sizeof(uint8_t) * bCardCount);
                            RemoveCardList(OutCardResult.cbResultCard, OutCardResult.cbCardCount, bLeftCardData,
                                           bCardCount);//移除搜索到的三连牌

                            //分析扑克
                            tagAnalyseResult AnalyseResultLeft;
                            AnalysebCardData(bLeftCardData, bLeftCount, AnalyseResultLeft);

                            //单牌处理
                            if (bTurnOutType == CT_THREE_LINE_TAKE_SINGLE || bTurnOutType == CT_THREE_LINE_TAKE_DOUBLE)
                            {
                                //提取单牌
                                for (uint8_t k = 0; k < AnalyseResultLeft.bSignedCount; k++)
                                {
                                    //中止判断
                                    if (OutCardResult.cbCardCount == bTurnCardCount) break;

                                    //设置扑克
                                    uint8_t bIndex = k;//AnalyseResultLeft.bSignedCount-k-1;
                                    uint8_t bSignedCard = AnalyseResultLeft.bSignedCardData[bIndex];
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = bSignedCard;
                                }

                                //提取对牌
                                for (uint8_t k = 0; k < AnalyseResultLeft.bDoubleCount * 2; k++)
                                {
                                    //中止判断
                                    if (OutCardResult.cbCardCount == bTurnCardCount) break;

                                    //设置扑克
                                    uint8_t bIndex = k;//(AnalyseResultLeft.bDoubleCount*2-k-1);
                                    uint8_t bSignedCard = AnalyseResultLeft.bDoubleCardData[bIndex];
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = bSignedCard;
                                }

                                //提取三牌
                                for (uint8_t k = 0; k < AnalyseResultLeft.bThreeCount * 3; k++)
                                {
                                    //中止判断
                                    if (OutCardResult.cbCardCount == bTurnCardCount) break;

                                    //设置扑克
                                    uint8_t bIndex = k;//(AnalyseResultLeft.bThreeCount*3-k-1);
                                    uint8_t bSignedCard = AnalyseResultLeft.bThreeCardData[bIndex];
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = bSignedCard;
                                }

                                //提取四牌
                                for (uint8_t k = 0; k < AnalyseResultLeft.bFourCount * 4; k++)
                                {
                                    //中止判断
                                    if (OutCardResult.cbCardCount == bTurnCardCount) break;

                                    //设置扑克
                                    uint8_t bIndex = k;//(AnalyseResultLeft.bFourCount*4-k-1);
                                    uint8_t bSignedCard = AnalyseResultLeft.bFourCardData[bIndex];
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = bSignedCard;
                                }
                            }


                            //完成判断
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
            case CT_BOMB:                        //炸弹类型
            {
                //长度判断
                if (bCardCount < bTurnCardCount) break;

                //获取数值
                uint8_t bLogicValue = GetCardLogicValue(bTurnCardData[0]);

                //搜索炸弹
                for (uint8_t i = 3; i < bCardCount; i++)
                {
                    //获取数值
                    uint8_t bHandLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);

                    //构造判断
                    if (bHandLogicValue <= bLogicValue) continue;

                    //炸弹判断
                    uint8_t bTempLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);
                    uint8_t j = 1;
                    for (j = 1; j < 4; j++)
                    {
                        if (GetCardLogicValue(bCardData[bCardCount + j - i - 1]) != bTempLogicValue) break;
                    }
                    if (j != 4) continue;

                    //完成处理
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

        //炸弹搜索
        for (uint8_t i = 3; i < bCardCount; i++)
        {
            //获取数值
            uint8_t bHandLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);

            //炸弹判断
            uint8_t bTempLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);
            uint8_t j = 1;
            for (j = 1; j < 4; j++)
            {
                if (GetCardLogicValue(bCardData[bCardCount + j - i - 1]) != bTempLogicValue) break;
            }
            if (j != 4) continue;

            //完成处理
            OutCardResult.cbCardCount = 4;
            OutCardResult.cbResultCard[0] = bCardData[bCardCount - i - 1];
            OutCardResult.cbResultCard[1] = bCardData[bCardCount - i];
            OutCardResult.cbResultCard[2] = bCardData[bCardCount - i + 1];
            OutCardResult.cbResultCard[3] = bCardData[bCardCount - i + 2];

            return true;
        }

        return false;
    }

//主动出牌判断
    bool CKuaipaoLogic::SearchActionOutCard(uint8_t bCardData[], uint8_t bCardCount, uint16_t nextPlayerCardNum,
                                            tagOutCardResult &OutCardResult) {

        //飞机
        if (SearchThreeTakeTwoOutCard(bCardData, bCardCount, 3, 2, OutCardResult))return true;//3飞机
        if (SearchThreeTakeTwoOutCard(bCardData, bCardCount, 2, 2, OutCardResult))return true;//2飞机
        //三带二
        if (SearchThreeTakeTwoOutCard(bCardData, bCardCount, 1, 2, OutCardResult))return true;//3带2
        //顺子
        if (SearchDoubleLineOutCard(bCardData, bCardCount, 7, OutCardResult))return true;//7连对
        if (SearchDoubleLineOutCard(bCardData, bCardCount, 6, OutCardResult))return true;//6连对
        if (SearchDoubleLineOutCard(bCardData, bCardCount, 5, OutCardResult))return true;//5连对
        if (SearchDoubleLineOutCard(bCardData, bCardCount, 4, OutCardResult))return true;//4连对
        if (SearchDoubleLineOutCard(bCardData, bCardCount, 3, OutCardResult))return true;//3连对
        if (SearchDoubleLineOutCard(bCardData, bCardCount, 2, OutCardResult))return true;//2连对
        if (SearchSingleLineOutCard(bCardData, bCardCount, OutCardResult))return true;//单连顺

        //炸弹
        if (SearchBombOutCard(bCardData, bCardCount, 4, OutCardResult))return true;
        //对子
        if (SearchBombOutCard(bCardData, bCardCount, 2, OutCardResult))return true;
        //单张(是否顶大)
        if (nextPlayerCardNum == 1)
        {//最大一张
            OutCardResult.cbResultCard[0] = bCardData[0];
            OutCardResult.cbCardCount = 1;
        }
        else
        {//最小单张
            OutCardResult.cbResultCard[0] = bCardData[bCardCount - 1];
            OutCardResult.cbCardCount = 1;
        }

        return true;
    }

//搜索单连出牌
    bool CKuaipaoLogic::SearchSingleLineOutCard(uint8_t bCardData[], uint8_t bCardCount, tagOutCardResult &OutCardResult) {
        //长度判断
        if (bCardCount < 5) return false;
        //获取数值
        uint8_t bLogicValue = GetCardLogicValue(bCardData[bCardCount - 1]);

        //搜索连牌
        for (uint8_t lineCount = bCardCount; lineCount > 5; --lineCount)
        {
            OutCardResult.cbCardCount = 0;

            for (uint8_t i = (lineCount - 1); i < bCardCount; i++)
            {
                //获取数值
                uint8_t bHandLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);

                //构造判断
                if (bHandLogicValue >= 15) return false;
                if (bHandLogicValue <= bLogicValue) continue;

                //搜索连牌
                uint8_t bLineCount = 0;
                for (uint8_t j = (bCardCount - i - 1); j < bCardCount; j++)
                {
                    if ((GetCardLogicValue(bCardData[j]) + bLineCount) == bHandLogicValue)
                    {
                        //增加连数
                        OutCardResult.cbResultCard[bLineCount++] = bCardData[j];

                        //完成判断
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
        //长度判断
        if (bCardCount < lineNum * 2) return false;

        //获取数值
        uint8_t bLogicValue = GetCardLogicValue(bCardData[bCardCount - 1]);

        //搜索2连牌
        OutCardResult.cbCardCount = 0;
        for (uint8_t i = (lineNum * 2 - 1); i < bCardCount; i++)
        {
            //获取数值
            uint8_t bHandLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);

            //构造判断
            if (bHandLogicValue <= bLogicValue) continue;
            if (bHandLogicValue >= 15) return false;

            //搜索连牌
            uint8_t bLineCount = 0;
            for (uint8_t j = (bCardCount - i - 1); j < (bCardCount - 1); j++)
            {
                if (((GetCardLogicValue(bCardData[j]) + bLineCount) == bHandLogicValue)
                    && ((GetCardLogicValue(bCardData[j + 1]) + bLineCount) == bHandLogicValue))
                {
                    //增加连数
                    OutCardResult.cbResultCard[bLineCount * 2] = bCardData[j];
                    OutCardResult.cbResultCard[(bLineCount++) * 2 + 1] = bCardData[j + 1];

                    //完成判断
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

    //搜索三带二或飞机
    bool CKuaipaoLogic::SearchThreeTakeTwoOutCard(uint8_t bCardData[], uint8_t bCardCount, uint8_t lineNum, uint8_t takeNum, tagOutCardResult &OutCardResult) {
        //长度判断
        uint8_t bTurnCardCount = lineNum * (3 + takeNum);
        if (bCardCount < bTurnCardCount) return false;

        //属性数值
        uint8_t bTurnLineCount = lineNum;
        LOG_DEBUG("搜索出牌三连:lines:{}", lineNum);
        //搜索连牌
        for (uint8_t i = bTurnLineCount * 3 - 1; i < bCardCount; i++)
        {
            //获取数值
            uint8_t bHandLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);
            //构造判断
            if ((bTurnLineCount > 1) && (bHandLogicValue >= 15)) break;

            //搜索连牌
            uint8_t bLineCount = 0;
            for (uint8_t j = (bCardCount - i - 1); j < (bCardCount - 2); j++)
            {
                //三牌判断
                if ((GetCardLogicValue(bCardData[j]) + bLineCount) != bHandLogicValue) continue;
                if ((GetCardLogicValue(bCardData[j + 1]) + bLineCount) != bHandLogicValue) continue;
                if ((GetCardLogicValue(bCardData[j + 2]) + bLineCount) != bHandLogicValue) continue;

                //增加连数
                OutCardResult.cbResultCard[bLineCount * 3] = bCardData[j];
                OutCardResult.cbResultCard[bLineCount * 3 + 1] = bCardData[j + 1];
                OutCardResult.cbResultCard[(bLineCount++) * 3 + 2] = bCardData[j + 2];

                //完成判断
                if (bLineCount == bTurnLineCount)
                {
                    //连牌设置
                    OutCardResult.cbCardCount = bLineCount * 3;

                    //构造扑克
                    uint8_t bLeftCardData[MAX_COUNT];
                    uint8_t bLeftCount = bCardCount - OutCardResult.cbCardCount;
                    memcpy(bLeftCardData, bCardData, sizeof(uint8_t) * bCardCount);
                    RemoveCardList(OutCardResult.cbResultCard, OutCardResult.cbCardCount, bLeftCardData,
                                   bCardCount);//移除搜索到的三连牌

                    //分析扑克
                    tagAnalyseResult AnalyseResultLeft;
                    AnalysebCardData(bLeftCardData, bLeftCount, AnalyseResultLeft);

                    //带牌处理
                    if (takeNum > 0)
                    {
                        //提取单牌
                        for (uint8_t k = 0; k < AnalyseResultLeft.bSignedCount; k++)
                        {
                            //中止判断
                            if (OutCardResult.cbCardCount == bTurnCardCount) break;

                            //设置扑克
                            uint8_t bIndex = k;//AnalyseResultLeft.bSignedCount-k-1;
                            uint8_t bSignedCard = AnalyseResultLeft.bSignedCardData[bIndex];
                            OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = bSignedCard;
                        }

                        //提取对牌
                        for (uint8_t k = 0; k < AnalyseResultLeft.bDoubleCount * 2; k++)
                        {
                            //中止判断
                            if (OutCardResult.cbCardCount == bTurnCardCount) break;

                            //设置扑克
                            uint8_t bIndex = k;//(AnalyseResultLeft.bDoubleCount*2-k-1);
                            uint8_t bSignedCard = AnalyseResultLeft.bDoubleCardData[bIndex];
                            OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = bSignedCard;
                        }

                        //提取三牌
                        for (uint8_t k = 0; k < AnalyseResultLeft.bThreeCount * 3; k++)
                        {
                            //中止判断
                            if (OutCardResult.cbCardCount == bTurnCardCount) break;

                            //设置扑克
                            uint8_t bIndex = k;//(AnalyseResultLeft.bThreeCount*3-k-1);
                            uint8_t bSignedCard = AnalyseResultLeft.bThreeCardData[bIndex];
                            OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = bSignedCard;
                        }

                        //提取四牌
                        for (uint8_t k = 0; k < AnalyseResultLeft.bFourCount * 4; k++)
                        {
                            //中止判断
                            if (OutCardResult.cbCardCount == bTurnCardCount) break;

                            //设置扑克
                            uint8_t bIndex = k;//(AnalyseResultLeft.bFourCount*4-k-1);
                            uint8_t bSignedCard = AnalyseResultLeft.bFourCardData[bIndex];
                            OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = bSignedCard;
                        }
                    }

                    //完成判断
                    if (OutCardResult.cbCardCount != bTurnCardCount && OutCardResult.cbCardCount != bCardCount)
                    {//牌数对或最后一手
                        OutCardResult.cbCardCount = 0;
                        return false;
                    }

                    return true;
                }
            }
        }
        return false;
    }

//搜索炸弹或对子
    bool CKuaipaoLogic::SearchBombOutCard(uint8_t bCardData[], uint8_t bCardCount, uint8_t bSameNum, tagOutCardResult &OutCardResult) {
        //长度判断
        if (bCardCount < bSameNum) return false;

        //搜索炸弹
        for (uint8_t i = bSameNum - 1; i < bCardCount; i++)
        {
            //炸弹判断
            uint8_t bTempLogicValue = GetCardLogicValue(bCardData[bCardCount - i - 1]);
            uint8_t j = 1;
            for (j = 1; j < bSameNum; j++)
            {
                if (GetCardLogicValue(bCardData[bCardCount + j - i - 1]) != bTempLogicValue) break;
            }
            if (j != bSameNum) continue;

            //完成处理
            OutCardResult.cbCardCount = bSameNum;
            for (uint8_t k = 0; k < bSameNum; ++k)
            {
                OutCardResult.cbResultCard[k] = bCardData[bCardCount - i - 1 + k];
            }
            return true;
        }

        return false;
    }

//分析扑克
    void CKuaipaoLogic::AnalysebCardData(const uint8_t bCardData[], uint8_t bCardCount, tagAnalyseResult &AnalyseResult) {
        //设置结果
        ZeroMemory(&AnalyseResult, sizeof(AnalyseResult));

        //扑克分析
        for (uint8_t i = 0; i < bCardCount; i++)
        {
            //变量定义
            uint8_t bSameCount = 1;
            uint8_t bSameCardData[4] = {bCardData[i], 0, 0, 0};
            uint8_t bLogicValue = GetCardLogicValue(bCardData[i]);

            //获取同牌
            for (int j = i + 1; j < bCardCount; j++)
            {
                //逻辑对比
                if (GetCardLogicValue(bCardData[j]) != bLogicValue) break;

                //设置扑克
                bSameCardData[bSameCount++] = bCardData[j];
            }

            //保存结果
            switch (bSameCount)
            {
                case 1:        //单张
                {
                    AnalyseResult.bSignedLogicVolue[AnalyseResult.bSignedCount] = bLogicValue;
                    memcpy(&AnalyseResult.bSignedCardData[(AnalyseResult.bSignedCount++) * bSameCount], bSameCardData,
                           bSameCount);
                    break;
                }
                case 2:        //两张
                {
                    AnalyseResult.bDoubleLogicVolue[AnalyseResult.bDoubleCount] = bLogicValue;
                    memcpy(&AnalyseResult.bDoubleCardData[(AnalyseResult.bDoubleCount++) * bSameCount], bSameCardData,
                           bSameCount);
                    break;
                }
                case 3:        //三张
                {
                    AnalyseResult.bThreeLogicVolue[AnalyseResult.bThreeCount] = bLogicValue;
                    memcpy(&AnalyseResult.bThreeCardData[(AnalyseResult.bThreeCount++) * bSameCount], bSameCardData,
                           bSameCount);
                    break;
                }
                case 4:        //四张
                {
                    AnalyseResult.bFourLogicVolue[AnalyseResult.bFourCount] = bLogicValue;
                    memcpy(&AnalyseResult.bFourCardData[(AnalyseResult.bFourCount++) * bSameCount], bSameCardData,
                           bSameCount);
                    break;
                }
            }

            //设置递增
            i += bSameCount - 1;
        }

        return;
    }

//炸弹拆三张
    void CKuaipaoLogic::SpiltBombToThree(tagAnalyseResult &AnalyseResult) {
        //炸弹叉开
        for (uint8_t i = 0; i < AnalyseResult.bFourCount; ++i)
        {
            AnalyseResult.bThreeLogicVolue[AnalyseResult.bThreeCount] = AnalyseResult.bFourLogicVolue[i];
            AnalyseResult.bThreeCount++;
            LOG_DEBUG("岔开炸弹:{}", AnalyseResult.bThreeCount);
            std::sort(AnalyseResult.bThreeLogicVolue, AnalyseResult.bThreeLogicVolue + AnalyseResult.bThreeCount,
                      [&](uint8_t a, uint8_t b)
                      {
                          return a > b;
                      });
        }
    }

//获取三连的数量及起始值
    void CKuaipaoLogic::GetThreeLine(tagAnalyseResult &AnalyseResult, uint8_t &lines, uint8_t &value) {
        lines = 0;
        value = 0;
        if (AnalyseResult.bThreeCount > 0)
        {
            //连牌判断
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
                    LOG_DEBUG("{}连顺 value {}", lines, value);
                    return;
                }
                else
                {
                    //特殊处理(三连带三张)
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






