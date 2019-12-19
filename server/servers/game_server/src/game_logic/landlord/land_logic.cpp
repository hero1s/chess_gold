

#include "land_logic.h"
#include "math/random.hpp"

using namespace game_land;
//静态变量
namespace game_land {
//索引变量
    const uint8_t cbIndexCount = 5;

//构造函数
    CLandLogic::CLandLogic() {
    }

//析构函数
    CLandLogic::~CLandLogic() {
    }

//获取类型
    uint8_t CLandLogic::GetCardType(const uint8_t cbCardData[], uint8_t cbCardCount) {
        //简单牌型
        switch (cbCardCount)
        {
            case 0:    //空牌
            {
                return CT_ERROR;
            }
            case 1: //单牌
            {
                return CT_SINGLE;
            }
            case 2:    //对牌火箭
            {
                //牌型判断
                if ((cbCardData[0] == 0x4F) && (cbCardData[1] == 0x4E)) return CT_MISSILE_CARD;
                if (GetCardLogicValue(cbCardData[0]) == GetCardLogicValue(cbCardData[1])) return CT_DOUBLE;

                return CT_ERROR;
            }
        }

        //分析扑克
        tagAnalyseResult AnalyseResult;
        AnalysebCardData(cbCardData, cbCardCount, AnalyseResult);

        //四牌判断
        if (AnalyseResult.cbBlockCount[3] > 0)
        {
            //牌型判断
            if ((AnalyseResult.cbBlockCount[3] == 1) && (cbCardCount == 4)) return CT_BOMB_CARD;
            if ((AnalyseResult.cbBlockCount[3] == 1) && (cbCardCount == 6)) return CT_FOUR_TAKE_ONE;
            if ((AnalyseResult.cbBlockCount[3] == 1) && (cbCardCount == 8) &&
                (AnalyseResult.cbBlockCount[1] == 2))
                return CT_FOUR_TAKE_TWO;

            return CT_ERROR;
        }

        //三牌判断
        if (AnalyseResult.cbBlockCount[2] > 0)
        {
            //连牌判断
            if (AnalyseResult.cbBlockCount[2] > 1)
            {
                //变量定义
                uint8_t cbCardData = AnalyseResult.cbCardData[2][0];
                uint8_t cbFirstLogicValue = GetCardLogicValue(cbCardData);

                //错误过虑
                if (cbFirstLogicValue >= 15) return CT_ERROR;

                //连牌判断
                for (uint8_t i = 1; i < AnalyseResult.cbBlockCount[2]; i++)
                {
                    uint8_t cbCardData = AnalyseResult.cbCardData[2][i * 3];
                    if (cbFirstLogicValue != (GetCardLogicValue(cbCardData) + i)) return CT_ERROR;
                }
            }
            else if (cbCardCount == 3) return CT_THREE;

            //牌形判断
            if (AnalyseResult.cbBlockCount[2] * 3 == cbCardCount) return CT_THREE_LINE;
            if (AnalyseResult.cbBlockCount[2] * 4 == cbCardCount) return CT_THREE_TAKE_ONE;
            if ((AnalyseResult.cbBlockCount[2] * 5 == cbCardCount) &&
                (AnalyseResult.cbBlockCount[1] == AnalyseResult.cbBlockCount[2]))
                return CT_THREE_TAKE_TWO;

            return CT_ERROR;
        }

        //两张类型
        if (AnalyseResult.cbBlockCount[1] >= 3)
        {
            //变量定义
            uint8_t cbCardData = AnalyseResult.cbCardData[1][0];
            uint8_t cbFirstLogicValue = GetCardLogicValue(cbCardData);

            //错误过虑
            if (cbFirstLogicValue >= 15) return CT_ERROR;

            //连牌判断
            for (uint8_t i = 1; i < AnalyseResult.cbBlockCount[1]; i++)
            {
                uint8_t cbCardData = AnalyseResult.cbCardData[1][i * 2];
                if (cbFirstLogicValue != (GetCardLogicValue(cbCardData) + i)) return CT_ERROR;
            }

            //二连判断
            if ((AnalyseResult.cbBlockCount[1] * 2) == cbCardCount) return CT_DOUBLE_LINE;

            return CT_ERROR;
        }

        //单张判断
        if ((AnalyseResult.cbBlockCount[0] >= 5) && (AnalyseResult.cbBlockCount[0] == cbCardCount))
        {
            //变量定义
            uint8_t cbCardData = AnalyseResult.cbCardData[0][0];
            uint8_t cbFirstLogicValue = GetCardLogicValue(cbCardData);

            //错误过虑
            if (cbFirstLogicValue >= 15) return CT_ERROR;

            //连牌判断
            for (uint8_t i = 1; i < AnalyseResult.cbBlockCount[0]; i++)
            {
                uint8_t cbCardData = AnalyseResult.cbCardData[0][i];
                if (cbFirstLogicValue != (GetCardLogicValue(cbCardData) + i)) return CT_ERROR;
            }

            return CT_SINGLE_LINE;
        }

        return CT_ERROR;
    }

//排列扑克
    void CLandLogic::SortCardList(uint8_t cbCardData[], uint8_t cbCardCount, uint8_t cbSortType) {
        //数目过虑
        if (cbCardCount == 0) return;
        if (cbSortType == ST_CUSTOM) return;

        SortCardListByLogicValue(cbCardData, cbCardCount);

        //数目排序
        if (cbSortType == ST_COUNT)
        {
            //变量定义
            uint8_t cbCardIndex = 0;

            //分析扑克
            tagAnalyseResult AnalyseResult;
            AnalysebCardData(&cbCardData[cbCardIndex], cbCardCount - cbCardIndex, AnalyseResult);

            //提取扑克
            for (uint8_t i = 0; i < getArrayLen(AnalyseResult.cbBlockCount); i++)
            {
                //拷贝扑克
                uint8_t cbIndex = getArrayLen(AnalyseResult.cbBlockCount) - i - 1;
                memcpy(&cbCardData[cbCardIndex], AnalyseResult.cbCardData[cbIndex],
                       AnalyseResult.cbBlockCount[cbIndex] * (cbIndex + 1) * sizeof(uint8_t));

                //设置索引
                cbCardIndex += AnalyseResult.cbBlockCount[cbIndex] * (cbIndex + 1) * sizeof(uint8_t);
            }
        }

        return;
    }

//混乱扑克
    void CLandLogic::RandCardList(uint8_t cbCardBuffer[], uint8_t cbBufferCount) {
        return GetRandCardList(m_cbFullCardData, getArrayLen(m_cbFullCardData), cbCardBuffer, cbBufferCount);
    }

//对比扑克
    bool CLandLogic::CompareCard(const uint8_t cbFirstCard[], const uint8_t cbNextCard[], uint8_t cbFirstCount,
                                 uint8_t cbNextCount) {
        //获取类型
        uint8_t cbNextType = GetCardType(cbNextCard, cbNextCount);
        uint8_t cbFirstType = GetCardType(cbFirstCard, cbFirstCount);

        //类型判断
        if (cbNextType == CT_ERROR) return false;
        if (cbNextType == CT_MISSILE_CARD) return true;

        //炸弹判断
        if ((cbFirstType != CT_BOMB_CARD) && (cbNextType == CT_BOMB_CARD)) return true;
        if ((cbFirstType == CT_BOMB_CARD) && (cbNextType != CT_BOMB_CARD)) return false;

        //规则判断
        if ((cbFirstType != cbNextType) || (cbFirstCount != cbNextCount)) return false;

        //开始对比
        switch (cbNextType)
        {
            case CT_SINGLE:
            case CT_DOUBLE:
            case CT_THREE:
            case CT_SINGLE_LINE:
            case CT_DOUBLE_LINE:
            case CT_THREE_LINE:
            case CT_BOMB_CARD:
            {
                //获取数值
                uint8_t cbNextLogicValue = GetCardLogicValue(cbNextCard[0]);
                uint8_t cbFirstLogicValue = GetCardLogicValue(cbFirstCard[0]);

                //对比扑克
                return cbNextLogicValue > cbFirstLogicValue;
            }
            case CT_THREE_TAKE_ONE:
            case CT_THREE_TAKE_TWO:
            {
                //分析扑克
                tagAnalyseResult NextResult;
                tagAnalyseResult FirstResult;
                AnalysebCardData(cbNextCard, cbNextCount, NextResult);
                AnalysebCardData(cbFirstCard, cbFirstCount, FirstResult);

                //获取数值
                uint8_t cbNextLogicValue = GetCardLogicValue(NextResult.cbCardData[2][0]);
                uint8_t cbFirstLogicValue = GetCardLogicValue(FirstResult.cbCardData[2][0]);

                //对比扑克
                return cbNextLogicValue > cbFirstLogicValue;
            }
            case CT_FOUR_TAKE_ONE:
            case CT_FOUR_TAKE_TWO:
            {
                //分析扑克
                tagAnalyseResult NextResult;
                tagAnalyseResult FirstResult;
                AnalysebCardData(cbNextCard, cbNextCount, NextResult);
                AnalysebCardData(cbFirstCard, cbFirstCount, FirstResult);

                //获取数值
                uint8_t cbNextLogicValue = GetCardLogicValue(NextResult.cbCardData[3][0]);
                uint8_t cbFirstLogicValue = GetCardLogicValue(FirstResult.cbCardData[3][0]);

                //对比扑克
                return cbNextLogicValue > cbFirstLogicValue;
            }
        }

        return false;
    }

//出牌搜索
    bool CLandLogic::SearchOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, const uint8_t cbTurnCardData[],
                                   uint8_t cbTurnCardCount, tagOutCardResult &OutCardResult) {
        //设置结果
        memset(&OutCardResult, 0, sizeof(OutCardResult));

        //构造扑克
        uint8_t cbCardData[MAX_LAND_COUNT];
        uint8_t cbCardCount = cbHandCardCount;
        memcpy(cbCardData, cbHandCardData, sizeof(uint8_t) * cbHandCardCount);

        //排列扑克
        SortCardList(cbCardData, cbCardCount, ST_ORDER);

        //获取类型
        uint8_t cbTurnOutType = GetCardType(cbTurnCardData, cbTurnCardCount);

        //出牌分析
        switch (cbTurnOutType)
        {
            case CT_ERROR:                    //错误类型
            {
                //获取数值
                uint8_t cbLogicValue = GetCardLogicValue(cbCardData[cbCardCount - 1]);

                //多牌判断
                uint8_t cbSameCount = 1;
                for (uint8_t i = 1; i < cbCardCount; i++)
                {
                    if (GetCardLogicValue(cbCardData[cbCardCount - i - 1]) == cbLogicValue) cbSameCount++;
                    else break;
                }

                //完成处理
                if (cbSameCount > 1)
                {
                    OutCardResult.cbCardCount = cbSameCount;
                    for (uint8_t j = 0; j < cbSameCount; j++)
                        OutCardResult.cbResultCard[j] = cbCardData[cbCardCount - 1 - j];
                    return true;
                }

                //单牌处理
                OutCardResult.cbCardCount = 1;
                OutCardResult.cbResultCard[0] = cbCardData[cbCardCount - 1];

                return true;
            }
            case CT_SINGLE:                    //单牌类型
            case CT_DOUBLE:                    //对牌类型
            case CT_THREE:                    //三条类型
            {
                //获取数值
                uint8_t cbLogicValue = GetCardLogicValue(cbTurnCardData[0]);

                //分析扑克
                tagAnalyseResult AnalyseResult;
                AnalysebCardData(cbCardData, cbCardCount, AnalyseResult);

                //寻找单牌
                if (cbTurnCardCount <= 1)
                {
                    for (uint8_t i = 0; i < AnalyseResult.cbBlockCount[0]; i++)
                    {
                        uint8_t cbIndex = AnalyseResult.cbBlockCount[0] - i - 1;
                        if (GetCardLogicValue(AnalyseResult.cbCardData[0][cbIndex]) > cbLogicValue)
                        {
                            //设置结果
                            OutCardResult.cbCardCount = cbTurnCardCount;
                            memcpy(OutCardResult.cbResultCard, &AnalyseResult.cbCardData[0][cbIndex],
                                   sizeof(uint8_t) * cbTurnCardCount);

                            return true;
                        }
                    }
                }

                //寻找对牌
                if (cbTurnCardCount <= 2)
                {
                    for (uint8_t i = 0; i < AnalyseResult.cbBlockCount[1]; i++)
                    {
                        uint8_t cbIndex = (AnalyseResult.cbBlockCount[1] - i - 1) * 2;
                        if (GetCardLogicValue(AnalyseResult.cbCardData[1][cbIndex]) > cbLogicValue)
                        {
                            //设置结果
                            OutCardResult.cbCardCount = cbTurnCardCount;
                            memcpy(OutCardResult.cbResultCard, &AnalyseResult.cbCardData[1][cbIndex],
                                   sizeof(uint8_t) * cbTurnCardCount);

                            return true;
                        }
                    }
                }

                //寻找三牌
                if (cbTurnCardCount <= 3)
                {
                    for (uint8_t i = 0; i < AnalyseResult.cbBlockCount[2]; i++)
                    {
                        uint8_t cbIndex = (AnalyseResult.cbBlockCount[2] - i - 1) * 3;
                        if (GetCardLogicValue(AnalyseResult.cbCardData[2][cbIndex]) > cbLogicValue)
                        {
                            //设置结果
                            OutCardResult.cbCardCount = cbTurnCardCount;
                            memcpy(OutCardResult.cbResultCard, &AnalyseResult.cbCardData[2][cbIndex],
                                   sizeof(uint8_t) * cbTurnCardCount);

                            return true;
                        }
                    }
                }

                break;
            }
            case CT_SINGLE_LINE:        //单连类型
            {
                //长度判断
                if (cbCardCount < cbTurnCardCount) break;

                //获取数值
                uint8_t cbLogicValue = GetCardLogicValue(cbTurnCardData[0]);

                //搜索连牌
                for (uint8_t i = (cbTurnCardCount - 1); i < cbCardCount; i++)
                {
                    //获取数值
                    uint8_t cbHandLogicValue = GetCardLogicValue(cbCardData[cbCardCount - i - 1]);

                    //构造判断
                    if (cbHandLogicValue >= 15) break;
                    if (cbHandLogicValue <= cbLogicValue) continue;

                    //搜索连牌
                    uint8_t cbLineCount = 0;
                    for (uint8_t j = (cbCardCount - i - 1); j < cbCardCount; j++)
                    {
                        if ((GetCardLogicValue(cbCardData[j]) + cbLineCount) == cbHandLogicValue)
                        {
                            //增加连数
                            OutCardResult.cbResultCard[cbLineCount++] = cbCardData[j];

                            //完成判断
                            if (cbLineCount == cbTurnCardCount)
                            {
                                OutCardResult.cbCardCount = cbTurnCardCount;
                                return true;
                            }
                        }
                    }
                }

                break;
            }
            case CT_DOUBLE_LINE:        //对连类型
            {
                //长度判断
                if (cbCardCount < cbTurnCardCount) break;

                //获取数值
                uint8_t cbLogicValue = GetCardLogicValue(cbTurnCardData[0]);

                //搜索连牌
                for (uint8_t i = (cbTurnCardCount - 1); i < cbCardCount; i++)
                {
                    //获取数值
                    uint8_t cbHandLogicValue = GetCardLogicValue(cbCardData[cbCardCount - i - 1]);

                    //构造判断
                    if (cbHandLogicValue <= cbLogicValue) continue;
                    if ((cbTurnCardCount > 1) && (cbHandLogicValue >= 15)) break;

                    //搜索连牌
                    uint8_t cbLineCount = 0;
                    for (uint8_t j = (cbCardCount - i - 1); j < (cbCardCount - 1); j++)
                    {
                        if (((GetCardLogicValue(cbCardData[j]) + cbLineCount) == cbHandLogicValue)
                            && ((GetCardLogicValue(cbCardData[j + 1]) + cbLineCount) == cbHandLogicValue))
                        {
                            //增加连数
                            OutCardResult.cbResultCard[cbLineCount * 2] = cbCardData[j];
                            OutCardResult.cbResultCard[(cbLineCount++) * 2 + 1] = cbCardData[j + 1];

                            //完成判断
                            if (cbLineCount * 2 == cbTurnCardCount)
                            {
                                OutCardResult.cbCardCount = cbTurnCardCount;
                                return true;
                            }
                        }
                    }
                }

                break;
            }
            case CT_THREE_LINE:        //三连类型
            case CT_THREE_TAKE_ONE:    //三带一单
            case CT_THREE_TAKE_TWO:    //三带一对
            {
                //长度判断
                if (cbCardCount < cbTurnCardCount) break;

                //获取数值
                uint8_t cbLogicValue = 0;
                for (uint8_t i = 0; i < cbTurnCardCount - 2; i++)
                {
                    cbLogicValue = GetCardLogicValue(cbTurnCardData[i]);
                    if (GetCardLogicValue(cbTurnCardData[i + 1]) != cbLogicValue) continue;
                    if (GetCardLogicValue(cbTurnCardData[i + 2]) != cbLogicValue) continue;
                    break;
                }

                //属性数值
                uint8_t cbTurnLineCount = 0;
                if (cbTurnOutType == CT_THREE_TAKE_ONE) cbTurnLineCount = cbTurnCardCount / 4;
                else if (cbTurnOutType == CT_THREE_TAKE_TWO) cbTurnLineCount = cbTurnCardCount / 5;
                else cbTurnLineCount = cbTurnCardCount / 3;

                //搜索连牌
                for (uint8_t i = cbTurnLineCount * 3 - 1; i < cbCardCount; i++)
                {
                    //获取数值
                    uint8_t cbHandLogicValue = GetCardLogicValue(cbCardData[cbCardCount - i - 1]);

                    //构造判断
                    if (cbHandLogicValue <= cbLogicValue) continue;
                    if ((cbTurnLineCount > 1) && (cbHandLogicValue >= 15)) break;

                    //搜索连牌
                    uint8_t cbLineCount = 0;
                    for (uint8_t j = (cbCardCount - i - 1); j < (cbCardCount - 2); j++)
                    {
                        //设置变量
                        OutCardResult.cbCardCount = 0;

                        //三牌判断
                        if ((GetCardLogicValue(cbCardData[j]) + cbLineCount) != cbHandLogicValue) continue;
                        if ((GetCardLogicValue(cbCardData[j + 1]) + cbLineCount) != cbHandLogicValue) continue;
                        if ((GetCardLogicValue(cbCardData[j + 2]) + cbLineCount) != cbHandLogicValue) continue;

                        //增加连数
                        OutCardResult.cbResultCard[cbLineCount * 3] = cbCardData[j];
                        OutCardResult.cbResultCard[cbLineCount * 3 + 1] = cbCardData[j + 1];
                        OutCardResult.cbResultCard[(cbLineCount++) * 3 + 2] = cbCardData[j + 2];

                        //完成判断
                        if (cbLineCount == cbTurnLineCount)
                        {
                            //连牌设置
                            OutCardResult.cbCardCount = cbLineCount * 3;

                            //构造扑克
                            uint8_t cbLeftCardData[MAX_LAND_COUNT];
                            uint8_t cbLeftCount = cbCardCount - OutCardResult.cbCardCount;
                            memcpy(cbLeftCardData, cbCardData, sizeof(uint8_t) * cbCardCount);
                            RemoveCardList(OutCardResult.cbResultCard, OutCardResult.cbCardCount, cbLeftCardData,
                                           cbCardCount);

                            //分析扑克
                            tagAnalyseResult AnalyseResultLeft;
                            AnalysebCardData(cbLeftCardData, cbLeftCount, AnalyseResultLeft);

                            //单牌处理
                            if (cbTurnOutType == CT_THREE_TAKE_ONE)
                            {
                                //提取单牌
                                for (uint8_t k = 0; k < AnalyseResultLeft.cbBlockCount[0]; k++)
                                {
                                    //中止判断
                                    if (OutCardResult.cbCardCount == cbTurnCardCount) break;

                                    //设置扑克
                                    uint8_t cbIndex = AnalyseResultLeft.cbBlockCount[0] - k - 1;
                                    uint8_t cbSignedCard = AnalyseResultLeft.cbCardData[0][cbIndex];
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = cbSignedCard;
                                }

                                //提取对牌
                                for (uint8_t k = 0; k < AnalyseResultLeft.cbBlockCount[1] * 2; k++)
                                {
                                    //中止判断
                                    if (OutCardResult.cbCardCount == cbTurnCardCount) break;

                                    //设置扑克
                                    uint8_t cbIndex = (AnalyseResultLeft.cbBlockCount[1] * 2 - k - 1);
                                    uint8_t cbSignedCard = AnalyseResultLeft.cbCardData[1][cbIndex];
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = cbSignedCard;
                                }

                                //提取三牌
                                for (uint8_t k = 0; k < AnalyseResultLeft.cbBlockCount[2] * 3; k++)
                                {
                                    //中止判断
                                    if (OutCardResult.cbCardCount == cbTurnCardCount) break;

                                    //设置扑克
                                    uint8_t cbIndex = (AnalyseResultLeft.cbBlockCount[2] * 3 - k - 1);
                                    uint8_t cbSignedCard = AnalyseResultLeft.cbCardData[2][cbIndex];
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = cbSignedCard;
                                }

                                //提取四牌
                                for (uint8_t k = 0; k < AnalyseResultLeft.cbBlockCount[3] * 4; k++)
                                {
                                    //中止判断
                                    if (OutCardResult.cbCardCount == cbTurnCardCount) break;

                                    //设置扑克
                                    uint8_t cbIndex = (AnalyseResultLeft.cbBlockCount[3] * 4 - k - 1);
                                    uint8_t cbSignedCard = AnalyseResultLeft.cbCardData[3][cbIndex];
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = cbSignedCard;
                                }
                            }

                            //对牌处理
                            if (cbTurnOutType == CT_THREE_TAKE_TWO)
                            {
                                //提取对牌
                                for (uint8_t k = 0; k < AnalyseResultLeft.cbBlockCount[1]; k++)
                                {
                                    //中止判断
                                    if (OutCardResult.cbCardCount == cbTurnCardCount) break;

                                    //设置扑克
                                    uint8_t cbIndex = (AnalyseResultLeft.cbBlockCount[1] - k - 1) * 2;
                                    uint8_t cbCardData1 = AnalyseResultLeft.cbCardData[1][cbIndex];
                                    uint8_t cbCardData2 = AnalyseResultLeft.cbCardData[1][cbIndex + 1];
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = cbCardData1;
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = cbCardData2;
                                }

                                //提取三牌
                                for (uint8_t k = 0; k < AnalyseResultLeft.cbBlockCount[2]; k++)
                                {
                                    //中止判断
                                    if (OutCardResult.cbCardCount == cbTurnCardCount) break;

                                    //设置扑克
                                    uint8_t cbIndex = (AnalyseResultLeft.cbBlockCount[2] - k - 1) * 3;
                                    uint8_t cbCardData1 = AnalyseResultLeft.cbCardData[2][cbIndex];
                                    uint8_t cbCardData2 = AnalyseResultLeft.cbCardData[2][cbIndex + 1];
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = cbCardData1;
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = cbCardData2;
                                }

                                //提取四牌
                                for (uint8_t k = 0; k < AnalyseResultLeft.cbBlockCount[3]; k++)
                                {
                                    //中止判断
                                    if (OutCardResult.cbCardCount == cbTurnCardCount) break;

                                    //设置扑克
                                    uint8_t cbIndex = (AnalyseResultLeft.cbBlockCount[3] - k - 1) * 4;
                                    uint8_t cbCardData1 = AnalyseResultLeft.cbCardData[3][cbIndex];
                                    uint8_t cbCardData2 = AnalyseResultLeft.cbCardData[3][cbIndex + 1];
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = cbCardData1;
                                    OutCardResult.cbResultCard[OutCardResult.cbCardCount++] = cbCardData2;
                                }
                            }

                            //完成判断
                            if (OutCardResult.cbCardCount == cbTurnCardCount) return true;
                        }
                    }
                }

                break;
            }
        }

        //搜索炸弹
        if ((cbCardCount >= 4) && (cbTurnOutType != CT_MISSILE_CARD))
        {
            //变量定义
            uint8_t cbLogicValue = 0;
            if (cbTurnOutType == CT_BOMB_CARD) cbLogicValue = GetCardLogicValue(cbTurnCardData[0]);

            //搜索炸弹
            for (uint8_t i = 3; i < cbCardCount; i++)
            {
                //获取数值
                uint8_t cbHandLogicValue = GetCardLogicValue(cbCardData[cbCardCount - i - 1]);

                //构造判断
                if (cbHandLogicValue <= cbLogicValue) continue;

                //炸弹判断
                uint8_t cbTempLogicValue = GetCardLogicValue(cbCardData[cbCardCount - i - 1]);
                uint8_t j = 1;
                for (; j < 4; j++)
                {
                    if (GetCardLogicValue(cbCardData[cbCardCount + j - i - 1]) != cbTempLogicValue) break;
                }
                if (j != 4) continue;

                //设置结果
                OutCardResult.cbCardCount = 4;
                OutCardResult.cbResultCard[0] = cbCardData[cbCardCount - i - 1];
                OutCardResult.cbResultCard[1] = cbCardData[cbCardCount - i];
                OutCardResult.cbResultCard[2] = cbCardData[cbCardCount - i + 1];
                OutCardResult.cbResultCard[3] = cbCardData[cbCardCount - i + 2];

                return true;
            }
        }

        //搜索火箭
        if ((cbCardCount >= 2) && (cbCardData[0] == 0x4F) && (cbCardData[1] == 0x4E))
        {
            //设置结果
            OutCardResult.cbCardCount = 2;
            OutCardResult.cbResultCard[0] = cbCardData[0];
            OutCardResult.cbResultCard[1] = cbCardData[1];

            return true;
        }

        return false;
    }

//分析扑克
    void CLandLogic::AnalysebCardData(const uint8_t cbCardData[], uint8_t cbCardCount, tagAnalyseResult &AnalyseResult) {
        //设置结果
        memset(&AnalyseResult, 0, sizeof(AnalyseResult));

        //扑克分析
        for (uint8_t i = 0; i < cbCardCount; i++)
        {
            //变量定义
            uint8_t cbSameCount = 1, cbCardValueTemp = 0;
            uint8_t cbLogicValue = GetCardLogicValue(cbCardData[i]);

            //搜索同牌
            for (uint8_t j = i + 1; j < cbCardCount; j++)
            {
                //获取扑克
                if (GetCardLogicValue(cbCardData[j]) != cbLogicValue) break;

                //设置变量
                cbSameCount++;
            }

            //设置结果
            uint8_t cbIndex = AnalyseResult.cbBlockCount[cbSameCount - 1]++;
            for (uint8_t j = 0; j < cbSameCount; j++)
                AnalyseResult.cbCardData[cbSameCount - 1][cbIndex * cbSameCount + j] = cbCardData[i + j];

            //设置索引
            i += cbSameCount - 1;
        }

        return;
    }

//分析分布
    void CLandLogic::AnalysebDistributing(const uint8_t cbCardData[], uint8_t cbCardCount, tagDistributing &Distributing) {
        //设置变量
        memset(&Distributing, 0, sizeof(Distributing));

        //设置变量
        for (uint8_t i = 0; i < cbCardCount; i++)
        {
            if (cbCardData[i] == 0) continue;

            //获取属性
            uint8_t cbCardColor = GetCardColor(cbCardData[i]);
            uint8_t cbCardValue = GetCardValue(cbCardData[i]);

            //分布信息
            Distributing.cbCardCount++;
            Distributing.cbDistributing[cbCardValue - 1][cbIndexCount]++;
            Distributing.cbDistributing[cbCardValue - 1][cbCardColor >> 4]++;
        }

        return;
    }

//设置扑克
    void CLandLogic::SetUserCard(uint16_t wChairID, uint8_t cbCardData[], uint8_t cbCardCount) {
        memcpy(m_cbAllCardData[wChairID], cbCardData, cbCardCount * sizeof(uint8_t));
        m_cbUserCardCount[wChairID] = cbCardCount;

        //排列扑克
        SortCardList(m_cbAllCardData[wChairID], cbCardCount, ST_ORDER);
    }

//设置底牌
    void CLandLogic::SetBackCard(uint16_t wChairID, uint8_t cbBackCardData[], uint8_t cbCardCount) {
        uint8_t cbTmpCount = m_cbUserCardCount[wChairID];
        memcpy(m_cbAllCardData[wChairID] + cbTmpCount, cbBackCardData, cbCardCount * sizeof(uint8_t));
        m_cbUserCardCount[wChairID] += cbCardCount;

        //排列扑克
        SortCardList(m_cbAllCardData[wChairID], m_cbUserCardCount[wChairID], ST_ORDER);
    }

//设置庄家
    void CLandLogic::SetBanker(uint16_t wBanker) {
        m_wBankerUser = wBanker;
    }

//叫牌扑克
    void CLandLogic::SetLandScoreCardData(uint16_t wChairID, uint8_t cbCardData[], uint8_t cbCardCount) {
        if (cbCardCount != MAX_LAND_COUNT) return;

        memcpy(m_cbLandScoreCardData[wChairID], cbCardData, cbCardCount * sizeof(uint8_t));
        //排列扑克
        SortCardList(m_cbLandScoreCardData[wChairID], cbCardCount, ST_ORDER);
    }

//删除扑克
    void CLandLogic::RemoveUserCardData(uint16_t wChairID, uint8_t cbRemoveCardData[], uint8_t cbRemoveCardCount) {
        bool bSuccess = RemoveCardList(cbRemoveCardData, cbRemoveCardCount, m_cbAllCardData[wChairID],
                                       m_cbUserCardCount[wChairID]);
        if (bSuccess)
        {
            m_cbUserCardCount[wChairID] -= cbRemoveCardCount;
        }
    }

//组合算法
    void CLandLogic::Combination(uint8_t cbCombineCardData[], uint8_t cbResComLen, uint8_t cbResultCardData[254][5],
                                 uint8_t &cbResCardLen, uint8_t cbSrcCardData[], uint8_t cbCombineLen1, uint8_t cbSrcLen,
                                 const uint8_t cbCombineLen2) {
        if (cbResComLen == cbCombineLen2)
        {
            memcpy(&cbResultCardData[cbResCardLen], cbCombineCardData, cbResComLen);
            ++cbResCardLen;

            ASSERT(cbResCardLen < 255);
        }
        else
        {
            if (cbCombineLen1 >= 1 && cbSrcLen > 0 && (cbSrcLen + 1) >= 0)
            {
                cbCombineCardData[cbCombineLen2 - cbCombineLen1] = cbSrcCardData[0];
                ++cbResComLen;
                Combination(cbCombineCardData, cbResComLen, cbResultCardData, cbResCardLen, cbSrcCardData + 1,
                            cbCombineLen1 - 1, cbSrcLen - 1, cbCombineLen2);

                --cbResComLen;
                Combination(cbCombineCardData, cbResComLen, cbResultCardData, cbResCardLen, cbSrcCardData + 1,
                            cbCombineLen1, cbSrcLen - 1, cbCombineLen2);
            }
        }
    }

//排列算法
    void CLandLogic::Permutation(uint8_t *list, int m, int n, uint8_t result[][4], uint8_t &len) {
        int j, temp;
        if (m == n)
        {
            for (j = 0; j < n; j++)
                result[len][j] = list[j];
            len++;
        }
        else
        {
            for (j = m; j < n; j++)
            {
                temp = list[m];
                list[m] = list[j];
                list[j] = temp;
                Permutation(list, m + 1, n, result, len);
                temp = list[m];
                list[m] = list[j];
                list[j] = temp;
            }
        }
    }

//分析炸弹
    void CLandLogic::GetAllBomCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbBomCardData[],
                                   uint8_t &cbBomCardCount) {
        uint8_t cbTmpCardData[MAX_LAND_COUNT];
        memcpy(cbTmpCardData, cbHandCardData, cbHandCardCount);

        //大小排序
        SortCardList(cbTmpCardData, cbHandCardCount, ST_ORDER);

        cbBomCardCount = 0;

        if (cbHandCardCount < 2) return;

        //双王炸弹
        if (0x4F == cbTmpCardData[0] && 0x4E == cbTmpCardData[1])
        {
            cbBomCardData[cbBomCardCount++] = cbTmpCardData[0];
            cbBomCardData[cbBomCardCount++] = cbTmpCardData[1];
        }

        //扑克分析
        for (uint8_t i = 0; i < cbHandCardCount; i++)
        {
            //变量定义
            uint8_t cbSameCount = 1;
            uint8_t cbLogicValue = GetCardLogicValue(cbTmpCardData[i]);

            //搜索同牌
            for (uint8_t j = i + 1; j < cbHandCardCount; j++)
            {
                //获取扑克
                if (GetCardLogicValue(cbTmpCardData[j]) != cbLogicValue) break;

                //设置变量
                cbSameCount++;
            }

            if (4 == cbSameCount)
            {
                cbBomCardData[cbBomCardCount++] = cbTmpCardData[i];
                cbBomCardData[cbBomCardCount++] = cbTmpCardData[i + 1];
                cbBomCardData[cbBomCardCount++] = cbTmpCardData[i + 2];
                cbBomCardData[cbBomCardCount++] = cbTmpCardData[i + 3];
            }

            //设置索引
            i += cbSameCount - 1;
        }
    }

//分析顺子
    void CLandLogic::GetAllLineCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbLineCardData[],
                                    uint8_t &cbLineCardCount) {
        uint8_t cbTmpCard[MAX_LAND_COUNT];
        memcpy(cbTmpCard, cbHandCardData, cbHandCardCount);
        //大小排序
        SortCardList(cbTmpCard, cbHandCardCount, ST_ORDER);

        cbLineCardCount = 0;

        //数据校验
        if (cbHandCardCount < 5) return;

        uint8_t cbFirstCard = 0;
        //去除2和王
        for (uint8_t i = 0; i < cbHandCardCount; ++i)
            if (GetCardLogicValue(cbTmpCard[i]) < 15)
            {
                cbFirstCard = i;
                break;
            }

        uint8_t cbSingleLineCard[12];
        uint8_t cbSingleLineCount = 0;
        uint8_t cbLeftCardCount = cbHandCardCount;
        bool bFindSingleLine = true;

        //连牌判断
        while (cbLeftCardCount >= 5 && bFindSingleLine)
        {
            cbSingleLineCount = 1;
            bFindSingleLine = false;
            uint8_t cbLastCard = cbTmpCard[cbFirstCard];
            cbSingleLineCard[cbSingleLineCount - 1] = cbTmpCard[cbFirstCard];
            for (uint8_t i = cbFirstCard + 1; i < cbLeftCardCount; i++)
            {
                uint8_t cbCardData = cbTmpCard[i];

                //连续判断
                if (1 != (GetCardLogicValue(cbLastCard) - GetCardLogicValue(cbCardData)) &&
                    GetCardValue(cbLastCard) != GetCardValue(cbCardData))
                {
                    cbLastCard = cbTmpCard[i];
                    if (cbSingleLineCount < 5)
                    {
                        cbSingleLineCount = 1;
                        cbSingleLineCard[cbSingleLineCount - 1] = cbTmpCard[i];
                        continue;
                    }
                    else break;
                }
                    //同牌判断
                else if (GetCardValue(cbLastCard) != GetCardValue(cbCardData))
                {
                    cbLastCard = cbCardData;
                    cbSingleLineCard[cbSingleLineCount] = cbCardData;
                    ++cbSingleLineCount;
                }
            }

            //保存数据
            if (cbSingleLineCount >= 5)
            {
                RemoveCardList(cbSingleLineCard, cbSingleLineCount, cbTmpCard, cbLeftCardCount);
                memcpy(cbLineCardData + cbLineCardCount, cbSingleLineCard, sizeof(uint8_t) * cbSingleLineCount);
                cbLineCardCount += cbSingleLineCount;
                cbLeftCardCount -= cbSingleLineCount;
                bFindSingleLine = true;
            }
        }
    }

//分析三条
    void CLandLogic::GetAllThreeCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount,
                                     uint8_t cbThreeCardData[],
                                     uint8_t &cbThreeCardCount) {
        uint8_t cbTmpCardData[MAX_LAND_COUNT];
        memcpy(cbTmpCardData, cbHandCardData, cbHandCardCount);

        //大小排序
        SortCardList(cbTmpCardData, cbHandCardCount, ST_ORDER);

        cbThreeCardCount = 0;

        //扑克分析
        for (uint8_t i = 0; i < cbHandCardCount; i++)
        {
            //变量定义
            uint8_t cbSameCount = 1;
            uint8_t cbLogicValue = GetCardLogicValue(cbTmpCardData[i]);

            //搜索同牌
            for (uint8_t j = i + 1; j < cbHandCardCount; j++)
            {
                //获取扑克
                if (GetCardLogicValue(cbTmpCardData[j]) != cbLogicValue) break;

                //设置变量
                cbSameCount++;
            }

            if (cbSameCount >= 3)
            {
                cbThreeCardData[cbThreeCardCount++] = cbTmpCardData[i];
                cbThreeCardData[cbThreeCardCount++] = cbTmpCardData[i + 1];
                cbThreeCardData[cbThreeCardCount++] = cbTmpCardData[i + 2];
            }

            //设置索引
            i += cbSameCount - 1;
        }
    }

//分析对子
    void CLandLogic::GetAllDoubleCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount,
                                      uint8_t cbDoubleCardData[],
                                      uint8_t &cbDoubleCardCount) {
        uint8_t cbTmpCardData[MAX_LAND_COUNT];
        memcpy(cbTmpCardData, cbHandCardData, cbHandCardCount);

        //大小排序
        SortCardList(cbTmpCardData, cbHandCardCount, ST_ORDER);

        cbDoubleCardCount = 0;

        //扑克分析
        for (uint8_t i = 0; i < cbHandCardCount; i++)
        {
            //变量定义
            uint8_t cbSameCount = 1;
            uint8_t cbLogicValue = GetCardLogicValue(cbTmpCardData[i]);

            //搜索同牌
            for (uint8_t j = i + 1; j < cbHandCardCount; j++)
            {
                //获取扑克
                if (GetCardLogicValue(cbTmpCardData[j]) != cbLogicValue) break;

                //设置变量
                cbSameCount++;
            }

            if (cbSameCount >= 2)
            {
                cbDoubleCardData[cbDoubleCardCount++] = cbTmpCardData[i];
                cbDoubleCardData[cbDoubleCardCount++] = cbTmpCardData[i + 1];
            }

            //设置索引
            i += cbSameCount - 1;
        }
    }

//分析单牌
    void CLandLogic::GetAllSingleCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount,
                                      uint8_t cbSingleCardData[],
                                      uint8_t &cbSingleCardCount) {
        cbSingleCardCount = 0;
        uint8_t cbTmpCardData[MAX_LAND_COUNT];
        memcpy(cbTmpCardData, cbHandCardData, cbHandCardCount);

        //大小排序
        SortCardList(cbTmpCardData, cbHandCardCount, ST_ORDER);

        //扑克分析
        for (uint8_t i = 0; i < cbHandCardCount; i++)
        {
            //变量定义
            uint8_t cbSameCount = 1;
            uint8_t cbLogicValue = GetCardLogicValue(cbTmpCardData[i]);

            //搜索同牌
            for (uint8_t j = i + 1; j < cbHandCardCount; j++)
            {
                //获取扑克
                if (GetCardLogicValue(cbTmpCardData[j]) != cbLogicValue) break;

                //设置变量
                cbSameCount++;
            }

            if (cbSameCount == 1)
            {
                cbSingleCardData[cbSingleCardCount++] = cbTmpCardData[i];
            }

            //设置索引
            i += cbSameCount - 1;
        }

    }

//分析牌牌（先出牌调用）
    void CLandLogic::AnalyseOutCardType(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount,
                                        tagOutCardTypeResult CardTypeResult[12 + 1]) {
        memset(CardTypeResult, 0, sizeof(CardTypeResult[0]) * 12);
        uint8_t cbTmpCardData[MAX_LAND_COUNT];
        //保留扑克，防止分析时改变扑克
        uint8_t cbReserveCardData[MAX_LAND_COUNT];
        memcpy(cbReserveCardData, cbHandCardData, cbHandCardCount);
        SortCardList(cbReserveCardData, cbHandCardCount, ST_ORDER);
        memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount);

        //单牌类型
        for (uint8_t i = 0; i < cbHandCardCount; ++i)
        {
            uint8_t Index = CardTypeResult[CT_SINGLE].cbCardTypeCount;
            CardTypeResult[CT_SINGLE].cbCardType = CT_SINGLE;
            CardTypeResult[CT_SINGLE].cbCardData[Index][0] = cbTmpCardData[i];
            CardTypeResult[CT_SINGLE].cbEachHandCardCount[Index] = 1;
            CardTypeResult[CT_SINGLE].cbCardTypeCount++;

            ASSERT(CardTypeResult[CT_SINGLE].cbCardTypeCount < MAX_TYPE_COUNT);
        }

        //对牌类型
        {
            uint8_t cbDoubleCardData[MAX_LAND_COUNT];
            uint8_t cbDoubleCardcount = 0;
            GetAllDoubleCard(cbTmpCardData, cbHandCardCount, cbDoubleCardData, cbDoubleCardcount);
            for (uint8_t i = 0; i < cbDoubleCardcount; i += 2)
            {
                uint8_t Index = CardTypeResult[CT_DOUBLE].cbCardTypeCount;
                CardTypeResult[CT_DOUBLE].cbCardType = CT_DOUBLE;
                CardTypeResult[CT_DOUBLE].cbCardData[Index][0] = cbDoubleCardData[i];
                CardTypeResult[CT_DOUBLE].cbCardData[Index][1] = cbDoubleCardData[i + 1];
                CardTypeResult[CT_DOUBLE].cbEachHandCardCount[Index] = 2;
                CardTypeResult[CT_DOUBLE].cbCardTypeCount++;

                ASSERT(CardTypeResult[CT_DOUBLE].cbCardTypeCount < MAX_TYPE_COUNT);
            }
        }

        //三条类型
        {
            uint8_t cbThreeCardData[MAX_LAND_COUNT];
            uint8_t cbThreeCardCount = 0;
            GetAllThreeCard(cbTmpCardData, cbHandCardCount, cbThreeCardData, cbThreeCardCount);
            for (uint8_t i = 0; i < cbThreeCardCount; i += 3)
            {
                uint8_t Index = CardTypeResult[CT_THREE].cbCardTypeCount;
                CardTypeResult[CT_THREE].cbCardType = CT_THREE;
                CardTypeResult[CT_THREE].cbCardData[Index][0] = cbThreeCardData[i];
                CardTypeResult[CT_THREE].cbCardData[Index][1] = cbThreeCardData[i + 1];
                CardTypeResult[CT_THREE].cbCardData[Index][2] = cbThreeCardData[i + 2];
                CardTypeResult[CT_THREE].cbEachHandCardCount[Index] = 3;
                CardTypeResult[CT_THREE].cbCardTypeCount++;

                ASSERT(CardTypeResult[CT_THREE].cbCardTypeCount < MAX_TYPE_COUNT);
            }
        }

        //炸弹类型
        {
            uint8_t cbFourCardData[MAX_LAND_COUNT];
            uint8_t cbFourCardCount = 0;
            if (cbHandCardCount >= 2 && 0x4F == cbTmpCardData[0] && 0x4E == cbTmpCardData[1])
            {
                uint8_t Index = CardTypeResult[CT_BOMB_CARD].cbCardTypeCount;
                CardTypeResult[CT_BOMB_CARD].cbCardType = CT_BOMB_CARD;
                CardTypeResult[CT_BOMB_CARD].cbCardData[Index][0] = cbTmpCardData[0];
                CardTypeResult[CT_BOMB_CARD].cbCardData[Index][1] = cbTmpCardData[1];
                CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] = 2;
                CardTypeResult[CT_BOMB_CARD].cbCardTypeCount++;
                GetAllBomCard(cbTmpCardData + 2, cbHandCardCount - 2, cbFourCardData, cbFourCardCount);
            }
            else GetAllBomCard(cbTmpCardData, cbHandCardCount, cbFourCardData, cbFourCardCount);
            for (uint8_t i = 0; i < cbFourCardCount; i += 4)
            {
                uint8_t Index = CardTypeResult[CT_BOMB_CARD].cbCardTypeCount;
                CardTypeResult[CT_BOMB_CARD].cbCardType = CT_BOMB_CARD;
                CardTypeResult[CT_BOMB_CARD].cbCardData[Index][0] = cbFourCardData[i];
                CardTypeResult[CT_BOMB_CARD].cbCardData[Index][1] = cbFourCardData[i + 1];
                CardTypeResult[CT_BOMB_CARD].cbCardData[Index][2] = cbFourCardData[i + 2];
                CardTypeResult[CT_BOMB_CARD].cbCardData[Index][3] = cbFourCardData[i + 3];
                CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] = 4;
                CardTypeResult[CT_BOMB_CARD].cbCardTypeCount++;

                ASSERT(CardTypeResult[CT_BOMB_CARD].cbCardTypeCount < MAX_TYPE_COUNT);
            }
        }
        //单连类型
        {
            //恢复扑克，防止分析时改变扑克
            memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount);

            uint8_t cbFirstCard = 0;
            //去除2和王
            for (uint8_t i = 0; i < cbHandCardCount; ++i)
            {
                if (GetCardLogicValue(cbTmpCardData[i]) < 15)
                {
                    cbFirstCard = i;
                    break;
                }
            }

            uint8_t cbSingleLineCard[12];
            uint8_t cbSingleLineCount = 1;
            uint8_t cbLeftCardCount = cbHandCardCount;
            bool bFindSingleLine = true;

            //连牌判断
            while (cbLeftCardCount >= 5 && bFindSingleLine)
            {
                cbSingleLineCount = 1;
                bFindSingleLine = false;
                uint8_t cbLastCard = cbTmpCardData[cbFirstCard];
                cbSingleLineCard[cbSingleLineCount - 1] = cbTmpCardData[cbFirstCard];
                for (uint8_t i = cbFirstCard + 1; i < cbLeftCardCount; i++)
                {
                    uint8_t cbCardData = cbTmpCardData[i];

                    //连续判断
                    if (1 != (GetCardLogicValue(cbLastCard) - GetCardLogicValue(cbCardData)) &&
                        GetCardValue(cbLastCard) != GetCardValue(cbCardData))
                    {
                        cbLastCard = cbTmpCardData[i];
                        //是否合法
                        if (cbSingleLineCount < 5)
                        {
                            cbSingleLineCount = 1;
                            cbSingleLineCard[cbSingleLineCount - 1] = cbTmpCardData[i];
                            continue;
                        }
                        else break;
                    }
                        //同牌判断
                    else if (GetCardValue(cbLastCard) != GetCardValue(cbCardData))
                    {
                        cbLastCard = cbCardData;
                        cbSingleLineCard[cbSingleLineCount] = cbCardData;
                        ++cbSingleLineCount;
                    }
                }

                //保存数据
                if (cbSingleLineCount >= 5)
                {
                    uint8_t Index;
                    //所有连牌
                    uint8_t cbStart = 0;
                    //从大到小
                    while (cbSingleLineCount - cbStart >= 5)
                    {
                        Index = CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount;
                        uint8_t cbThisLineCount = cbSingleLineCount - cbStart;
                        CardTypeResult[CT_SINGLE_LINE].cbCardType = CT_SINGLE_LINE;
                        memcpy(CardTypeResult[CT_SINGLE_LINE].cbCardData[Index], cbSingleLineCard + cbStart,
                               sizeof(uint8_t) * (cbThisLineCount));
                        CardTypeResult[CT_SINGLE_LINE].cbEachHandCardCount[Index] = cbThisLineCount;
                        CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount++;

                        ASSERT(CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount < MAX_TYPE_COUNT);
                        cbStart++;
                    }
                    //从小到大
                    cbStart = 1;
                    while (cbSingleLineCount - cbStart >= 5)
                    {
                        Index = CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount;
                        uint8_t cbThisLineCount = cbSingleLineCount - cbStart;
                        CardTypeResult[CT_SINGLE_LINE].cbCardType = CT_SINGLE_LINE;
                        memcpy(CardTypeResult[CT_SINGLE_LINE].cbCardData[Index], cbSingleLineCard,
                               sizeof(uint8_t) * (cbThisLineCount));
                        CardTypeResult[CT_SINGLE_LINE].cbEachHandCardCount[Index] = cbThisLineCount;
                        CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount++;

                        ASSERT(CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount < MAX_TYPE_COUNT);
                        cbStart++;
                    }

                    RemoveCardList(cbSingleLineCard, cbSingleLineCount, cbTmpCardData, cbLeftCardCount);
                    cbLeftCardCount -= cbSingleLineCount;
                    bFindSingleLine = true;
                }
            }

        }

        //对连类型
        {
            //恢复扑克，防止分析时改变扑克
            memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount);

            //连牌判断
            uint8_t cbFirstCard = 0;
            //去除2和王
            for (uint8_t i = 0; i < cbHandCardCount; ++i)
                if (GetCardLogicValue(cbTmpCardData[i]) < 15)
                {
                    cbFirstCard = i;
                    break;
                }

            uint8_t cbLeftCardCount = cbHandCardCount - cbFirstCard;
            bool bFindDoubleLine = true;
            uint8_t cbDoubleLineCount = 0;
            uint8_t cbDoubleLineCard[24];
            //开始判断
            while (cbLeftCardCount >= 6 && bFindDoubleLine)
            {
                uint8_t cbLastCard = cbTmpCardData[cbFirstCard];
                uint8_t cbSameCount = 1;
                cbDoubleLineCount = 0;
                bFindDoubleLine = false;
                for (uint8_t i = cbFirstCard + 1; i < cbLeftCardCount + cbFirstCard; ++i)
                {
                    //搜索同牌
                    while (GetCardLogicValue(cbLastCard) == GetCardLogicValue(cbTmpCardData[i]) &&
                           i < cbLeftCardCount + cbFirstCard)
                    {
                        ++cbSameCount;
                        ++i;
                    }

                    uint8_t cbLastDoubleCardValue;
                    if (cbDoubleLineCount > 0)
                        cbLastDoubleCardValue = GetCardLogicValue(cbDoubleLineCard[cbDoubleLineCount - 1]);
                    //重新开始
                    if ((cbSameCount < 2 ||
                         (cbDoubleLineCount > 0
                          && (cbLastDoubleCardValue - GetCardLogicValue(cbLastCard)) != 1)) &&
                        i <= cbLeftCardCount + cbFirstCard)
                    {
                        if (cbDoubleLineCount >= 6) break;
                        //回退
                        if (cbSameCount >= 2) i -= cbSameCount;
                        cbLastCard = cbTmpCardData[i];
                        cbDoubleLineCount = 0;
                    }
                        //保存数据
                    else if (cbSameCount >= 2)
                    {
                        cbDoubleLineCard[cbDoubleLineCount] = cbTmpCardData[i - cbSameCount];
                        cbDoubleLineCard[cbDoubleLineCount + 1] = cbTmpCardData[i - cbSameCount + 1];
                        cbDoubleLineCount += 2;

                        //结尾判断
                        if (i == (cbLeftCardCount + cbFirstCard - 2))
                            if ((GetCardLogicValue(cbLastCard) - GetCardLogicValue(cbTmpCardData[i])) == 1 &&
                                (GetCardLogicValue(cbTmpCardData[i])
                                 == GetCardLogicValue(cbTmpCardData[i + 1])))
                            {
                                cbDoubleLineCard[cbDoubleLineCount] = cbTmpCardData[i];
                                cbDoubleLineCard[cbDoubleLineCount + 1] = cbTmpCardData[i + 1];
                                cbDoubleLineCount += 2;
                                break;
                            }

                    }

                    cbLastCard = cbTmpCardData[i];
                    cbSameCount = 1;
                }

                //保存数据
                if (cbDoubleLineCount >= 6)
                {
                    uint8_t Index;

                    Index = CardTypeResult[CT_DOUBLE_LINE].cbCardTypeCount;
                    CardTypeResult[CT_DOUBLE_LINE].cbCardType = CT_DOUBLE_LINE;
                    memcpy(CardTypeResult[CT_DOUBLE_LINE].cbCardData[Index], cbDoubleLineCard,
                           sizeof(uint8_t) * cbDoubleLineCount);
                    CardTypeResult[CT_DOUBLE_LINE].cbEachHandCardCount[Index] = cbDoubleLineCount;
                    CardTypeResult[CT_DOUBLE_LINE].cbCardTypeCount++;

                    ASSERT(CardTypeResult[CT_DOUBLE_LINE].cbCardTypeCount < MAX_TYPE_COUNT);

                    RemoveCardList(cbDoubleLineCard, cbDoubleLineCount, cbTmpCardData, cbFirstCard + cbLeftCardCount);
                    bFindDoubleLine = true;
                    cbLeftCardCount -= cbDoubleLineCount;
                }
            }
        }

        //三连类型
        {
            //恢复扑克，防止分析时改变扑克
            memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount);

            //连牌判断
            uint8_t cbFirstCard = 0;
            //去除2和王
            for (uint8_t i = 0; i < cbHandCardCount; ++i)
                if (GetCardLogicValue(cbTmpCardData[i]) < 15)
                {
                    cbFirstCard = i;
                    break;
                }

            uint8_t cbLeftCardCount = cbHandCardCount - cbFirstCard;
            bool bFindThreeLine = true;
            uint8_t cbThreeLineCount = 0;
            uint8_t cbThreeLineCard[20];
            //开始判断
            while (cbLeftCardCount >= 6 && bFindThreeLine)
            {
                uint8_t cbLastCard = cbTmpCardData[cbFirstCard];
                uint8_t cbSameCount = 1;
                cbThreeLineCount = 0;
                bFindThreeLine = false;
                for (uint8_t i = cbFirstCard + 1; i < cbLeftCardCount + cbFirstCard; ++i)
                {
                    //搜索同牌
                    while (GetCardLogicValue(cbLastCard) == GetCardLogicValue(cbTmpCardData[i]) &&
                           i < cbLeftCardCount + cbFirstCard)
                    {
                        ++cbSameCount;
                        ++i;
                    }

                    uint8_t cbLastThreeCardValue;
                    if (cbThreeLineCount > 0)
                        cbLastThreeCardValue = GetCardLogicValue(cbThreeLineCard[cbThreeLineCount - 1]);

                    //重新开始
                    if ((cbSameCount < 3 ||
                         (cbThreeLineCount > 0 && (cbLastThreeCardValue - GetCardLogicValue(cbLastCard)) != 1))
                        &&
                        i <= cbLeftCardCount + cbFirstCard)
                    {
                        if (cbThreeLineCount >= 6) break;

                        if (cbSameCount >= 3) i -= cbSameCount;
                        cbLastCard = cbTmpCardData[i];
                        cbThreeLineCount = 0;
                    }
                        //保存数据
                    else if (cbSameCount >= 3)
                    {
                        cbThreeLineCard[cbThreeLineCount] = cbTmpCardData[i - cbSameCount];
                        cbThreeLineCard[cbThreeLineCount + 1] = cbTmpCardData[i - cbSameCount + 1];
                        cbThreeLineCard[cbThreeLineCount + 2] = cbTmpCardData[i - cbSameCount + 2];
                        cbThreeLineCount += 3;

                        //结尾判断
                        if (i == (cbLeftCardCount + cbFirstCard - 3))
                            if ((GetCardLogicValue(cbLastCard) - GetCardLogicValue(cbTmpCardData[i])) == 1 &&
                                (GetCardLogicValue(cbTmpCardData[i])
                                 == GetCardLogicValue(cbTmpCardData[i + 1])) &&
                                (GetCardLogicValue(cbTmpCardData[i])
                                 == GetCardLogicValue(cbTmpCardData[i + 2])))
                            {
                                cbThreeLineCard[cbThreeLineCount] = cbTmpCardData[i];
                                cbThreeLineCard[cbThreeLineCount + 1] = cbTmpCardData[i + 1];
                                cbThreeLineCard[cbThreeLineCount + 2] = cbTmpCardData[i + 2];
                                cbThreeLineCount += 3;
                                break;
                            }

                    }

                    cbLastCard = cbTmpCardData[i];
                    cbSameCount = 1;
                }

                //保存数据
                if (cbThreeLineCount >= 6)
                {
                    uint8_t Index;

                    Index = CardTypeResult[CT_THREE_LINE].cbCardTypeCount;
                    CardTypeResult[CT_THREE_LINE].cbCardType = CT_THREE_LINE;
                    memcpy(CardTypeResult[CT_THREE_LINE].cbCardData[Index], cbThreeLineCard,
                           sizeof(uint8_t) * cbThreeLineCount);
                    CardTypeResult[CT_THREE_LINE].cbEachHandCardCount[Index] = cbThreeLineCount;
                    CardTypeResult[CT_THREE_LINE].cbCardTypeCount++;

                    ASSERT(CardTypeResult[CT_THREE_LINE].cbCardTypeCount < MAX_TYPE_COUNT);

                    RemoveCardList(cbThreeLineCard, cbThreeLineCount, cbTmpCardData, cbFirstCard + cbLeftCardCount);
                    bFindThreeLine = true;
                    cbLeftCardCount -= cbThreeLineCount;
                }
            }

        }
        //三带一单
        {
            //恢复扑克，防止分析时改变扑克
            memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount);

            uint8_t cbHandThreeCard[MAX_LAND_COUNT];
            uint8_t cbHandThreeCount = 0;

            //移除炸弹
            uint8_t cbAllBomCardData[MAX_LAND_COUNT];
            uint8_t cbAllBomCardCount = 0;
            GetAllBomCard(cbTmpCardData, cbHandCardCount, cbAllBomCardData, cbAllBomCardCount);
            RemoveCardList(cbAllBomCardData, cbAllBomCardCount, cbTmpCardData, cbHandCardCount);

            GetAllThreeCard(cbTmpCardData, cbHandCardCount - cbAllBomCardCount, cbHandThreeCard, cbHandThreeCount);

            {
                uint8_t Index;
                //去掉三条
                uint8_t cbRemainCardData[MAX_LAND_COUNT];
                memcpy(cbRemainCardData, cbTmpCardData, cbHandCardCount - cbAllBomCardCount);
                uint8_t cbRemainCardCount = cbHandCardCount - cbAllBomCardCount - cbHandThreeCount;
                RemoveCardList(cbHandThreeCard, cbHandThreeCount, cbRemainCardData,
                               cbHandCardCount - cbAllBomCardCount);
                //三条带一张
                for (uint8_t i = 0; i < cbHandThreeCount; i += 3)
                {
                    //三条带一张
                    for (uint8_t j = 0; j < cbRemainCardCount; ++j)
                    {
                        Index = CardTypeResult[CT_THREE_TAKE_ONE].cbCardTypeCount;
                        CardTypeResult[CT_THREE_TAKE_ONE].cbCardType = CT_THREE_TAKE_ONE;
                        CardTypeResult[CT_THREE_TAKE_ONE].cbCardData[Index][0] = cbHandThreeCard[i];
                        CardTypeResult[CT_THREE_TAKE_ONE].cbCardData[Index][1] = cbHandThreeCard[i + 1];
                        CardTypeResult[CT_THREE_TAKE_ONE].cbCardData[Index][2] = cbHandThreeCard[i + 2];
                        CardTypeResult[CT_THREE_TAKE_ONE].cbCardData[Index][3] = cbRemainCardData[j];
                        CardTypeResult[CT_THREE_TAKE_ONE].cbEachHandCardCount[Index] = 4;
                        CardTypeResult[CT_THREE_TAKE_ONE].cbCardTypeCount++;
                    }
                }
            }

            //三连带单
            uint8_t cbLeftThreeCardCount = cbHandThreeCount;
            bool bFindThreeLine = true;
            uint8_t cbLastIndex = 0;
            if (GetCardLogicValue(cbHandThreeCard[0]) == 15) cbLastIndex = 3;
            while (cbLeftThreeCardCount >= 6 && bFindThreeLine)
            {
                uint8_t cbLastLogicCard = GetCardLogicValue(cbHandThreeCard[cbLastIndex]);
                uint8_t cbThreeLineCard[MAX_LAND_COUNT];
                uint8_t cbThreeLineCardCount = 3;
                cbThreeLineCard[0] = cbHandThreeCard[cbLastIndex];
                cbThreeLineCard[1] = cbHandThreeCard[cbLastIndex + 1];
                cbThreeLineCard[2] = cbHandThreeCard[cbLastIndex + 2];

                bFindThreeLine = false;
                for (uint8_t j = 3 + cbLastIndex; j < cbLeftThreeCardCount; j += 3)
                {
                    //连续判断
                    if (1 != (cbLastLogicCard - (GetCardLogicValue(cbHandThreeCard[j]))))
                    {
                        cbLastIndex = j;
                        if (cbLeftThreeCardCount - j >= 6) bFindThreeLine = true;

                        break;
                    }

                    cbLastLogicCard = GetCardLogicValue(cbHandThreeCard[j]);
                    cbThreeLineCard[cbThreeLineCardCount] = cbHandThreeCard[j];
                    cbThreeLineCard[cbThreeLineCardCount + 1] = cbHandThreeCard[j + 1];
                    cbThreeLineCard[cbThreeLineCardCount + 2] = cbHandThreeCard[j + 2];
                    cbThreeLineCardCount += 3;
                }
                if (cbThreeLineCardCount > 3)
                {
                    uint8_t Index;

                    uint8_t cbRemainCard[MAX_LAND_COUNT];
                    uint8_t cbRemainCardCount = cbHandCardCount - cbAllBomCardCount - cbHandThreeCount;


                    //移除三条（还应该移除炸弹王等）
                    memcpy(cbRemainCard, cbTmpCardData, (cbHandCardCount - cbAllBomCardCount) * sizeof(uint8_t));
                    RemoveCardList(cbHandThreeCard, cbHandThreeCount, cbRemainCard,
                                   cbHandCardCount - cbAllBomCardCount);

                    for (uint8_t start = 0; start < cbThreeLineCardCount - 3; start += 3)
                    {
                        //本顺数目
                        uint8_t cbThisTreeLineCardCount = cbThreeLineCardCount - start;
                        //单牌个数
                        uint8_t cbSingleCardCount = (cbThisTreeLineCardCount) / 3;

                        //单牌不够
                        if (cbRemainCardCount < cbSingleCardCount) continue;

                        //单牌组合
                        uint8_t cbComCard[5];
                        uint8_t cbComResCard[254][5];
                        uint8_t cbComResLen = 0;

                        Combination(cbComCard, 0, cbComResCard, cbComResLen, cbRemainCard, cbSingleCardCount,
                                    cbRemainCardCount, cbSingleCardCount);
                        for (uint8_t i = 0; i < cbComResLen; ++i)
                        {
                            Index = CardTypeResult[CT_THREE_TAKE_ONE].cbCardTypeCount;
                            CardTypeResult[CT_THREE_TAKE_ONE].cbCardType = CT_THREE_TAKE_ONE;
                            //保存三条
                            memcpy(CardTypeResult[CT_THREE_TAKE_ONE].cbCardData[Index], cbThreeLineCard + start,
                                   sizeof(uint8_t) * cbThisTreeLineCardCount);
                            //保存单牌
                            memcpy(CardTypeResult[CT_THREE_TAKE_ONE].cbCardData[Index] + cbThisTreeLineCardCount,
                                   cbComResCard[i], cbSingleCardCount);

                            CardTypeResult[CT_THREE_TAKE_ONE].cbEachHandCardCount[Index] =
                                    cbThisTreeLineCardCount + cbSingleCardCount;
                            CardTypeResult[CT_THREE_TAKE_ONE].cbCardTypeCount++;

                            ASSERT(CardTypeResult[CT_THREE_TAKE_ONE].cbCardTypeCount < MAX_TYPE_COUNT);
                        }

                    }

                    //移除三连
                    bFindThreeLine = true;
                    RemoveCardList(cbThreeLineCard, cbThreeLineCardCount, cbHandThreeCard, cbLeftThreeCardCount);
                    cbLeftThreeCardCount -= cbThreeLineCardCount;
                }
            }
        }

        //三带一对
        {
            //恢复扑克，防止分析时改变扑克
            memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount);

            uint8_t cbHandThreeCard[MAX_LAND_COUNT];
            uint8_t cbHandThreeCount = 0;
            uint8_t cbRemainCarData[MAX_LAND_COUNT];
            uint8_t cbRemainCardCount = 0;

            //抽取三条
            GetAllThreeCard(cbTmpCardData, cbHandCardCount, cbHandThreeCard, cbHandThreeCount);

            //移除三条（还应该移除炸弹王等）
            memcpy(cbRemainCarData, cbTmpCardData, cbHandCardCount);
            RemoveCardList(cbHandThreeCard, cbHandThreeCount, cbRemainCarData, cbHandCardCount);
            cbRemainCardCount = cbHandCardCount - cbHandThreeCount;

            //抽取对牌
            uint8_t cbAllDoubleCardData[MAX_LAND_COUNT];
            uint8_t cbAllDoubleCardCount = 0;
            GetAllDoubleCard(cbRemainCarData, cbRemainCardCount, cbAllDoubleCardData, cbAllDoubleCardCount);

            //三条带一对
            for (uint8_t i = 0; i < cbHandThreeCount; i += 3)
            {
                uint8_t Index;

                //三条带一张
                for (uint8_t j = 0; j < cbAllDoubleCardCount; j += 2)
                {
                    Index = CardTypeResult[CT_THREE_TAKE_TWO].cbCardTypeCount;
                    CardTypeResult[CT_THREE_TAKE_TWO].cbCardType = CT_THREE_TAKE_TWO;
                    CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index][0] = cbHandThreeCard[i];
                    CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index][1] = cbHandThreeCard[i + 1];
                    CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index][2] = cbHandThreeCard[i + 2];
                    CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index][3] = cbAllDoubleCardData[j];
                    CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index][4] = cbAllDoubleCardData[j + 1];
                    CardTypeResult[CT_THREE_TAKE_TWO].cbEachHandCardCount[Index] = 5;
                    CardTypeResult[CT_THREE_TAKE_TWO].cbCardTypeCount++;
                }
            }

            //三连带对
            uint8_t cbLeftThreeCardCount = cbHandThreeCount;
            bool bFindThreeLine = true;
            uint8_t cbLastIndex = 0;
            if (GetCardLogicValue(cbHandThreeCard[0]) == 15) cbLastIndex = 3;
            while (cbLeftThreeCardCount >= 6 && bFindThreeLine)
            {
                uint8_t cbLastLogicCard = GetCardLogicValue(cbHandThreeCard[cbLastIndex]);
                uint8_t cbThreeLineCard[MAX_LAND_COUNT];
                uint8_t cbThreeLineCardCount = 3;
                cbThreeLineCard[0] = cbHandThreeCard[cbLastIndex];
                cbThreeLineCard[1] = cbHandThreeCard[cbLastIndex + 1];
                cbThreeLineCard[2] = cbHandThreeCard[cbLastIndex + 2];

                bFindThreeLine = false;
                for (uint8_t j = 3 + cbLastIndex; j < cbLeftThreeCardCount; j += 3)
                {
                    //连续判断
                    if (1 != (cbLastLogicCard - (GetCardLogicValue(cbHandThreeCard[j]))))
                    {
                        cbLastIndex = j;
                        if (cbLeftThreeCardCount - j >= 6) bFindThreeLine = true;

                        break;
                    }

                    cbLastLogicCard = GetCardLogicValue(cbHandThreeCard[j]);
                    cbThreeLineCard[cbThreeLineCardCount] = cbHandThreeCard[j];
                    cbThreeLineCard[cbThreeLineCardCount + 1] = cbHandThreeCard[j + 1];
                    cbThreeLineCard[cbThreeLineCardCount + 2] = cbHandThreeCard[j + 2];
                    cbThreeLineCardCount += 3;
                }
                if (cbThreeLineCardCount > 3)
                {
                    uint8_t Index;

                    for (uint8_t start = 0; start < cbThreeLineCardCount - 3; start += 3)
                    {
                        //本顺数目
                        uint8_t cbThisTreeLineCardCount = cbThreeLineCardCount - start;
                        //对牌张数
                        uint8_t cbDoubleCardCount = ((cbThisTreeLineCardCount) / 3);

                        //对牌不够
                        if (cbRemainCardCount < cbDoubleCardCount) continue;

                        uint8_t cbDoubleCardIndex[10]; //对牌下标
                        for (uint8_t i = 0, j = 0; i < cbAllDoubleCardCount; i += 2, ++j)
                            cbDoubleCardIndex[j] = i;

                        //对牌组合
                        uint8_t cbComCard[5];
                        uint8_t cbComResCard[254][5];
                        uint8_t cbComResLen = 0;

                        //利用对牌的下标做组合，再根据下标提取出对牌
                        Combination(cbComCard, 0, cbComResCard, cbComResLen, cbDoubleCardIndex, cbDoubleCardCount,
                                    cbAllDoubleCardCount / 2, cbDoubleCardCount);

                        ASSERT(cbComResLen <= 254);

                        for (uint8_t i = 0; i < cbComResLen; ++i)
                        {
                            Index = CardTypeResult[CT_THREE_TAKE_TWO].cbCardTypeCount;
                            CardTypeResult[CT_THREE_TAKE_TWO].cbCardType = CT_THREE_TAKE_TWO;
                            //保存三条
                            memcpy(CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index], cbThreeLineCard + start,
                                   sizeof(uint8_t) * cbThisTreeLineCardCount);
                            //保存对牌
                            for (uint8_t j = 0, k = 0; j < cbDoubleCardCount; ++j, k += 2)
                            {
                                CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index][cbThisTreeLineCardCount +
                                                                                    k] = cbAllDoubleCardData[cbComResCard[i][j]];
                                CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index][cbThisTreeLineCardCount + k +
                                                                                    1] = cbAllDoubleCardData[
                                        cbComResCard[i][j] + 1];
                            }

                            CardTypeResult[CT_THREE_TAKE_TWO].cbEachHandCardCount[Index] =
                                    cbThisTreeLineCardCount + 2 * cbDoubleCardCount;
                            CardTypeResult[CT_THREE_TAKE_TWO].cbCardTypeCount++;

                            ASSERT(CardTypeResult[CT_THREE_TAKE_TWO].cbCardTypeCount < MAX_TYPE_COUNT);
                        }

                    }
                    //移除三连
                    bFindThreeLine = true;
                    RemoveCardList(cbThreeLineCard, cbThreeLineCardCount, cbHandThreeCard, cbLeftThreeCardCount);
                    cbLeftThreeCardCount -= cbThreeLineCardCount;
                }
            }
        }
        //四带两单
        /*{
        	//恢复扑克，防止分析时改变扑克
        	memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

        	uint8 cbFirstCard = 0 ;
        	//去除王牌
        	for(uint8 i=0 ; i<cbHandCardCount ; ++i)	if(GetCardColor(cbTmpCardData[i])!=0x40)	{cbFirstCard = i ; break ;}

        	uint8 cbHandAllFourCardData[MAX_LAND_COUNT] ;
        	uint8 cbHandAllFourCardCount=0;
        	//抽取四张
        	GetAllBomCard(cbTmpCardData+cbFirstCard, cbHandCardCount-cbFirstCard, cbHandAllFourCardData, cbHandAllFourCardCount) ;

        	//移除四条
        	uint8 cbRemainCard[MAX_LAND_COUNT];
        	uint8 cbRemainCardCount=cbHandCardCount-cbHandAllFourCardCount ;
        	memcpy(cbRemainCard, cbTmpCardData, cbHandCardCount*sizeof(uint8));
        	RemoveCardList(cbHandAllFourCardData, cbHandAllFourCardCount, cbRemainCard, cbHandCardCount) ;

        	for(uint8 Start=0; Start<cbHandAllFourCardCount; Start += 4)
        	{
            	uint8 Index ;
            	//单牌组合
            	uint8 cbComCard[5];
            	uint8 cbComResCard[254][5] ;
            	uint8 cbComResLen=0 ;
            	//单牌组合
            	Combination(cbComCard, 0, cbComResCard, cbComResLen, cbRemainCard, 2, cbRemainCardCount, 2);
            	for(uint8 i=0; i<cbComResLen; ++i)
            	{
                	//不能带对
                	if(GetCardValue(cbComResCard[i][0])==GetCardValue(cbComResCard[i][1])) continue ;

                	Index=CardTypeResult[CT_FOUR_TAKE_ONE].cbCardTypeCount ;
                	CardTypeResult[CT_FOUR_TAKE_ONE].cbCardType = CT_FOUR_TAKE_ONE ;
                	memcpy(CardTypeResult[CT_FOUR_TAKE_ONE].cbCardData[Index], cbHandAllFourCardData+Start, 4) ;
                	CardTypeResult[CT_FOUR_TAKE_ONE].cbCardData[Index][4] = cbComResCard[i][0] ;
                	CardTypeResult[CT_FOUR_TAKE_ONE].cbCardData[Index][4+1] = cbComResCard[i][1] ;
                	CardTypeResult[CT_FOUR_TAKE_ONE].cbEachHandCardCount[Index] = 6 ;
                	CardTypeResult[CT_FOUR_TAKE_ONE].cbCardTypeCount++ ;

                	ASSERT(CardTypeResult[CT_FOUR_TAKE_ONE].cbCardTypeCount<MAX_TYPE_COUNT) ;
            	}
        	}
    	}
		*/

        //四带两对
        /*
    	{
        	//恢复扑克，防止分析时改变扑克
        	memcpy(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

        	uint8 cbFirstCard = 0 ;
        	//去除王牌
        	for(uint8 i=0 ; i<cbHandCardCount ; ++i)	if(GetCardColor(cbTmpCardData[i])!=0x40)	{cbFirstCard = i ; break ;}

        	uint8 cbHandAllFourCardData[MAX_LAND_COUNT] ;
        	uint8 cbHandAllFourCardCount=0;

        	//抽取四张
        	GetAllBomCard(cbTmpCardData+cbFirstCard, cbHandCardCount-cbFirstCard, cbHandAllFourCardData, cbHandAllFourCardCount) ;

        	//移除四条
        	uint8 cbRemainCard[MAX_LAND_COUNT];
        	uint8 cbRemainCardCount=cbHandCardCount-cbHandAllFourCardCount ;
        	memcpy(cbRemainCard, cbTmpCardData, cbHandCardCount*sizeof(uint8));
        	RemoveCardList(cbHandAllFourCardData, cbHandAllFourCardCount, cbRemainCard, cbHandCardCount) ;

        	for(uint8 Start=0; Start<cbHandAllFourCardCount; Start += 4)
        	{
            	//抽取对牌
            	uint8 cbAllDoubleCardData[MAX_LAND_COUNT] ;
            	uint8 cbAllDoubleCardCount=0 ;
            	GetAllDoubleCard(cbRemainCard, cbRemainCardCount, cbAllDoubleCardData, cbAllDoubleCardCount) ;

            	uint8 cbDoubleCardIndex[10]; //对牌下标
            	for(uint8 i=0, j=0; i<cbAllDoubleCardCount; i+=2, ++j)
            	    cbDoubleCardIndex[j]=i ;

            	//对牌组合
            	uint8 cbComCard[5];
            	uint8 cbComResCard[255][5] ;
            	uint8 cbComResLen=0 ;

            	//利用对牌的下标做组合，再根据下标提取出对牌
            	Combination(cbComCard, 0, cbComResCard, cbComResLen, cbDoubleCardIndex, 2, cbAllDoubleCardCount/2, 2);
            	for(uint8 i=0; i<cbComResLen; ++i)
            	{
                	uint8 Index = CardTypeResult[CT_FOUR_TAKE_TWO].cbCardTypeCount ;
                	CardTypeResult[CT_FOUR_TAKE_TWO].cbCardType = CT_FOUR_TAKE_TWO ;
                	memcpy(CardTypeResult[CT_FOUR_TAKE_TWO].cbCardData[Index], cbHandAllFourCardData+Start, 4) ;

                	//保存对牌
                	for(uint8 j=0, k=0; j<4; ++j, k+=2)
                	{
                    	CardTypeResult[CT_FOUR_TAKE_TWO].cbCardData[Index][4+k] = cbAllDoubleCardData[cbComResCard[i][j]];
                    	CardTypeResult[CT_FOUR_TAKE_TWO].cbCardData[Index][4+k+1] = cbAllDoubleCardData[cbComResCard[i][j]+1];
                    }

                	CardTypeResult[CT_FOUR_TAKE_TWO].cbEachHandCardCount[Index] = 8 ;
                	CardTypeResult[CT_FOUR_TAKE_TWO].cbCardTypeCount++ ;

                	ASSERT(CardTypeResult[CT_FOUR_TAKE_TWO].cbCardTypeCount<MAX_TYPE_COUNT) ;
            	}
        	}
    	}*/

    }

//分析牌型（后出牌调用）
    void
    CLandLogic::AnalyseOutCardType(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount,
                                   uint8_t const cbTurnCardData[],
                                   uint8_t const cbTurnCardCount, tagOutCardTypeResult CardTypeResult[12 + 1]) {
        memset(CardTypeResult, 0, sizeof(CardTypeResult[0]) * 12);
        //数据校验
        if (cbHandCardCount < cbTurnCardCount) return;

        uint8_t cbTmpCard[MAX_LAND_COUNT];
        memcpy(cbTmpCard, cbHandCardData, cbHandCardCount);
        SortCardList(cbTmpCard, cbHandCardCount, ST_ORDER);
        //	SortCardList(cbTurnCardData, cbTurnCardCount, ST_ORDER) ;

        uint8_t cbTurnCardType = GetCardType(cbTurnCardData, cbTurnCardCount);
        if (cbTurnCardType == CT_ERROR)
        {
            LOG_ERROR("ASSERT(cbTurnCardType!=CT_ERROR);");
            return;
        }

        if (cbTurnCardType != CT_MISSILE_CARD && cbTurnCardType != CT_BOMB_CARD)
        {
            //双王炸弹
            if (cbHandCardCount >= 2 && 0x4F == cbTmpCard[0] && 0x4E == cbTmpCard[1])
            {
                uint8_t Index = CardTypeResult[CT_BOMB_CARD].cbCardTypeCount;
                CardTypeResult[CT_BOMB_CARD].cbCardType = CT_BOMB_CARD;
                CardTypeResult[CT_BOMB_CARD].cbCardData[Index][0] = cbTmpCard[0];
                CardTypeResult[CT_BOMB_CARD].cbCardData[Index][1] = cbTmpCard[1];
                CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] = 2;
                CardTypeResult[CT_BOMB_CARD].cbCardTypeCount++;

                uint8_t cbBomCardData[MAX_LAND_COUNT];
                uint8_t cbBomCardCount = 0;
                GetAllBomCard(cbTmpCard + 2, cbHandCardCount - 2, cbBomCardData, cbBomCardCount);
                for (uint8_t i = 0; i < cbBomCardCount / 4; ++i)
                {
                    Index = CardTypeResult[CT_BOMB_CARD].cbCardTypeCount;
                    CardTypeResult[CT_BOMB_CARD].cbCardType = CT_BOMB_CARD;
                    memcpy(CardTypeResult[CT_BOMB_CARD].cbCardData[Index], cbBomCardData + 4 * i, 4);
                    CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] = 4;
                    CardTypeResult[CT_BOMB_CARD].cbCardTypeCount++;

                    ASSERT(CardTypeResult[CT_BOMB_CARD].cbCardTypeCount <= MAX_TYPE_COUNT);
                }
            }
                //炸弹牌型
            else
            {
                uint8_t cbBomCardData[MAX_LAND_COUNT];
                uint8_t cbBomCardCount = 0;
                GetAllBomCard(cbTmpCard, cbHandCardCount, cbBomCardData, cbBomCardCount);
                for (uint8_t i = 0; i < cbBomCardCount / 4; ++i)
                {
                    uint8_t Index = CardTypeResult[CT_BOMB_CARD].cbCardTypeCount;
                    CardTypeResult[CT_BOMB_CARD].cbCardType = CT_BOMB_CARD;
                    memcpy(CardTypeResult[CT_BOMB_CARD].cbCardData[Index], cbBomCardData + 4 * i, 4);
                    CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] = 4;
                    CardTypeResult[CT_BOMB_CARD].cbCardTypeCount++;

                    ASSERT(CardTypeResult[CT_BOMB_CARD].cbCardTypeCount <= MAX_TYPE_COUNT);
                }
            }
        }

        switch (cbTurnCardType)
        {
            case CT_SINGLE:                //单牌类型
            {
                for (uint8_t i = 0; i < cbHandCardCount; ++i)
                {
                    if (GetCardLogicValue(cbTmpCard[i]) > GetCardLogicValue(cbTurnCardData[0]))
                    {
                        uint8_t Index = CardTypeResult[CT_SINGLE].cbCardTypeCount;
                        CardTypeResult[CT_SINGLE].cbCardType = CT_SINGLE;
                        CardTypeResult[CT_SINGLE].cbCardData[Index][0] = cbTmpCard[i];
                        CardTypeResult[CT_SINGLE].cbEachHandCardCount[Index] = 1;
                        CardTypeResult[CT_SINGLE].cbCardTypeCount++;

                        ASSERT(CardTypeResult[CT_SINGLE].cbCardTypeCount <= MAX_TYPE_COUNT);
                    }
                    break;
                }
            }
            case CT_DOUBLE:                //对牌类型
            {
                //扑克分析
                for (uint8_t i = 0; i < cbHandCardCount; i++)
                {
                    //变量定义
                    uint8_t cbSameCount = 1;
                    uint8_t cbLogicValue = GetCardLogicValue(cbTmpCard[i]);

                    //搜索同牌
                    for (uint8_t j = i + 1; j < cbHandCardCount; j++)
                    {
                        //获取扑克
                        if (GetCardLogicValue(cbTmpCard[j]) != cbLogicValue) break;

                        //设置变量
                        cbSameCount++;
                    }

                    if (cbSameCount >= 2 && GetCardLogicValue(cbTmpCard[i]) > GetCardLogicValue(cbTurnCardData[0]))
                    {
                        uint8_t Index = CardTypeResult[CT_DOUBLE].cbCardTypeCount;
                        CardTypeResult[CT_DOUBLE].cbCardType = CT_DOUBLE;
                        CardTypeResult[CT_DOUBLE].cbCardData[Index][0] = cbTmpCard[i];
                        CardTypeResult[CT_DOUBLE].cbCardData[Index][1] = cbTmpCard[i + 1];
                        CardTypeResult[CT_DOUBLE].cbEachHandCardCount[Index] = 2;
                        CardTypeResult[CT_DOUBLE].cbCardTypeCount++;

                        ASSERT(CardTypeResult[CT_DOUBLE].cbCardTypeCount <= MAX_TYPE_COUNT);
                    }
                    //设置索引
                    i += cbSameCount - 1;
                }
                break;
            }
            case CT_THREE:                //三条类型
            {
                //扑克分析
                for (uint8_t i = 0; i < cbHandCardCount; i++)
                {
                    //变量定义
                    uint8_t cbSameCount = 1;
                    uint8_t cbLogicValue = GetCardLogicValue(cbTmpCard[i]);

                    //搜索同牌
                    for (uint8_t j = i + 1; j < cbHandCardCount; j++)
                    {
                        //获取扑克
                        if (GetCardLogicValue(cbTmpCard[j]) != cbLogicValue) break;

                        //设置变量
                        cbSameCount++;
                    }

                    if (cbSameCount >= 3 && GetCardLogicValue(cbTmpCard[i]) > GetCardLogicValue(cbTurnCardData[0]))
                    {
                        uint8_t Index = CardTypeResult[CT_THREE].cbCardTypeCount;
                        CardTypeResult[CT_THREE].cbCardType = CT_THREE;
                        CardTypeResult[CT_THREE].cbCardData[Index][0] = cbTmpCard[i];
                        CardTypeResult[CT_THREE].cbCardData[Index][1] = cbTmpCard[i + 1];
                        CardTypeResult[CT_THREE].cbCardData[Index][2] = cbTmpCard[i + 2];
                        CardTypeResult[CT_THREE].cbEachHandCardCount[Index] = 3;
                        CardTypeResult[CT_THREE].cbCardTypeCount++;

                        ASSERT(CardTypeResult[CT_THREE].cbCardTypeCount <= MAX_TYPE_COUNT);
                    }
                    //设置索引
                    i += cbSameCount - 1;
                }
                break;
            }
            case CT_SINGLE_LINE:        //单连类型
            {
                uint8_t cbFirstCard = 0;
                //去除2和王
                for (uint8_t i = 0; i < cbHandCardCount; ++i)
                    if (GetCardLogicValue(cbTmpCard[i]) < 15)
                    {
                        cbFirstCard = i;
                        break;
                    }

                uint8_t cbSingleLineCard[12];
                uint8_t cbSingleLineCount = 1;
                uint8_t cbLeftCardCount = cbHandCardCount;
                bool bFindSingleLine = true;

                //连牌判断
                while (cbLeftCardCount >= cbTurnCardCount && bFindSingleLine)
                {
                    cbSingleLineCount = 1;
                    bFindSingleLine = false;
                    uint8_t cbLastCard = cbTmpCard[cbFirstCard];
                    cbSingleLineCard[cbSingleLineCount - 1] = cbTmpCard[cbFirstCard];
                    for (uint8_t i = cbFirstCard + 1; i < cbLeftCardCount; i++)
                    {
                        uint8_t cbCardData = cbTmpCard[i];

                        //连续判断
                        if (1 != (GetCardLogicValue(cbLastCard) - GetCardLogicValue(cbCardData)) &&
                            GetCardValue(cbLastCard) != GetCardValue(cbCardData))
                        {
                            cbLastCard = cbTmpCard[i];
                            //是否合法
                            if (cbSingleLineCount < cbTurnCardCount)
                            {
                                cbSingleLineCount = 1;
                                cbSingleLineCard[cbSingleLineCount - 1] = cbTmpCard[i];
                                continue;
                            }
                            else break;
                        }
                            //同牌判断
                        else if (GetCardValue(cbLastCard) != GetCardValue(cbCardData))
                        {
                            cbLastCard = cbCardData;
                            cbSingleLineCard[cbSingleLineCount] = cbCardData;
                            ++cbSingleLineCount;
                        }
                    }

                    //保存数据
                    if (cbSingleLineCount >= cbTurnCardCount &&
                        GetCardLogicValue(cbSingleLineCard[0]) > GetCardLogicValue(cbTurnCardData[0]))
                    {
                        uint8_t Index;
                        uint8_t cbStart = 0;
                        //所有连牌
                        while (GetCardLogicValue(cbSingleLineCard[cbStart]) > GetCardLogicValue(cbTurnCardData[0]) &&
                               ((cbSingleLineCount - cbStart) >= cbTurnCardCount))
                        {
                            Index = CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount;
                            CardTypeResult[CT_SINGLE_LINE].cbCardType = CT_SINGLE_LINE;
                            memcpy(CardTypeResult[CT_SINGLE_LINE].cbCardData[Index], cbSingleLineCard + cbStart,
                                   sizeof(uint8_t) * cbTurnCardCount);
                            CardTypeResult[CT_SINGLE_LINE].cbEachHandCardCount[Index] = cbTurnCardCount;
                            CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount++;
                            cbStart++;

                            ASSERT(CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount <= MAX_TYPE_COUNT);
                        }

                        RemoveCardList(cbSingleLineCard, cbSingleLineCount, cbTmpCard, cbLeftCardCount);
                        cbLeftCardCount -= cbSingleLineCount;
                        bFindSingleLine = true;
                    }
                }

                break;
            }
            case CT_DOUBLE_LINE:        //对连类型
            {
                //连牌判断
                uint8_t cbFirstCard = 0;
                //去除2和王
                for (uint8_t i = 0; i < cbHandCardCount; ++i)
                    if (GetCardLogicValue(cbTmpCard[i]) < 15)
                    {
                        cbFirstCard = i;
                        break;
                    }

                uint8_t cbLeftCardCount = cbHandCardCount - cbFirstCard;
                bool bFindDoubleLine = true;
                uint8_t cbDoubleLineCount = 0;
                uint8_t cbDoubleLineCard[24];
                //开始判断
                while (cbLeftCardCount >= cbTurnCardCount && bFindDoubleLine)
                {
                    uint8_t cbLastCard = cbTmpCard[cbFirstCard];
                    uint8_t cbSameCount = 1;
                    cbDoubleLineCount = 0;
                    bFindDoubleLine = false;
                    for (uint8_t i = cbFirstCard + 1; i < cbLeftCardCount + cbFirstCard; ++i)
                    {
                        //搜索同牌
                        while (GetCardValue(cbLastCard) == GetCardValue(cbTmpCard[i]) &&
                               i < cbLeftCardCount + cbFirstCard)
                        {
                            ++cbSameCount;
                            ++i;
                        }

                        uint8_t cbLastDoubleCardValue;
                        if (cbDoubleLineCount > 0)
                            cbLastDoubleCardValue = GetCardLogicValue(cbDoubleLineCard[cbDoubleLineCount - 1]);
                        //重新开始
                        if ((cbSameCount < 2 ||
                             (cbDoubleLineCount > 0
                              && (cbLastDoubleCardValue - GetCardLogicValue(cbLastCard)) != 1)) &&
                            i <= cbLeftCardCount + cbFirstCard)
                        {
                            if (cbDoubleLineCount >= cbTurnCardCount) break;

                            if (cbSameCount >= 2) i -= cbSameCount;

                            cbLastCard = cbTmpCard[i];
                            cbDoubleLineCount = 0;
                        }
                            //保存数据
                        else if (cbSameCount >= 2)
                        {
                            cbDoubleLineCard[cbDoubleLineCount] = cbTmpCard[i - cbSameCount];
                            cbDoubleLineCard[cbDoubleLineCount + 1] = cbTmpCard[i - cbSameCount + 1];
                            cbDoubleLineCount += 2;

                            //结尾判断
                            if (i == (cbLeftCardCount + cbFirstCard - 2))
                                if ((GetCardLogicValue(cbLastCard) - GetCardLogicValue(cbTmpCard[i])) == 1 &&
                                    (GetCardLogicValue(cbTmpCard[i]) == GetCardLogicValue(cbTmpCard[i + 1])))
                                {
                                    cbDoubleLineCard[cbDoubleLineCount] = cbTmpCard[i];
                                    cbDoubleLineCard[cbDoubleLineCount + 1] = cbTmpCard[i + 1];
                                    cbDoubleLineCount += 2;
                                    break;
                                }

                        }

                        cbLastCard = cbTmpCard[i];
                        cbSameCount = 1;
                    }

                    //保存数据
                    if (cbDoubleLineCount >= cbTurnCardCount)
                    {
                        uint8_t Index;
                        uint8_t cbStart = 0;
                        //所有连牌
                        while (GetCardLogicValue(cbDoubleLineCard[cbStart]) > GetCardLogicValue(cbTurnCardData[0]) &&
                               ((cbDoubleLineCount - cbStart) >= cbTurnCardCount))
                        {
                            Index = CardTypeResult[CT_DOUBLE_LINE].cbCardTypeCount;
                            CardTypeResult[CT_DOUBLE_LINE].cbCardType = CT_DOUBLE_LINE;
                            memcpy(CardTypeResult[CT_DOUBLE_LINE].cbCardData[Index], cbDoubleLineCard + cbStart,
                                   sizeof(uint8_t) * cbTurnCardCount);
                            CardTypeResult[CT_DOUBLE_LINE].cbEachHandCardCount[Index] = cbTurnCardCount;
                            CardTypeResult[CT_DOUBLE_LINE].cbCardTypeCount++;
                            cbStart += 2;

                            ASSERT(CardTypeResult[CT_DOUBLE_LINE].cbCardTypeCount <= MAX_TYPE_COUNT);
                        }
                        RemoveCardList(cbDoubleLineCard, cbDoubleLineCount, cbTmpCard, cbFirstCard + cbLeftCardCount);
                        bFindDoubleLine = true;
                        cbLeftCardCount -= cbDoubleLineCount;
                    }
                }

                break;
            }
            case CT_THREE_LINE:            //三连类型
            {
                //连牌判断
                uint8_t cbFirstCard = 0;
                //去除2和王
                for (uint8_t i = 0; i < cbHandCardCount; ++i)
                    if (GetCardLogicValue(cbTmpCard[i]) < 15)
                    {
                        cbFirstCard = i;
                        break;
                    }

                uint8_t cbLeftCardCount = cbHandCardCount - cbFirstCard;
                bool bFindThreeLine = true;
                uint8_t cbThreeLineCount = 0;
                uint8_t cbThreeLineCard[20];
                //开始判断
                while (cbLeftCardCount >= cbTurnCardCount && bFindThreeLine)
                {
                    uint8_t cbLastCard = cbTmpCard[cbFirstCard];
                    uint8_t cbSameCount = 1;
                    cbThreeLineCount = 0;
                    bFindThreeLine = false;
                    for (uint8_t i = cbFirstCard + 1; i < cbLeftCardCount + cbFirstCard; ++i)
                    {
                        //搜索同牌
                        while (GetCardValue(cbLastCard) == GetCardValue(cbTmpCard[i]) &&
                               i < cbLeftCardCount + cbFirstCard)
                        {
                            ++cbSameCount;
                            ++i;
                        }

                        uint8_t cbLastThreeCardValue;
                        if (cbThreeLineCount > 0)
                            cbLastThreeCardValue = GetCardLogicValue(cbThreeLineCard[cbThreeLineCount - 1]);

                        //重新开始
                        if ((cbSameCount < 3 ||
                             (cbThreeLineCount > 0 && (cbLastThreeCardValue - GetCardLogicValue(cbLastCard)) != 1))
                            &&
                            i <= cbLeftCardCount + cbFirstCard)
                        {
                            if (cbThreeLineCount >= cbTurnCardCount) break;

                            if (cbSameCount >= 3) i -= 3;
                            cbLastCard = cbTmpCard[i];
                            cbThreeLineCount = 0;
                        }
                            //保存数据
                        else if (cbSameCount >= 3)
                        {
                            cbThreeLineCard[cbThreeLineCount] = cbTmpCard[i - cbSameCount];
                            cbThreeLineCard[cbThreeLineCount + 1] = cbTmpCard[i - cbSameCount + 1];
                            cbThreeLineCard[cbThreeLineCount + 2] = cbTmpCard[i - cbSameCount + 2];
                            cbThreeLineCount += 3;

                            //结尾判断
                            if (i == (cbLeftCardCount + cbFirstCard - 3))
                                if ((GetCardLogicValue(cbLastCard) - GetCardLogicValue(cbTmpCard[i])) == 1 &&
                                    (GetCardLogicValue(cbTmpCard[i]) == GetCardLogicValue(cbTmpCard[i + 1])) &&
                                    (GetCardLogicValue(cbTmpCard[i]) == GetCardLogicValue(cbTmpCard[i + 2])))
                                {
                                    cbThreeLineCard[cbThreeLineCount] = cbTmpCard[i];
                                    cbThreeLineCard[cbThreeLineCount + 1] = cbTmpCard[i + 1];
                                    cbThreeLineCard[cbThreeLineCount + 2] = cbTmpCard[i + 2];
                                    cbThreeLineCount += 3;
                                    break;
                                }

                        }

                        cbLastCard = cbTmpCard[i];
                        cbSameCount = 1;
                    }

                    //保存数据
                    if (cbThreeLineCount >= cbTurnCardCount)
                    {
                        uint8_t Index;
                        uint8_t cbStart = 0;
                        //所有连牌
                        while (GetCardLogicValue(cbThreeLineCard[cbStart]) > GetCardLogicValue(cbTurnCardData[0]) &&
                               ((cbThreeLineCount - cbStart) >= cbTurnCardCount))
                        {
                            Index = CardTypeResult[CT_THREE_LINE].cbCardTypeCount;
                            CardTypeResult[CT_THREE_LINE].cbCardType = CT_THREE_LINE;
                            memcpy(CardTypeResult[CT_THREE_LINE].cbCardData[Index], cbThreeLineCard + cbStart,
                                   sizeof(uint8_t) * cbTurnCardCount);
                            CardTypeResult[CT_THREE_LINE].cbEachHandCardCount[Index] = cbTurnCardCount;
                            CardTypeResult[CT_THREE_LINE].cbCardTypeCount++;
                            cbStart += 3;

                            ASSERT(CardTypeResult[CT_THREE_LINE].cbCardTypeCount <= MAX_TYPE_COUNT);
                        }

                        RemoveCardList(cbThreeLineCard, cbThreeLineCount, cbTmpCard, cbFirstCard + cbLeftCardCount);
                        bFindThreeLine = true;
                        cbLeftCardCount -= cbThreeLineCount;
                    }
                }

                break;
            }
            case CT_THREE_TAKE_ONE://三带一单
            {
                uint8_t cbTurnThreeCard[MAX_LAND_COUNT];
                uint8_t cbTurnThreeCount = 0;
                uint8_t cbHandThreeCard[MAX_LAND_COUNT];
                uint8_t cbHandThreeCount = 0;
                uint8_t cbSingleCardCount = cbTurnCardCount / 4;

                //移除炸弹
                uint8_t cbAllBomCardData[MAX_LAND_COUNT];
                uint8_t cbAllBomCardCount = 0;
                GetAllBomCard(cbTmpCard, cbHandCardCount, cbAllBomCardData, cbAllBomCardCount);
                RemoveCardList(cbAllBomCardData, cbAllBomCardCount, cbTmpCard, cbHandCardCount);

                //三条扑克
                GetAllThreeCard(cbTurnCardData, cbTurnCardCount, cbTurnThreeCard, cbTurnThreeCount);

                uint8_t cbFirstCard = 0;

                //去除2和王
                if (cbTurnThreeCount > 3)
                    for (uint8_t i = 0; i < cbHandCardCount - cbAllBomCardCount; ++i)
                        if (GetCardLogicValue(cbTmpCard[i]) < 15)
                        {
                            cbFirstCard = i;
                            break;
                        }

                GetAllThreeCard(cbTmpCard + cbFirstCard, cbHandCardCount - cbFirstCard - cbAllBomCardCount,
                                cbHandThreeCard, cbHandThreeCount);

                if (cbHandThreeCount < cbTurnThreeCount || (cbHandThreeCount > 0 &&
                                                            GetCardLogicValue(cbHandThreeCard[0]) <
                                                            GetCardLogicValue(cbTurnThreeCard[0])))
                    return;

                for (uint8_t i = 0; i < cbHandThreeCount; i += 3)
                {
                    uint8_t cbLastLogicCard = GetCardLogicValue(cbHandThreeCard[i]);
                    uint8_t cbThreeLineCard[MAX_LAND_COUNT];
                    uint8_t cbThreeLineCardCount = 3;
                    cbThreeLineCard[0] = cbHandThreeCard[i];
                    cbThreeLineCard[1] = cbHandThreeCard[i + 1];
                    cbThreeLineCard[2] = cbHandThreeCard[i + 2];
                    for (uint8_t j = i + 3; j < cbHandThreeCount; j += 3)
                    {
                        //连续判断
                        if (1 != (cbLastLogicCard - (GetCardLogicValue(cbHandThreeCard[j]))) ||
                            cbThreeLineCardCount == cbTurnThreeCount)
                            break;

                        cbLastLogicCard = GetCardLogicValue(cbHandThreeCard[j]);
                        cbThreeLineCard[cbThreeLineCardCount] = cbHandThreeCard[j];
                        cbThreeLineCard[cbThreeLineCardCount + 1] = cbHandThreeCard[j + 1];
                        cbThreeLineCard[cbThreeLineCardCount + 2] = cbHandThreeCard[j + 2];
                        cbThreeLineCardCount += 3;
                    }
                    if (cbThreeLineCardCount == cbTurnThreeCount &&
                        GetCardLogicValue(cbThreeLineCard[0]) > GetCardLogicValue(cbTurnThreeCard[0]))
                    {
                        uint8_t Index;

                        uint8_t cbRemainCard[MAX_LAND_COUNT];
                        memcpy(cbRemainCard, cbTmpCard, (cbHandCardCount - cbAllBomCardCount) * sizeof(uint8_t));
                        RemoveCardList(cbThreeLineCard, cbTurnThreeCount, cbRemainCard,
                                       (cbHandCardCount - cbAllBomCardCount));

                        //单牌组合
                        uint8_t cbComCard[5];
                        uint8_t cbComResCard[254][5];
                        uint8_t cbComResLen = 0;
                        Combination(cbComCard, 0, cbComResCard, cbComResLen, cbRemainCard, cbSingleCardCount,
                                    (cbHandCardCount - cbAllBomCardCount) - cbTurnThreeCount, cbSingleCardCount);
                        for (uint8_t i = 0; i < cbComResLen; ++i)
                        {
                            Index = CardTypeResult[CT_THREE_TAKE_ONE].cbCardTypeCount;
                            CardTypeResult[CT_THREE_TAKE_ONE].cbCardType = CT_THREE_TAKE_ONE;
                            //保存三条
                            memcpy(CardTypeResult[CT_THREE_TAKE_ONE].cbCardData[Index], cbThreeLineCard,
                                   sizeof(uint8_t) * cbTurnThreeCount);
                            //保存单牌
                            memcpy(CardTypeResult[CT_THREE_TAKE_ONE].cbCardData[Index] + cbTurnThreeCount,
                                   cbComResCard[i], cbSingleCardCount);

                            ASSERT(cbTurnThreeCount + cbSingleCardCount == cbTurnCardCount);
                            CardTypeResult[CT_THREE_TAKE_ONE].cbEachHandCardCount[Index] = cbTurnCardCount;
                            CardTypeResult[CT_THREE_TAKE_ONE].cbCardTypeCount++;

                            ASSERT(CardTypeResult[CT_THREE_TAKE_ONE].cbCardTypeCount <= MAX_TYPE_COUNT);
                        }

                    }
                }

                break;
            }
            case CT_THREE_TAKE_TWO://三带一对
            {
                uint8_t cbTurnThreeCard[MAX_LAND_COUNT];
                uint8_t cbTurnThreeCount = 0;
                uint8_t cbHandThreeCard[MAX_LAND_COUNT];
                uint8_t cbHandThreeCount = 0;
                uint8_t cbDoubleCardCount = cbTurnCardCount / 5;

                //三条扑克
                GetAllThreeCard(cbTurnCardData, cbTurnCardCount, cbTurnThreeCard, cbTurnThreeCount);

                uint8_t cbFirstCard = 0;

                //去除2和王
                if (cbTurnThreeCount > 3)
                    for (uint8_t i = 0; i < cbHandCardCount; ++i)
                        if (GetCardLogicValue(cbTmpCard[i]) < 15)
                        {
                            cbFirstCard = i;
                            break;
                        }

                GetAllThreeCard(cbTmpCard + cbFirstCard, cbHandCardCount - cbFirstCard, cbHandThreeCard,
                                cbHandThreeCount);

                if (cbHandThreeCount < cbTurnThreeCount || (cbHandThreeCount > 0 &&
                                                            GetCardLogicValue(cbHandThreeCard[0]) <
                                                            GetCardLogicValue(cbTurnThreeCard[0])))
                    return;

                for (uint8_t i = 0; i < cbHandThreeCount; i += 3)
                {
                    uint8_t cbLastLogicCard = GetCardLogicValue(cbHandThreeCard[i]);
                    uint8_t cbThreeLineCard[MAX_LAND_COUNT];
                    uint8_t cbThreeLineCardCount = 3;
                    cbThreeLineCard[0] = cbHandThreeCard[i];
                    cbThreeLineCard[1] = cbHandThreeCard[i + 1];
                    cbThreeLineCard[2] = cbHandThreeCard[i + 2];
                    for (uint8_t j = i + 3; j < cbHandThreeCount; j += 3)
                    {
                        //连续判断
                        if (1 != (cbLastLogicCard - (GetCardLogicValue(cbHandThreeCard[j]))) ||
                            cbThreeLineCardCount == cbTurnThreeCount)
                            break;

                        cbLastLogicCard = GetCardLogicValue(cbHandThreeCard[j]);
                        cbThreeLineCard[cbThreeLineCardCount] = cbHandThreeCard[j];
                        cbThreeLineCard[cbThreeLineCardCount + 1] = cbHandThreeCard[j + 1];
                        cbThreeLineCard[cbThreeLineCardCount + 2] = cbHandThreeCard[j + 2];
                        cbThreeLineCardCount += 3;
                    }
                    if (cbThreeLineCardCount == cbTurnThreeCount &&
                        GetCardLogicValue(cbThreeLineCard[0]) > GetCardLogicValue(cbTurnThreeCard[0]))
                    {
                        uint8_t Index;

                        uint8_t cbRemainCard[MAX_LAND_COUNT];
                        memcpy(cbRemainCard, cbTmpCard, cbHandCardCount * sizeof(uint8_t));
                        RemoveCardList(cbThreeLineCard, cbTurnThreeCount, cbRemainCard, cbHandCardCount);

                        uint8_t cbAllDoubleCardData[MAX_LAND_COUNT];
                        uint8_t cbAllDoubleCardCount = 0;
                        GetAllDoubleCard(cbRemainCard, cbHandCardCount - cbTurnThreeCount, cbAllDoubleCardData,
                                         cbAllDoubleCardCount);

                        uint8_t cbDoubleCardIndex[10]; //对牌下标
                        for (uint8_t i = 0, j = 0; i < cbAllDoubleCardCount; i += 2, ++j)
                            cbDoubleCardIndex[j] = i;

                        //对牌组合
                        uint8_t cbComCard[5];
                        uint8_t cbComResCard[254][5];
                        uint8_t cbComResLen = 0;

                        //利用对牌的下标做组合，再根据下标提取出对牌
                        Combination(cbComCard, 0, cbComResCard, cbComResLen, cbDoubleCardIndex, cbDoubleCardCount,
                                    cbAllDoubleCardCount / 2, cbDoubleCardCount);
                        for (uint8_t i = 0; i < cbComResLen; ++i)
                        {
                            Index = CardTypeResult[CT_THREE_TAKE_TWO].cbCardTypeCount;
                            CardTypeResult[CT_THREE_TAKE_TWO].cbCardType = CT_THREE_TAKE_TWO;
                            //保存三条
                            memcpy(CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index], cbThreeLineCard,
                                   sizeof(uint8_t) * cbTurnThreeCount);
                            //保存对牌
                            for (uint8_t j = 0, k = 0; j < cbDoubleCardCount; ++j, k += 2)
                            {
                                CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index][cbTurnThreeCount +
                                                                                    k] = cbAllDoubleCardData[cbComResCard[i][j]];
                                CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index][cbTurnThreeCount + k +
                                                                                    1] = cbAllDoubleCardData[
                                        cbComResCard[i][j] + 1];
                            }

                            ASSERT(cbTurnThreeCount + cbDoubleCardCount * 2 == cbTurnCardCount);
                            CardTypeResult[CT_THREE_TAKE_TWO].cbEachHandCardCount[Index] = cbTurnCardCount;

                            CardTypeResult[CT_THREE_TAKE_TWO].cbCardTypeCount++;

                            ASSERT(CardTypeResult[CT_THREE_TAKE_TWO].cbCardTypeCount <= MAX_TYPE_COUNT);
                        }

                    }
                }

                break;
            }
            case CT_FOUR_TAKE_ONE://四带两单
            {
                uint8_t cbFirstCard = 0;
                //去除王牌
                for (uint8_t i = 0; i < cbHandCardCount; ++i)
                    if (GetCardColor(cbTmpCard[i]) != 0x40)
                    {
                        cbFirstCard = i;
                        break;
                    }

                uint8_t cbHandAllFourCardData[MAX_LAND_COUNT];
                uint8_t cbHandAllFourCardCount = 0;
                uint8_t cbTurnAllFourCardData[MAX_LAND_COUNT];
                uint8_t cbTurnAllFourCardCount = 0;
                //抽取四张
                GetAllBomCard(cbTmpCard + cbFirstCard, cbHandCardCount - cbFirstCard, cbHandAllFourCardData,
                              cbHandAllFourCardCount);
                GetAllBomCard(cbTurnCardData, cbTurnCardCount, cbTurnAllFourCardData, cbTurnAllFourCardCount);

                if (cbHandAllFourCardCount > 0 &&
                    GetCardLogicValue(cbHandAllFourCardData[0]) < GetCardLogicValue(cbTurnAllFourCardData[0]))
                    return;

                uint8_t cbCanOutFourCardData[MAX_LAND_COUNT];
                uint8_t cbCanOutFourCardCount = 0;
                //可出的牌
                for (uint8_t i = 0; i < cbHandAllFourCardCount; i += 4)
                {
                    if (GetCardLogicValue(cbHandAllFourCardData[i]) > GetCardLogicValue(cbTurnAllFourCardData[0]))
                    {
                        cbCanOutFourCardData[cbCanOutFourCardCount] = cbHandAllFourCardData[i];
                        cbCanOutFourCardData[cbCanOutFourCardCount + 1] = cbHandAllFourCardData[i + 1];
                        cbCanOutFourCardData[cbCanOutFourCardCount + 2] = cbHandAllFourCardData[i + 2];
                        cbCanOutFourCardData[cbCanOutFourCardCount + 3] = cbHandAllFourCardData[i + 3];
                        cbCanOutFourCardCount += 4;
                    }
                }

                if ((cbHandCardCount - cbCanOutFourCardCount) < (cbTurnCardCount - cbTurnAllFourCardCount)) return;

                uint8_t cbRemainCard[MAX_LAND_COUNT];
                memcpy(cbRemainCard, cbTmpCard, cbHandCardCount * sizeof(uint8_t));
                RemoveCardList(cbCanOutFourCardData, cbCanOutFourCardCount, cbRemainCard, cbHandCardCount);
                for (uint8_t Start = 0; Start < cbCanOutFourCardCount; Start += 4)
                {
                    uint8_t Index;
                    //单牌组合
                    uint8_t cbComCard[5];
                    uint8_t cbComResCard[254][5];
                    uint8_t cbComResLen = 0;
                    //单牌组合
                    Combination(cbComCard, 0, cbComResCard, cbComResLen, cbRemainCard, 2,
                                cbHandCardCount - cbCanOutFourCardCount, 2);
                    for (uint8_t i = 0; i < cbComResLen; ++i)
                    {
                        //不能带对
                        if (GetCardValue(cbComResCard[i][0]) == GetCardValue(cbComResCard[i][1])) continue;

                        Index = CardTypeResult[CT_FOUR_TAKE_ONE].cbCardTypeCount;
                        CardTypeResult[CT_FOUR_TAKE_ONE].cbCardType = CT_FOUR_TAKE_ONE;
                        memcpy(CardTypeResult[CT_FOUR_TAKE_ONE].cbCardData[Index], cbCanOutFourCardData + Start, 4);
                        CardTypeResult[CT_FOUR_TAKE_ONE].cbCardData[Index][4] = cbComResCard[i][0];
                        CardTypeResult[CT_FOUR_TAKE_ONE].cbCardData[Index][4 + 1] = cbComResCard[i][1];
                        CardTypeResult[CT_FOUR_TAKE_ONE].cbEachHandCardCount[Index] = 6;
                        CardTypeResult[CT_FOUR_TAKE_ONE].cbCardTypeCount++;

                        ASSERT(CardTypeResult[CT_FOUR_TAKE_ONE].cbCardTypeCount <= MAX_TYPE_COUNT);
                    }
                }

                break;
            }
            case CT_FOUR_TAKE_TWO://四带两对
            {
                uint8_t cbFirstCard = 0;
                //去除王牌
                for (uint8_t i = 0; i < cbHandCardCount; ++i)
                    if (GetCardColor(cbTmpCard[i]) != 0x40)
                    {
                        cbFirstCard = i;
                        break;
                    }

                uint8_t cbHandAllFourCardData[MAX_LAND_COUNT];
                uint8_t cbHandAllFourCardCount = 0;
                uint8_t cbTurnAllFourCardData[MAX_LAND_COUNT];
                uint8_t cbTurnAllFourCardCount = 0;
                //抽取四张
                GetAllBomCard(cbTmpCard + cbFirstCard, cbHandCardCount - cbFirstCard, cbHandAllFourCardData,
                              cbHandAllFourCardCount);
                GetAllBomCard(cbTurnCardData, cbTurnCardCount, cbTurnAllFourCardData, cbTurnAllFourCardCount);

                if (cbHandAllFourCardCount > 0 &&
                    GetCardLogicValue(cbHandAllFourCardData[0]) < GetCardLogicValue(cbTurnAllFourCardData[0]))
                    return;

                uint8_t cbCanOutFourCardData[MAX_LAND_COUNT];
                uint8_t cbCanOutFourCardCount = 0;
                //可出的牌
                for (uint8_t i = 0; i < cbHandAllFourCardCount; i += 4)
                {
                    if (GetCardLogicValue(cbHandAllFourCardData[i]) > GetCardLogicValue(cbTurnAllFourCardData[0]))
                    {
                        cbCanOutFourCardData[cbCanOutFourCardCount] = cbHandAllFourCardData[i];
                        cbCanOutFourCardData[cbCanOutFourCardCount + 1] = cbHandAllFourCardData[i + 1];
                        cbCanOutFourCardData[cbCanOutFourCardCount + 2] = cbHandAllFourCardData[i + 2];
                        cbCanOutFourCardData[cbCanOutFourCardCount + 3] = cbHandAllFourCardData[i + 3];
                        cbCanOutFourCardCount += 4;
                    }
                }

                if ((cbHandCardCount - cbCanOutFourCardCount) < (cbTurnCardCount - cbTurnAllFourCardCount)) return;

                uint8_t cbRemainCard[MAX_LAND_COUNT];
                memcpy(cbRemainCard, cbTmpCard, cbHandCardCount * sizeof(uint8_t));
                RemoveCardList(cbCanOutFourCardData, cbCanOutFourCardCount, cbRemainCard, cbHandCardCount);
                for (uint8_t Start = 0; Start < cbCanOutFourCardCount; Start += 4)
                {
                    uint8_t cbAllDoubleCardData[MAX_LAND_COUNT];
                    uint8_t cbAllDoubleCardCount = 0;
                    GetAllDoubleCard(cbRemainCard, cbHandCardCount - cbCanOutFourCardCount, cbAllDoubleCardData,
                                     cbAllDoubleCardCount);

                    uint8_t cbDoubleCardIndex[10]; //对牌下标
                    for (uint8_t i = 0, j = 0; i < cbAllDoubleCardCount; i += 2, ++j)
                        cbDoubleCardIndex[j] = i;

                    //对牌组合
                    uint8_t cbComCard[5];
                    uint8_t cbComResCard[254][5];
                    uint8_t cbComResLen = 0;

                    //利用对牌的下标做组合，再根据下标提取出对牌
                    Combination(cbComCard, 0, cbComResCard, cbComResLen, cbDoubleCardIndex, 2, cbAllDoubleCardCount / 2,
                                2);
                    for (uint8_t i = 0; i < cbComResLen; ++i)
                    {
                        uint8_t Index = CardTypeResult[CT_FOUR_TAKE_TWO].cbCardTypeCount;
                        CardTypeResult[CT_FOUR_TAKE_TWO].cbCardType = CT_FOUR_TAKE_TWO;
                        memcpy(CardTypeResult[CT_FOUR_TAKE_TWO].cbCardData[Index], cbCanOutFourCardData + Start, 4);

                        //保存对牌
                        for (uint8_t j = 0, k = 0; j < 4; ++j, k += 2)
                        {
                            CardTypeResult[CT_FOUR_TAKE_TWO].cbCardData[Index][4 +
                                                                               k] = cbAllDoubleCardData[cbComResCard[i][j]];
                            CardTypeResult[CT_FOUR_TAKE_TWO].cbCardData[Index][4 + k + 1] = cbAllDoubleCardData[
                                    cbComResCard[i][j] + 1];
                        }

                        CardTypeResult[CT_FOUR_TAKE_TWO].cbEachHandCardCount[Index] = 8;
                        CardTypeResult[CT_FOUR_TAKE_TWO].cbCardTypeCount++;

                        ASSERT(CardTypeResult[CT_FOUR_TAKE_TWO].cbCardTypeCount <= MAX_TYPE_COUNT);
                    }
                }
                break;
            }
            case CT_BOMB_CARD:            //炸弹类型
            {
                uint8_t cbAllBomCardData[MAX_LAND_COUNT];
                uint8_t cbAllBomCardCount = 0;
                GetAllBomCard(cbTmpCard, cbHandCardCount, cbAllBomCardData, cbAllBomCardCount);
                uint8_t cbFirstBom = 0;
                uint8_t Index;
                if (cbAllBomCardCount > 0 && cbAllBomCardData[0] == 0x4F)
                {
                    Index = CardTypeResult[CT_BOMB_CARD].cbCardTypeCount;
                    CardTypeResult[CT_BOMB_CARD].cbCardType = CT_BOMB_CARD;
                    CardTypeResult[CT_BOMB_CARD].cbCardData[Index][0] = 0x4F;
                    CardTypeResult[CT_BOMB_CARD].cbCardData[Index][1] = 0x4E;
                    CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] = 2;
                    CardTypeResult[CT_BOMB_CARD].cbCardTypeCount++;

                    ASSERT(CardTypeResult[CT_BOMB_CARD].cbCardTypeCount <= MAX_TYPE_COUNT);
                    cbFirstBom = 2;
                }
                for (uint8_t i = cbFirstBom; i < cbAllBomCardCount; i += 4)
                {
                    if (GetCardLogicValue(cbAllBomCardData[i]) > GetCardLogicValue(cbTurnCardData[0]))
                    {
                        Index = CardTypeResult[CT_BOMB_CARD].cbCardTypeCount;
                        CardTypeResult[CT_BOMB_CARD].cbCardType = CT_BOMB_CARD;
                        memcpy(CardTypeResult[CT_BOMB_CARD].cbCardData[Index], cbAllBomCardData + i, 4);
                        CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] = 4;
                        CardTypeResult[CT_BOMB_CARD].cbCardTypeCount++;

                        ASSERT(CardTypeResult[CT_BOMB_CARD].cbCardTypeCount <= MAX_TYPE_COUNT);
                    }
                }
                break;
            }
            case CT_MISSILE_CARD:        //火箭类型
            {
                //没有比火箭更大的牌了
                break;
            }
            default:
            {
                ASSERT(false);
                break;
            }
        }

    }

//单牌个数
    uint8_t CLandLogic::AnalyseSinleCardCount(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount,
                                              uint8_t const cbWantOutCardData[], uint8_t const cbWantOutCardCount,
                                              uint8_t cbSingleCardData[]) {
        uint8_t cbRemainCard[MAX_LAND_COUNT];
        uint8_t cbRemainCardCount = 0;
        memcpy(cbRemainCard, cbHandCardData, cbHandCardCount);
        SortCardList(cbRemainCard, cbHandCardCount, ST_ORDER);

        if (cbWantOutCardCount != 0)
            RemoveCardList(cbWantOutCardData, cbWantOutCardCount, cbRemainCard, cbHandCardCount);
        cbRemainCardCount = cbHandCardCount - cbWantOutCardCount;

        //函数指针
        typedef void (CLandLogic::*pGetAllCardFun)(uint8_t const [], uint8_t const, uint8_t[], uint8_t &);

        //指针数组
        pGetAllCardFun GetAllCardFunArray[4];
        GetAllCardFunArray[0] = (pGetAllCardFun) &CLandLogic::GetAllBomCard;    //炸弹函数
        GetAllCardFunArray[1] = (pGetAllCardFun) &CLandLogic::GetAllLineCard;    //顺子函数
        GetAllCardFunArray[2] = (pGetAllCardFun) &CLandLogic::GetAllThreeCard;    //三条函数
        GetAllCardFunArray[3] = (pGetAllCardFun) &CLandLogic::GetAllDoubleCard;    //对子函数

        //指针数组下标
        uint8_t cbIndexArray[4] = {0, 1, 2, 3};
        //排列结果
        uint8_t cbPermutationRes[24][4];
        uint8_t cbLen = 0;
        //计算排列
        Permutation(cbIndexArray, 0, 4, cbPermutationRes, cbLen);
        if (cbLen != 24)
        {
            LOG_ERROR("ASSERT(cbLen==24)");
            return MAX_LAND_COUNT;
        }

        //单牌数目
        uint8_t cbMinSingleCardCount = MAX_LAND_COUNT;
        //计算最小值
        for (uint8_t i = 0; i < 24; ++i)
        {
            //保留数据
            uint8_t cbTmpCardData[MAX_LAND_COUNT];
            uint8_t cbTmpCardCount = cbRemainCardCount;
            memcpy(cbTmpCardData, cbRemainCard, cbRemainCardCount);

            for (uint8_t j = 0; j < 4; ++j)
            {
                uint8_t Index = cbPermutationRes[i][j];

                //校验下标
                if (Index >= 4)
                {
                    LOG_ERROR("ASSERT(Index<4)");
                    return MAX_LAND_COUNT;
                }

                pGetAllCardFun pTmpGetAllCardFun = GetAllCardFunArray[Index];

                //提取扑克
                uint8_t cbGetCardData[MAX_LAND_COUNT];
                uint8_t cbGetCardCount = 0;
                //成员函数
                ((*this).*pTmpGetAllCardFun)(cbTmpCardData, cbTmpCardCount, cbGetCardData, cbGetCardCount);

                //删除扑克
                if (cbGetCardCount != 0)
                    RemoveCardList(cbGetCardData, cbGetCardCount, cbTmpCardData, cbTmpCardCount);
                cbTmpCardCount -= cbGetCardCount;
            }

            //计算单牌
            uint8_t cbSingleCard[MAX_LAND_COUNT];
            uint8_t cbSingleCardCount = 0;
            GetAllSingleCard(cbTmpCardData, cbTmpCardCount, cbSingleCard, cbSingleCardCount);
            cbMinSingleCardCount = cbMinSingleCardCount > cbSingleCardCount ? cbSingleCardCount : cbMinSingleCardCount;
        }

        return cbMinSingleCardCount;
    }

//地主出牌（先出牌）
    void
    CLandLogic::BankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, tagOutCardResult &OutCardResult) {
        //零下标没用
        tagOutCardTypeResult CardTypeResult[12 + 1];
        memset(CardTypeResult, 0, sizeof(CardTypeResult));

        //初始变量
        memset(&OutCardResult, 0, sizeof(OutCardResult));

        uint8_t cbLineCard[MAX_LAND_COUNT];
        uint8_t cbThreeLineCard[MAX_LAND_COUNT];
        uint8_t cbDoubleLineCard[MAX_LAND_COUNT];
        uint8_t cbLineCardCount;
        uint8_t cbThreeLineCardCount;
        uint8_t cbDoubleLineCount;
        GetAllLineCard(cbHandCardData, cbHandCardCount, cbLineCard, cbLineCardCount);
        GetAllThreeCard(cbHandCardData, cbHandCardCount, cbThreeLineCard, cbThreeLineCardCount);
        GetAllDoubleCard(cbHandCardData, cbHandCardCount, cbDoubleLineCard, cbDoubleLineCount);

        uint16_t wUndersideOfBanker = (m_wBankerUser + 1) % GAME_LAND_PLAYER;        //地主下家
        uint16_t wUpsideOfBanker = (wUndersideOfBanker + 1) % GAME_LAND_PLAYER;    //地主上家

        //如果只剩顺牌和单只，则先出顺
        {
            if (cbLineCardCount + 1 == cbHandCardCount && CT_SINGLE == GetCardType(cbLineCard, cbLineCardCount))
            {
                OutCardResult.cbCardCount = cbLineCardCount;
                memcpy(OutCardResult.cbResultCard, cbLineCard, cbLineCardCount);
            }
            else if (cbThreeLineCardCount + 1 == cbHandCardCount &&
                     CT_THREE_LINE == GetCardType(cbThreeLineCard, cbThreeLineCardCount))
            {
                OutCardResult.cbCardCount = cbThreeLineCardCount;
                memcpy(OutCardResult.cbResultCard, cbThreeLineCard, cbThreeLineCardCount);
            }
            else if (cbDoubleLineCount + 1 == cbHandCardCount &&
                     CT_DOUBLE_LINE == GetCardType(cbDoubleLineCard, cbDoubleLineCount))
            {
                OutCardResult.cbCardCount = cbDoubleLineCount;
                memcpy(OutCardResult.cbResultCard, cbDoubleLineCard, cbDoubleLineCount);
            }
                //双王炸弹和一手
            else if (cbHandCardCount > 2 && cbHandCardData[0] == 0x4f && cbHandCardData[1] == 0x4e &&
                     CT_ERROR != GetCardType(cbHandCardData + 2, cbHandCardCount - 2))
            {
                OutCardResult.cbCardCount = 2;
                OutCardResult.cbResultCard[0] = 0x4f;
                OutCardResult.cbResultCard[1] = 0x4e;
            }

            if (OutCardResult.cbCardCount > 0)
            {
                return;
            }
        }

        //对王加一只
        if (cbHandCardCount == 3 && GetCardColor(cbHandCardData[0]) == 0x40
            && GetCardColor(cbHandCardData[1]) == 0x40)
        {
            OutCardResult.cbCardCount = 2;
            OutCardResult.cbResultCard[0] = 0x4f;
            OutCardResult.cbResultCard[1] = 0x4e;
            return;
        }
            //对王
        else if (cbHandCardCount == 2 && GetCardColor(cbHandCardData[0]) == 0x40 &&
                 GetCardColor(cbHandCardData[1]) == 0x40)
        {
            OutCardResult.cbCardCount = 2;
            OutCardResult.cbResultCard[0] = 0x4f;
            OutCardResult.cbResultCard[1] = 0x4e;
            return;
        }
            //只剩一手牌
        else if (CT_ERROR != GetCardType(cbHandCardData, cbHandCardCount))
        {
            OutCardResult.cbCardCount = cbHandCardCount;
            memcpy(OutCardResult.cbResultCard, cbHandCardData, cbHandCardCount);
            return;
        }

        //只剩一张和一手
        if (cbHandCardCount >= 2)
        {
            //上家扑克
            tagOutCardTypeResult UpsideCanOutCardType1[13];
            memset(UpsideCanOutCardType1, 0, sizeof(UpsideCanOutCardType1));
            tagOutCardTypeResult UpsideCanOutCardType2[13];
            memset(UpsideCanOutCardType2, 0, sizeof(UpsideCanOutCardType2));

            //下家扑克
            tagOutCardTypeResult UndersideCanOutCardType1[13];
            memset(UndersideCanOutCardType1, 0, sizeof(UndersideCanOutCardType1));
            tagOutCardTypeResult UndersideCanOutCardType2[13];
            memset(UndersideCanOutCardType2, 0, sizeof(UndersideCanOutCardType2));

            uint8_t cbFirstHandCardType = GetCardType(cbHandCardData, cbHandCardCount - 1);
            uint8_t cbSecondHandCardType = GetCardType(cbHandCardData + 1, cbHandCardCount - 1);

            if (CT_ERROR != cbFirstHandCardType && cbFirstHandCardType != CT_FOUR_TAKE_ONE &&
                cbFirstHandCardType != CT_FOUR_TAKE_TWO)
            {
                AnalyseOutCardType(m_cbAllCardData[wUpsideOfBanker], m_cbUserCardCount[wUpsideOfBanker],
                                   cbHandCardData,
                                   cbHandCardCount - 1, UpsideCanOutCardType1);
                AnalyseOutCardType(m_cbAllCardData[wUndersideOfBanker], m_cbUserCardCount[wUndersideOfBanker],
                                   cbHandCardData, cbHandCardCount - 1, UndersideCanOutCardType1);
            }
            if (CT_ERROR != cbSecondHandCardType && cbSecondHandCardType != CT_FOUR_TAKE_ONE &&
                cbSecondHandCardType != CT_FOUR_TAKE_TWO)
            {
                AnalyseOutCardType(m_cbAllCardData[wUpsideOfBanker], m_cbUserCardCount[wUpsideOfBanker],
                                   cbHandCardData + 1, cbHandCardCount - 1, UpsideCanOutCardType2);
                AnalyseOutCardType(m_cbAllCardData[wUndersideOfBanker], m_cbUserCardCount[wUndersideOfBanker],
                                   cbHandCardData + 1, cbHandCardCount - 1, UndersideCanOutCardType2);
            }

            if (cbSecondHandCardType != CT_ERROR && cbSecondHandCardType != CT_FOUR_TAKE_ONE &&
                cbSecondHandCardType != CT_FOUR_TAKE_TWO &&
                UpsideCanOutCardType2[cbSecondHandCardType].cbCardTypeCount == 0 &&
                UndersideCanOutCardType2[cbSecondHandCardType].cbCardTypeCount == 0 &&
                UpsideCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount == 0 &&
                UndersideCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount == 0)
            {
                OutCardResult.cbCardCount = cbHandCardCount - 1;
                memcpy(OutCardResult.cbResultCard, cbHandCardData + 1, cbHandCardCount - 1);
                return;
            }

            if (cbFirstHandCardType != CT_ERROR && cbFirstHandCardType != CT_FOUR_TAKE_ONE &&
                cbFirstHandCardType != CT_FOUR_TAKE_TWO &&
                UpsideCanOutCardType1[cbFirstHandCardType].cbCardTypeCount == 0 &&
                UndersideCanOutCardType1[cbFirstHandCardType].cbCardTypeCount == 0 &&
                UpsideCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount == 0 &&
                UndersideCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount == 0)
            {
                OutCardResult.cbCardCount = cbHandCardCount - 1;
                memcpy(OutCardResult.cbResultCard, cbHandCardData, cbHandCardCount - 1);
                return;
            }

            if (GetCardLogicValue(cbHandCardData[0]) >= GetCardLogicValue(m_cbAllCardData[wUpsideOfBanker][0]) &&
                GetCardLogicValue(cbHandCardData[0])
                >= GetCardLogicValue(m_cbAllCardData[wUndersideOfBanker][0]) &&
                CT_ERROR != cbSecondHandCardType && cbSecondHandCardType != CT_FOUR_TAKE_ONE &&
                cbSecondHandCardType != CT_FOUR_TAKE_TWO &&
                UpsideCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount == 0 &&
                UndersideCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount == 0)
            {
                OutCardResult.cbCardCount = 1;
                OutCardResult.cbResultCard[0] = cbHandCardData[0];
                return;
            }

            if (CT_ERROR != cbSecondHandCardType && cbSecondHandCardType != CT_FOUR_TAKE_ONE &&
                cbSecondHandCardType != CT_FOUR_TAKE_TWO &&
                UpsideCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount == 0 &&
                UndersideCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount == 0)
            {
                OutCardResult.cbCardCount = cbHandCardCount - 1;
                memcpy(OutCardResult.cbResultCard, cbHandCardData + 1, cbHandCardCount - 1);
                return;
            }
        }

        {
            {
                //分析扑克
                tagOutCardTypeResult MeOutCardTypeResult[13];
                memset(MeOutCardTypeResult, 0, sizeof(MeOutCardTypeResult));
                AnalyseOutCardType(cbHandCardData, cbHandCardCount, MeOutCardTypeResult);

                //计算单牌
                uint8_t cbMinSingleCardCount[4];
                cbMinSingleCardCount[0] = MAX_LAND_COUNT;
                cbMinSingleCardCount[1] = MAX_LAND_COUNT;
                cbMinSingleCardCount[2] = MAX_LAND_COUNT;
                cbMinSingleCardCount[3] = MAX_LAND_COUNT;
                uint8_t cbIndex[4] = {0};
                uint8_t cbOutcardType[4] = {CT_ERROR};
                uint8_t cbMinValue = MAX_LAND_COUNT;
                uint8_t cbMinSingleCountInFour = MAX_LAND_COUNT;
                uint8_t cbMinCardType = CT_ERROR;
                uint8_t cbMinIndex = 0;

                //除炸弹外的牌
                for (uint8_t cbCardType = CT_DOUBLE; cbCardType < CT_BOMB_CARD; ++cbCardType)
                {

                    tagOutCardTypeResult const &tmpCardResult = MeOutCardTypeResult[cbCardType];

                    //相同牌型，相同长度，单连，对连等相同牌型可能长度不一样
                    uint8_t cbThisHandCardCount = MAX_LAND_COUNT;

                    //上家扑克
                    tagOutCardTypeResult UpsideOutCardTypeResult[13];
                    memset(UpsideOutCardTypeResult, 0, sizeof(UpsideOutCardTypeResult));

                    //下家扑克
                    tagOutCardTypeResult UndersideOutCardTypeResult[13];
                    memset(UndersideOutCardTypeResult, 0, sizeof(UndersideOutCardTypeResult));

                    for (uint8_t i = 0; i < tmpCardResult.cbCardTypeCount; ++i)
                    {
                        uint8_t cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,
                                                                   tmpCardResult.cbCardData[i],
                                                                   tmpCardResult.cbEachHandCardCount[i]);

                        //重新分析
                        if (tmpCardResult.cbEachHandCardCount[i] != cbThisHandCardCount)
                        {
                            cbThisHandCardCount = tmpCardResult.cbEachHandCardCount[i];
                            AnalyseOutCardType(m_cbAllCardData[wUpsideOfBanker], m_cbUserCardCount[wUpsideOfBanker],
                                               tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i],
                                               UpsideOutCardTypeResult);
                            AnalyseOutCardType(m_cbAllCardData[wUndersideOfBanker],
                                               m_cbUserCardCount[wUndersideOfBanker],
                                               tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i],
                                               UndersideOutCardTypeResult);
                        }
                        uint8_t cbMaxValue = 0;
                        uint8_t Index = 0;

                        //敌方可以压住牌
                        if (UpsideOutCardTypeResult[cbCardType].cbCardTypeCount > 0 ||
                            UndersideOutCardTypeResult[cbCardType].cbCardTypeCount > 0)
                        {
                            continue;
                        }
                        //是否有大牌
                        if (tmpCardResult.cbEachHandCardCount[i] != cbHandCardCount)
                        {
                            bool bHaveLargeCard = false;
                            for (uint8_t j = 0; j < tmpCardResult.cbEachHandCardCount[i]; ++j)
                            {
                                if (GetCardLogicValue(tmpCardResult.cbCardData[i][j]) >= 15) bHaveLargeCard = true;
                                if (cbCardType != CT_SINGLE_LINE && cbCardType != CT_DOUBLE_LINE &&
                                    GetCardLogicValue(tmpCardResult.cbCardData[i][0]) == 14)
                                    bHaveLargeCard = true;
                            }

                            if (bHaveLargeCard)
                                continue;
                        }

                        //搜索cbMinSingleCardCount[4]的最大值
                        for (uint8_t j = 0; j < 4; ++j)
                        {
                            if (cbMinSingleCardCount[j] >= cbTmpCount)
                            {
                                cbMinSingleCardCount[j] = cbTmpCount;
                                cbIndex[j] = i;
                                cbOutcardType[j] = cbCardType;
                                break;
                            }
                        }

                        //保存最小值
                        if (cbMinSingleCountInFour >= cbTmpCount)
                        {
                            //最小牌型
                            cbMinCardType = cbCardType;
                            //最小牌型中的最小单牌
                            cbMinSingleCountInFour = cbTmpCount;
                            //最小牌型中的最小牌
                            cbMinIndex = i;
                        }
                    }
                }

                if (cbMinSingleCountInFour >= AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, NULL, 0) + 3 &&
                    (m_cbUserCardCount[wUndersideOfBanker] >= 4 && m_cbUserCardCount[wUpsideOfBanker] >= 4))
                    cbMinSingleCountInFour = MAX_LAND_COUNT;

                if (cbMinSingleCountInFour != MAX_LAND_COUNT)
                {
                    uint8_t Index = cbMinIndex;

                    //选择最小牌
                    for (uint8_t i = 0; i < 4; ++i)
                    {
                        if (cbOutcardType[i] == cbMinCardType && cbMinSingleCardCount[i] <= cbMinSingleCountInFour &&
                            GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0]) <
                            GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[Index][0]))
                            Index = cbIndex[i];
                    }

                    //对王加一只
                    if (cbHandCardCount == 3 && GetCardColor(cbHandCardData[0]) == 0x40 &&
                        GetCardColor(cbHandCardData[1]) == 0x40)
                    {
                        OutCardResult.cbCardCount = 2;
                        OutCardResult.cbResultCard[0] = 0x4f;
                        OutCardResult.cbResultCard[1] = 0x4e;
                        return;
                    }
                        //对王
                    else if (cbHandCardCount == 2 && GetCardColor(cbHandCardData[0]) == 0x40 &&
                             GetCardColor(cbHandCardData[1]) == 0x40)
                    {
                        OutCardResult.cbCardCount = 2;
                        OutCardResult.cbResultCard[0] = 0x4f;
                        OutCardResult.cbResultCard[1] = 0x4e;
                        return;
                    }
                    else
                    {
                        //设置变量
                        OutCardResult.cbCardCount = MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
                        memcpy(OutCardResult.cbResultCard, MeOutCardTypeResult[cbMinCardType].cbCardData[Index],
                               MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index] * sizeof(uint8_t));
                        return;
                    }

                    ASSERT(OutCardResult.cbCardCount > 0);

                    return;
                }

                //如果地主扑克少于5，还没有找到适合的牌则从大出到小
                if (OutCardResult.cbCardCount <= 0 &&
                    (m_cbUserCardCount[wUndersideOfBanker] >= 4 || m_cbUserCardCount[wUpsideOfBanker] >= 4))
                {
                    //只有一张牌时不能放地主走
                    if (m_cbUserCardCount[m_wBankerUser] == 1 && MeOutCardTypeResult[CT_SINGLE].cbCardTypeCount > 0)
                    {
                        uint8_t Index = MAX_LAND_COUNT;
                        for (uint8_t i = 0; i < MeOutCardTypeResult[CT_SINGLE].cbCardTypeCount; ++i)
                        {
                            if (GetCardLogicValue(MeOutCardTypeResult[CT_SINGLE].cbCardData[i][0]) >=
                                GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]))
                            {
                                Index = i;
                            }
                            else break;
                        }

                        if (MAX_LAND_COUNT != Index)
                        {
                            OutCardResult.cbCardCount = MeOutCardTypeResult[CT_SINGLE].cbEachHandCardCount[Index];
                            memcpy(OutCardResult.cbResultCard, MeOutCardTypeResult[CT_SINGLE].cbCardData[Index],
                                   OutCardResult.cbCardCount);
                            return;
                        }
                    }
                }
            }
        }
        uint8_t cbFirstCard = 0;
        //过滤王和2
        for (uint8_t i = 0; i < cbHandCardCount; ++i)
            if (GetCardLogicValue(cbHandCardData[i]) < 15)
            {
                cbFirstCard = i;
                break;
            }

        if (cbFirstCard < cbHandCardCount - 1)
            AnalyseOutCardType(cbHandCardData + cbFirstCard, cbHandCardCount - cbFirstCard, CardTypeResult);
        else
            AnalyseOutCardType(cbHandCardData, cbHandCardCount, CardTypeResult);

        //计算单牌
        uint8_t cbMinSingleCardCount[4];
        cbMinSingleCardCount[0] = MAX_LAND_COUNT;
        cbMinSingleCardCount[1] = MAX_LAND_COUNT;
        cbMinSingleCardCount[2] = MAX_LAND_COUNT;
        cbMinSingleCardCount[3] = MAX_LAND_COUNT;
        uint8_t cbIndex[4] = {0};
        uint8_t cbOutcardType[4] = {CT_ERROR};
        uint8_t cbMinValue = MAX_LAND_COUNT;
        uint8_t cbMinSingleCountInFour = MAX_LAND_COUNT;
        uint8_t cbMinCardType = CT_ERROR;
        uint8_t cbMinIndex = 0;

        //除炸弹外的牌
        for (uint8_t cbCardType = CT_SINGLE; cbCardType < CT_BOMB_CARD; ++cbCardType)
        {
            tagOutCardTypeResult const &tmpCardResult = CardTypeResult[cbCardType];
            for (uint8_t i = 0; i < tmpCardResult.cbCardTypeCount; ++i)
            {
                uint8_t cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, tmpCardResult.cbCardData[i],
                                                           tmpCardResult.cbEachHandCardCount[i]);

                uint8_t cbMaxValue = 0;
                uint8_t Index = 0;

                //搜索cbMinSingleCardCount[4]的最大值
                for (uint8_t j = 0; j < 4; ++j)
                {
                    if (cbMinSingleCardCount[j] >= cbTmpCount)
                    {
                        cbMinSingleCardCount[j] = cbTmpCount;
                        cbIndex[j] = i;
                        cbOutcardType[j] = cbCardType;
                        break;
                    }
                }

                //保存最小值
                if (cbMinSingleCountInFour >= cbTmpCount)
                {
                    //最小牌型
                    cbMinCardType = cbCardType;
                    //最小牌型中的最小单牌
                    cbMinSingleCountInFour = cbTmpCount;
                    //最小牌型中的最小牌
                    cbMinIndex = i;
                }
            }
        }

        if (cbMinSingleCountInFour != MAX_LAND_COUNT)
        {
            uint8_t Index = cbMinIndex;

            //选择最小牌
            for (uint8_t i = 0; i < 4; ++i)
            {
                if (cbOutcardType[i] == cbMinCardType && cbMinSingleCardCount[i] <= cbMinSingleCountInFour &&
                    GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0]) <
                    GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[Index][0]))
                    Index = cbIndex[i];
            }

            //对王加一只
            if (cbHandCardCount == 3 && GetCardColor(cbHandCardData[0]) == 0x40 &&
                GetCardColor(cbHandCardData[1]) == 0x40)
            {
                OutCardResult.cbCardCount = 2;
                OutCardResult.cbResultCard[0] = 0x4f;
                OutCardResult.cbResultCard[1] = 0x4e;
                return;
            }
                //对王
            else if (cbHandCardCount == 2 && GetCardColor(cbHandCardData[0]) == 0x40 &&
                     GetCardColor(cbHandCardData[1]) == 0x40)
            {
                OutCardResult.cbCardCount = 2;
                OutCardResult.cbResultCard[0] = 0x4f;
                OutCardResult.cbResultCard[1] = 0x4e;
                return;
            }
            else
            {
                //设置变量
                OutCardResult.cbCardCount = CardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
                memcpy(OutCardResult.cbResultCard, CardTypeResult[cbMinCardType].cbCardData[Index],
                       CardTypeResult[cbMinCardType].cbEachHandCardCount[Index] * sizeof(uint8_t));
                return;
            }

            ASSERT(OutCardResult.cbCardCount > 0);

            return;
        }
            //如果只剩炸弹
        else
        {
            uint8_t Index = 0;
            uint8_t cbLogicCardValue = GetCardLogicValue(0x4F) + 1;
            //最小炸弹
            for (uint8_t i = 0; i < CardTypeResult[CT_BOMB_CARD].cbCardTypeCount; ++i)
                if (cbLogicCardValue > GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]))
                {
                    cbLogicCardValue = GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]);
                    Index = i;
                }

            //设置变量
            OutCardResult.cbCardCount = CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index];
            memcpy(OutCardResult.cbResultCard, CardTypeResult[CT_BOMB_CARD].cbCardData[Index],
                   CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] * sizeof(uint8_t));

            return;
        }

        //如果都没有搜索到就出最小的一张
        OutCardResult.cbCardCount = 1;
        OutCardResult.cbResultCard[0] = cbHandCardData[cbHandCardCount - 1];

        return;
    }

//地主出牌（后出牌）
    void CLandLogic::BankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint16_t wOutCardUser,
                                   const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount,
                                   tagOutCardResult &OutCardResult) {
        //初始变量
        memset(&OutCardResult, 0, sizeof(OutCardResult));

        //零下标没用
        tagOutCardTypeResult CardTypeResult[12 + 1];
        memset(CardTypeResult, 0, sizeof(CardTypeResult));

        //出牌类型
        uint8_t cbOutCardType = GetCardType(cbTurnCardData, cbTurnCardCount);
        AnalyseOutCardType(cbHandCardData, cbHandCardCount, cbTurnCardData, cbTurnCardCount, CardTypeResult);

        uint16_t wUndersideUser = (m_wBankerUser + 1) % GAME_LAND_PLAYER;
        uint16_t wUpsideUser = (wUndersideUser + 1) % GAME_LAND_PLAYER;

        //只剩炸弹
        if (cbHandCardCount == CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0])
        {
            OutCardResult.cbCardCount = CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0];
            memcpy(OutCardResult.cbResultCard, CardTypeResult[CT_BOMB_CARD].cbCardData, OutCardResult.cbCardCount);

            return;
        }
            //双王炸弹和一手
        else if (cbHandCardCount > 2 && cbHandCardData[0] == 0x4f && cbHandCardData[1] == 0x4e &&
                 CT_ERROR != GetCardType(cbHandCardData + 2, cbHandCardCount - 2))
        {
            OutCardResult.cbCardCount = 2;
            OutCardResult.cbResultCard[0] = 0x4f;
            OutCardResult.cbResultCard[1] = 0x4e;
            return;
        }

        //取出四个最小单牌
        uint8_t cbMinSingleCardCount[4];
        cbMinSingleCardCount[0] = MAX_LAND_COUNT;
        cbMinSingleCardCount[1] = MAX_LAND_COUNT;
        cbMinSingleCardCount[2] = MAX_LAND_COUNT;
        cbMinSingleCardCount[3] = MAX_LAND_COUNT;
        uint8_t cbIndex[4] = {0};
        uint8_t cbMinSingleCountInFour = MAX_LAND_COUNT;

        //可出扑克（这里已经过滤掉炸弹了）
        tagOutCardTypeResult const &CanOutCard = CardTypeResult[cbOutCardType];

        for (uint8_t i = 0; i < CanOutCard.cbCardTypeCount; ++i)
        {
            //最小单牌
            uint8_t cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, CanOutCard.cbCardData[i],
                                                       CanOutCard.cbEachHandCardCount[i]);
            uint8_t cbMaxValue = 0;
            uint8_t Index = 0;

            //搜索cbMinSingleCardCount[4]的最大值
            for (uint8_t j = 0; j < 4; ++j)
            {
                if (cbMinSingleCardCount[j] >= cbTmpCount)
                {
                    cbMinSingleCardCount[j] = cbTmpCount;
                    cbIndex[j] = i;
                    break;
                }
            }

        }

        for (uint8_t i = 0; i < 4; ++i)
            if (cbMinSingleCountInFour > cbMinSingleCardCount[i]) cbMinSingleCountInFour = cbMinSingleCardCount[i];


        //原始单牌数
        uint8_t cbOriginSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, NULL, 0);

        if (CanOutCard.cbCardTypeCount > 0)
        {
            uint8_t cbMinLogicCardValue = GetCardLogicValue(0x4F) + 1;
            bool bFindCard = false;
            uint8_t cbCanOutIndex = 0;
            for (uint8_t i = 0; i < 4; ++i)
            {
                uint8_t Index = cbIndex[i];

                if ((cbMinSingleCardCount[i] < cbOriginSingleCardCount + 3) &&
                    cbMinSingleCardCount[i] <= cbMinSingleCountInFour &&
                    cbMinLogicCardValue > GetCardLogicValue(CanOutCard.cbCardData[Index][0]))
                {
                    //针对大牌
                    bool bNoLargeCard = true;

                    //当出牌玩家手上牌数大于4，而且出的是小于K的牌而且不是出牌手上的最大牌时，不能出2去打
                    if (m_cbUserCardCount[wOutCardUser] >= 4 && cbHandCardCount >= 5 &&
                        CanOutCard.cbEachHandCardCount[Index] >= 2 &&
                        GetCardLogicValue(CanOutCard.cbCardData[Index][0]) >= 15 &&
                        GetCardLogicValue(cbTurnCardData[0]) < 13 &&
                        ((wOutCardUser == wUndersideUser && GetCardLogicValue(cbTurnCardData[0]) <
                                                            GetCardLogicValue(m_cbAllCardData[wUndersideUser][0])) ||
                         (wOutCardUser == wUpsideUser &&
                          GetCardLogicValue(cbTurnCardData[0])
                          < GetCardLogicValue(m_cbAllCardData[wUpsideUser][0])))
                        && CanOutCard.cbEachHandCardCount[Index] != cbHandCardCount)
                        bNoLargeCard = false;

                    //搜索有没有大牌（针对飞机带翅膀后面的带牌）
                    for (uint8_t k = 3; k < CanOutCard.cbEachHandCardCount[Index]; ++k)
                    {
                        if (GetCardLogicValue(CanOutCard.cbCardData[Index][k]) >= 15 &&
                            CanOutCard.cbEachHandCardCount[Index] != cbHandCardCount)
                            bNoLargeCard = false;
                    }
                    if (bNoLargeCard)
                    {
                        bFindCard = true;
                        cbCanOutIndex = Index;
                        cbMinLogicCardValue = GetCardLogicValue(CanOutCard.cbCardData[Index][0]);
                    }
                }
            }

            if (bFindCard)
            {
                //最大牌
                uint8_t cbLargestLogicCard;
                if (wOutCardUser == wUndersideUser)
                    cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[wUndersideUser][0]);
                else if (wOutCardUser == wUpsideUser)
                    cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[wUpsideUser][0]);
                bool bCanOut = true;

                //王只压2
                if (GetCardLogicValue(cbTurnCardData[0]) < cbLargestLogicCard)
                {
                    if (GetCardColor(CanOutCard.cbCardData[cbCanOutIndex][0]) == 0x40 &&
                        GetCardLogicValue(cbTurnCardData[0]) <= 14 && cbHandCardCount > 5)
                    {
                        bCanOut = false;
                    }
                }

                if (bCanOut)
                {
                    //设置变量
                    OutCardResult.cbCardCount = CanOutCard.cbEachHandCardCount[cbCanOutIndex];
                    memcpy(OutCardResult.cbResultCard, CanOutCard.cbCardData[cbCanOutIndex],
                           CanOutCard.cbEachHandCardCount[cbCanOutIndex] * sizeof(uint8_t));

                    return;
                }
            }

            if (cbOutCardType == CT_SINGLE)
            {
                //闲家的最大牌
                uint8_t cbLargestLogicCard;
                if (wOutCardUser == wUndersideUser)
                    cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[wUndersideUser][0]);
                else if (wOutCardUser == wUpsideUser)
                    cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[wUpsideUser][0]);

                if (GetCardLogicValue(cbTurnCardData[0]) == 14 ||
                    GetCardLogicValue(cbTurnCardData[0]) >= cbLargestLogicCard ||
                    (GetCardLogicValue(cbTurnCardData[0]) < cbLargestLogicCard - 1) ||
                    ((wOutCardUser == wUndersideUser && m_cbUserCardCount[wUndersideUser] <= 5) ||
                     (wOutCardUser == wUpsideUser && m_cbUserCardCount[wUpsideUser] <= 5)))
                {
                    //取一张大于等于2而且要比闲家出的牌大的牌，
                    uint8_t cbIndex = MAX_LAND_COUNT;
                    for (uint8_t i = 0; i < cbHandCardCount; ++i)
                        if (GetCardLogicValue(cbHandCardData[i]) > GetCardLogicValue(cbTurnCardData[0]) &&
                            GetCardLogicValue(cbHandCardData[i]) >= 15)
                        {
                            cbIndex = i;
                        }
                    if (cbIndex != MAX_LAND_COUNT)
                    {
                        //设置变量
                        OutCardResult.cbCardCount = 1;
                        OutCardResult.cbResultCard[0] = cbHandCardData[cbIndex];

                        return;
                    }
                }
            }

            uint8_t cbMinSingleCount = MAX_LAND_COUNT;
            uint8_t Index = 0;
            for (uint8_t i = 0; i < CardTypeResult[cbOutCardType].cbCardTypeCount; ++i)
            {
                uint8_t cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,
                                                           CardTypeResult[cbOutCardType].cbCardData[i],
                                                           CardTypeResult[cbOutCardType].cbEachHandCardCount[i]);
                if (cbMinSingleCount >= cbTmpCount)
                {
                    cbMinSingleCount = cbTmpCount;
                    Index = i;
                }
            }
            //设置变量
            OutCardResult.cbCardCount = CardTypeResult[cbOutCardType].cbEachHandCardCount[Index];
            memcpy(OutCardResult.cbResultCard, CardTypeResult[cbOutCardType].cbCardData[Index],
                   OutCardResult.cbCardCount);

            return;
        }

        //还要考虑炸弹
        if (CardTypeResult[CT_BOMB_CARD].cbCardTypeCount > 0 && cbHandCardCount <= 10)
        {
            tagOutCardTypeResult const &BomCard = CardTypeResult[CT_BOMB_CARD];
            uint8_t cbMinLogicValue = GetCardLogicValue(BomCard.cbCardData[0][0]);
            uint8_t Index = 0;
            for (uint8_t i = 0; i < BomCard.cbCardTypeCount; ++i)
            {
                if (cbMinLogicValue > GetCardLogicValue(BomCard.cbCardData[i][0]))
                {
                    cbMinLogicValue = GetCardLogicValue(BomCard.cbCardData[i][0]);
                    Index = i;
                }
            }

            //判断出了炸弹后的单牌数
            uint8_t cbSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, BomCard.cbCardData[Index],
                                                              BomCard.cbEachHandCardCount[Index]);
            if (cbSingleCardCount >= 3 ||
                (cbOutCardType == CT_SINGLE && GetCardLogicValue(cbTurnCardData[0]) < 15))
                return;

            //设置变量
            OutCardResult.cbCardCount = BomCard.cbEachHandCardCount[Index];
            memcpy(OutCardResult.cbResultCard, BomCard.cbCardData[Index],
                   BomCard.cbEachHandCardCount[Index] * sizeof(uint8_t));

            return;
        }

        return;
    }

//地主上家（先出牌）
    void CLandLogic::UpsideOfBankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint16_t wMeChairID,
                                           tagOutCardResult &OutCardResult) {
        //零下标没用
        tagOutCardTypeResult CardTypeResult[12 + 1];
        memset(CardTypeResult, 0, sizeof(CardTypeResult));

        //初始变量
        memset(&OutCardResult, 0, sizeof(OutCardResult));

        uint8_t cbLineCard[MAX_LAND_COUNT];
        uint8_t cbThreeLineCard[MAX_LAND_COUNT];
        uint8_t cbDoubleLineCard[MAX_LAND_COUNT];
        uint8_t cbLineCardCount;
        uint8_t cbThreeLineCardCount;
        uint8_t cbDoubleLineCount;
        GetAllLineCard(cbHandCardData, cbHandCardCount, cbLineCard, cbLineCardCount);
        GetAllThreeCard(cbHandCardData, cbHandCardCount, cbThreeLineCard, cbThreeLineCardCount);
        GetAllDoubleCard(cbHandCardData, cbHandCardCount, cbDoubleLineCard, cbDoubleLineCount);

        //如果有顺牌和单只或一对，而且单只或对比地主的小，则先出顺
        {
            if (cbLineCardCount + 1 == cbHandCardCount && CT_SINGLE == GetCardType(cbLineCard, cbLineCardCount))
            {
                OutCardResult.cbCardCount = cbLineCardCount;
                memcpy(OutCardResult.cbResultCard, cbLineCard, cbLineCardCount);
            }
            else if (cbThreeLineCardCount + 1 == cbHandCardCount &&
                     CT_THREE_LINE == GetCardType(cbThreeLineCard, cbThreeLineCardCount))
            {
                OutCardResult.cbCardCount = cbThreeLineCardCount;
                memcpy(OutCardResult.cbResultCard, cbThreeLineCard, cbThreeLineCardCount);
            }
            else if (cbDoubleLineCount + 1 == cbHandCardCount &&
                     CT_DOUBLE_LINE == GetCardType(cbDoubleLineCard, cbDoubleLineCount))
            {
                OutCardResult.cbCardCount = cbDoubleLineCount;
                memcpy(OutCardResult.cbResultCard, cbDoubleLineCard, cbDoubleLineCount);
            }
                //双王炸弹和一手
            else if (cbHandCardCount > 2 && cbHandCardData[0] == 0x4f && cbHandCardData[1] == 0x4e &&
                     CT_ERROR != GetCardType(cbHandCardData + 2, cbHandCardCount - 2))
            {
                OutCardResult.cbCardCount = 2;
                OutCardResult.cbResultCard[0] = 0x4f;
                OutCardResult.cbResultCard[1] = 0x4e;
            }

            if (OutCardResult.cbCardCount > 0)
            {
                return;
            }
        }
        //对王加一只
        if (cbHandCardCount == 3 && GetCardColor(cbHandCardData[0]) == 0x40
            && GetCardColor(cbHandCardData[1]) == 0x40)
        {
            OutCardResult.cbCardCount = 2;
            OutCardResult.cbResultCard[0] = 0x4f;
            OutCardResult.cbResultCard[1] = 0x4e;
            return;
        }
            //对王
        else if (cbHandCardCount == 2 && GetCardColor(cbHandCardData[0]) == 0x40 &&
                 GetCardColor(cbHandCardData[1]) == 0x40)
        {
            OutCardResult.cbCardCount = 2;
            OutCardResult.cbResultCard[0] = 0x4f;
            OutCardResult.cbResultCard[1] = 0x4e;
            return;
        }
            //只剩一手牌
        else if (CT_ERROR != GetCardType(cbHandCardData, cbHandCardCount))
        {
            OutCardResult.cbCardCount = cbHandCardCount;
            memcpy(OutCardResult.cbResultCard, cbHandCardData, cbHandCardCount);
            return;
        }

        //只剩一张和一手
        if (cbHandCardCount >= 2)
        {
            //地主扑克
            tagOutCardTypeResult BankerCanOutCardType1[13];
            memset(BankerCanOutCardType1, 0, sizeof(BankerCanOutCardType1));
            tagOutCardTypeResult BankerCanOutCardType2[13];
            memset(BankerCanOutCardType2, 0, sizeof(BankerCanOutCardType2));

            uint8_t cbFirstHandCardType = GetCardType(cbHandCardData, cbHandCardCount - 1);
            uint8_t cbSecondHandCardType = GetCardType(cbHandCardData + 1, cbHandCardCount - 1);

            //地主可以出的牌
            if (cbFirstHandCardType != CT_ERROR)
                AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], cbHandCardData,
                                   cbHandCardCount - 1, BankerCanOutCardType1);
            if (cbSecondHandCardType != CT_ERROR)
                AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser],
                                   cbHandCardData + 1,
                                   cbHandCardCount - 1, BankerCanOutCardType2);

            if (cbSecondHandCardType != CT_ERROR && cbSecondHandCardType != CT_FOUR_TAKE_ONE &&
                cbSecondHandCardType != CT_FOUR_TAKE_TWO &&
                BankerCanOutCardType2[cbSecondHandCardType].cbCardTypeCount == 0 &&
                BankerCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount == 0)
            {
                OutCardResult.cbCardCount = cbHandCardCount - 1;
                memcpy(OutCardResult.cbResultCard, cbHandCardData + 1, cbHandCardCount - 1);
                return;
            }

            if (cbFirstHandCardType != CT_ERROR && cbFirstHandCardType != CT_FOUR_TAKE_ONE &&
                cbFirstHandCardType != CT_FOUR_TAKE_TWO &&
                BankerCanOutCardType1[cbFirstHandCardType].cbCardTypeCount == 0 &&
                BankerCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount == 0)
            {
                OutCardResult.cbCardCount = cbHandCardCount - 1;
                memcpy(OutCardResult.cbResultCard, cbHandCardData, cbHandCardCount - 1);
                return;
            }

            if (GetCardLogicValue(cbHandCardData[0]) >= GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) &&
                CT_ERROR != cbSecondHandCardType && cbSecondHandCardType != CT_FOUR_TAKE_ONE &&
                cbSecondHandCardType != CT_FOUR_TAKE_TWO &&
                BankerCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount == 0)
            {
                OutCardResult.cbCardCount = 1;
                OutCardResult.cbResultCard[0] = cbHandCardData[0];
                return;
            }

            if (CT_ERROR != cbSecondHandCardType && cbSecondHandCardType != CT_FOUR_TAKE_ONE &&
                cbSecondHandCardType != CT_FOUR_TAKE_TWO &&
                BankerCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount == 0)
            {
                OutCardResult.cbCardCount = cbHandCardCount - 1;
                memcpy(OutCardResult.cbResultCard, cbHandCardData + 1, cbHandCardCount - 1);
                return;
            }
        }


        //下家为地主，而且地主扑克少于5张
        //	if(m_cbUserCardCount[m_wBankerUser]<=5)
        {
            //分析扑克
            tagOutCardTypeResult MeOutCardTypeResult[13];
            memset(MeOutCardTypeResult, 0, sizeof(MeOutCardTypeResult));
            AnalyseOutCardType(cbHandCardData, cbHandCardCount, MeOutCardTypeResult);

            //对家扑克
            uint16_t wFriendID;
            for (uint16_t wChairID = 0; wChairID < GAME_LAND_PLAYER; ++wChairID)
                if (wChairID != m_wBankerUser && wMeChairID != wChairID) wFriendID = wChairID;

            //计算单牌
            uint8_t cbMinSingleCardCount[4];
            cbMinSingleCardCount[0] = MAX_LAND_COUNT;
            cbMinSingleCardCount[1] = MAX_LAND_COUNT;
            cbMinSingleCardCount[2] = MAX_LAND_COUNT;
            cbMinSingleCardCount[3] = MAX_LAND_COUNT;
            uint8_t cbIndex[4] = {0};
            uint8_t cbOutcardType[4] = {CT_ERROR};
            uint8_t cbMinValue = MAX_LAND_COUNT;
            uint8_t cbMinSingleCountInFour = MAX_LAND_COUNT;
            uint8_t cbMinCardType = CT_ERROR;
            uint8_t cbMinIndex = 0;

            //除炸弹外的牌
            for (uint8_t cbCardType = CT_DOUBLE; cbCardType < CT_BOMB_CARD; ++cbCardType)
            {
                tagOutCardTypeResult const &tmpCardResult = MeOutCardTypeResult[cbCardType];

                //相同牌型，相同长度，单连，对连等相同牌型可能长度不一样
                uint8_t cbThisHandCardCount = MAX_LAND_COUNT;

                //地主扑克
                tagOutCardTypeResult BankerCanOutCard[13];
                memset(BankerCanOutCard, 0, sizeof(BankerCanOutCard));

                tagOutCardTypeResult FriendOutCardTypeResult[13];
                memset(FriendOutCardTypeResult, 0, sizeof(FriendOutCardTypeResult));

                for (uint8_t i = 0; i < tmpCardResult.cbCardTypeCount; ++i)
                {
                    uint8_t cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,
                                                               tmpCardResult.cbCardData[i],
                                                               tmpCardResult.cbEachHandCardCount[i]);

                    //重新分析
                    if (tmpCardResult.cbEachHandCardCount[i] != cbThisHandCardCount)
                    {
                        AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser],
                                           tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i],
                                           BankerCanOutCard);
                        AnalyseOutCardType(m_cbAllCardData[wFriendID], m_cbUserCardCount[wFriendID],
                                           tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i],
                                           FriendOutCardTypeResult);
                    }

                    uint8_t cbMaxValue = 0;
                    uint8_t Index = 0;

                    //地主可以压牌，而且队友不可以压地主
                    if ((BankerCanOutCard[cbCardType].cbCardTypeCount > 0 &&
                         FriendOutCardTypeResult[cbCardType].cbCardTypeCount == 0) ||
                        (BankerCanOutCard[cbCardType].cbCardTypeCount > 0 &&
                         FriendOutCardTypeResult[cbCardType].cbCardTypeCount > 0 &&
                         GetCardLogicValue(FriendOutCardTypeResult[cbCardType].cbCardData[0][0]) <=
                         GetCardLogicValue(BankerCanOutCard[cbCardType].cbCardData[0][0])))
                    {
                        continue;
                    }
                    //是否有大牌
                    if (tmpCardResult.cbEachHandCardCount[i] != cbHandCardCount)
                    {
                        bool bHaveLargeCard = false;
                        for (uint8_t j = 0; j < tmpCardResult.cbEachHandCardCount[i]; ++j)
                            if (GetCardLogicValue(tmpCardResult.cbCardData[i][j]) >= 15) bHaveLargeCard = true;
                        if (cbCardType != CT_SINGLE_LINE && cbCardType != CT_DOUBLE_LINE &&
                            GetCardLogicValue(tmpCardResult.cbCardData[i][0]) == 14)
                            bHaveLargeCard = true;

                        if (bHaveLargeCard) continue;
                    }

                    //地主是否可以走掉，这里都没有考虑炸弹
                    if (tmpCardResult.cbEachHandCardCount[i] == m_cbUserCardCount[m_wBankerUser] &&
                        GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) >
                        GetCardLogicValue(tmpCardResult.cbCardData[i][0]))
                        continue;

                    //搜索cbMinSingleCardCount[4]的最大值
                    for (uint8_t j = 0; j < 4; ++j)
                    {
                        if (cbMinSingleCardCount[j] >= cbTmpCount)
                        {
                            cbMinSingleCardCount[j] = cbTmpCount;
                            cbIndex[j] = i;
                            cbOutcardType[j] = cbCardType;
                            break;
                        }
                    }

                    //保存最小值
                    if (cbMinSingleCountInFour >= cbTmpCount)
                    {
                        //最小牌型
                        cbMinCardType = cbCardType;
                        //最小牌型中的最小单牌
                        cbMinSingleCountInFour = cbTmpCount;
                        //最小牌型中的最小牌
                        cbMinIndex = i;
                    }
                }
            }

            if (cbMinSingleCountInFour >= AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, NULL, 0) + 3 &&
                m_cbUserCardCount[m_wBankerUser] > 4)
                cbMinSingleCountInFour = MAX_LAND_COUNT;

            if (cbMinSingleCountInFour != MAX_LAND_COUNT)
            {
                uint8_t Index = cbMinIndex;

                //选择最小牌
                for (uint8_t i = 0; i < 4; ++i)
                {
                    if (cbOutcardType[i] == cbMinCardType && cbMinSingleCardCount[i] <= cbMinSingleCountInFour &&
                        GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0]) <
                        GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[Index][0]))
                        Index = cbIndex[i];
                }

                //对王加一只
                if (cbHandCardCount == 3 && GetCardColor(cbHandCardData[0]) == 0x40 &&
                    GetCardColor(cbHandCardData[1]) == 0x40)
                {
                    OutCardResult.cbCardCount = 2;
                    OutCardResult.cbResultCard[0] = 0x4f;
                    OutCardResult.cbResultCard[1] = 0x4e;
                    return;
                }
                    //对王
                else if (cbHandCardCount == 2 && GetCardColor(cbHandCardData[0]) == 0x40 &&
                         GetCardColor(cbHandCardData[1]) == 0x40)
                {
                    OutCardResult.cbCardCount = 2;
                    OutCardResult.cbResultCard[0] = 0x4f;
                    OutCardResult.cbResultCard[1] = 0x4e;
                    return;
                }
                else
                {
                    //设置变量
                    OutCardResult.cbCardCount = MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
                    memcpy(OutCardResult.cbResultCard, MeOutCardTypeResult[cbMinCardType].cbCardData[Index],
                           MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index] * sizeof(uint8_t));
                    return;
                }

                ASSERT(OutCardResult.cbCardCount > 0);

                return;
            }

            //如果地主扑克少于5，还没有找到适合的牌则从大出到小
            if (OutCardResult.cbCardCount <= 0 && m_cbUserCardCount[m_wBankerUser] <= 5)
            {
                //只有一张牌时不能放地主走
                if (m_cbUserCardCount[m_wBankerUser] == 1 && MeOutCardTypeResult[CT_SINGLE].cbCardTypeCount > 0)
                {
                    uint8_t Index = MAX_LAND_COUNT;
                    for (uint8_t i = 0; i < MeOutCardTypeResult[CT_SINGLE].cbCardTypeCount; ++i)
                    {
                        if (GetCardLogicValue(MeOutCardTypeResult[CT_SINGLE].cbCardData[i][0]) >=
                            GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]))
                        {
                            Index = i;
                        }
                        else break;
                    }

                    if (MAX_LAND_COUNT != Index)
                    {
                        OutCardResult.cbCardCount = MeOutCardTypeResult[CT_SINGLE].cbEachHandCardCount[Index];
                        memcpy(OutCardResult.cbResultCard, MeOutCardTypeResult[CT_SINGLE].cbCardData[Index],
                               OutCardResult.cbCardCount);
                        return;
                    }
                }
            }
        }

        uint8_t cbFirstCard = 0;
        //过滤王和2
        for (uint8_t i = 0; i < cbHandCardCount; ++i)
            if (GetCardLogicValue(cbHandCardData[i]) < 15)
            {
                cbFirstCard = i;
                break;
            }

        if (cbFirstCard < cbHandCardCount - 1)
            AnalyseOutCardType(cbHandCardData + cbFirstCard, cbHandCardCount - cbFirstCard, CardTypeResult);
        else
            AnalyseOutCardType(cbHandCardData, cbHandCardCount, CardTypeResult);

        //计算单牌
        uint8_t cbMinSingleCardCount[4];
        cbMinSingleCardCount[0] = MAX_LAND_COUNT;
        cbMinSingleCardCount[1] = MAX_LAND_COUNT;
        cbMinSingleCardCount[2] = MAX_LAND_COUNT;
        cbMinSingleCardCount[3] = MAX_LAND_COUNT;
        uint8_t cbIndex[4] = {0};
        uint8_t cbOutcardType[4] = {CT_ERROR};
        uint8_t cbMinValue = MAX_LAND_COUNT;
        uint8_t cbMinSingleCountInFour = MAX_LAND_COUNT;
        uint8_t cbMinCardType = CT_ERROR;
        uint8_t cbMinIndex = 0;

        //分析地主单牌
        uint8_t cbBankerSingleCardData[MAX_LAND_COUNT];
        uint8_t cbBankerSingleCardCount = AnalyseSinleCardCount(m_cbAllCardData[m_wBankerUser],
                                                                m_cbUserCardCount[m_wBankerUser], NULL, 0,
                                                                cbBankerSingleCardData);
        uint8_t cbBankerSingleCardLogic = 0;
        if (cbBankerSingleCardCount > 2 && GetCardLogicValue(cbBankerSingleCardData[1]) <= 10)
            cbBankerSingleCardLogic = GetCardLogicValue(cbBankerSingleCardData[1]);
        else if (cbBankerSingleCardCount > 0 &&
                 GetCardLogicValue(cbBankerSingleCardData[0]) <= 10)
            cbBankerSingleCardLogic = GetCardLogicValue(cbBankerSingleCardData[0]);

        //除炸弹外的牌
        for (uint8_t cbCardType = CT_SINGLE; cbCardType < CT_BOMB_CARD; ++cbCardType)
        {
            tagOutCardTypeResult const &tmpCardResult = CardTypeResult[cbCardType];
            for (uint8_t i = 0; i < tmpCardResult.cbCardTypeCount; ++i)
            {
                //不能放走地主小牌
                if (cbCardType == CT_SINGLE &&
                    GetCardLogicValue(tmpCardResult.cbCardData[i][0]) < cbBankerSingleCardLogic)
                    continue;

                uint8_t cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, tmpCardResult.cbCardData[i],
                                                           tmpCardResult.cbEachHandCardCount[i]);

                uint8_t cbMaxValue = 0;
                uint8_t Index = 0;

                //搜索cbMinSingleCardCount[4]的最大值
                for (uint8_t j = 0; j < 4; ++j)
                {
                    if (cbMinSingleCardCount[j] >= cbTmpCount)
                    {
                        cbMinSingleCardCount[j] = cbTmpCount;
                        cbIndex[j] = i;
                        cbOutcardType[j] = cbCardType;
                        break;
                    }
                }

                //保存最小值
                if (cbMinSingleCountInFour >= cbTmpCount)
                {
                    //最小牌型
                    cbMinCardType = cbCardType;
                    //最小牌型中的最小单牌
                    cbMinSingleCountInFour = cbTmpCount;
                    //最小牌型中的最小牌
                    cbMinIndex = i;
                }
            }
        }

        if (cbMinSingleCountInFour != MAX_LAND_COUNT)
        {
            uint8_t Index = cbMinIndex;

            //选择最小牌
            for (uint8_t i = 0; i < 4; ++i)
            {
                if (cbOutcardType[i] == cbMinCardType && cbMinSingleCardCount[i] <= cbMinSingleCountInFour &&
                    GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0]) <
                    GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[Index][0]))
                    Index = cbIndex[i];
            }

            //对王加一只
            if (cbHandCardCount == 3 && GetCardColor(cbHandCardData[0]) == 0x40 &&
                GetCardColor(cbHandCardData[1]) == 0x40)
            {
                OutCardResult.cbCardCount = 2;
                OutCardResult.cbResultCard[0] = 0x4f;
                OutCardResult.cbResultCard[1] = 0x4e;
                return;
            }
                //对王
            else if (cbHandCardCount == 2 && GetCardColor(cbHandCardData[0]) == 0x40 &&
                     GetCardColor(cbHandCardData[1]) == 0x40)
            {
                OutCardResult.cbCardCount = 2;
                OutCardResult.cbResultCard[0] = 0x4f;
                OutCardResult.cbResultCard[1] = 0x4e;
                return;
            }
            else
            {
                //设置变量
                OutCardResult.cbCardCount = CardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
                memcpy(OutCardResult.cbResultCard, CardTypeResult[cbMinCardType].cbCardData[Index],
                       CardTypeResult[cbMinCardType].cbEachHandCardCount[Index] * sizeof(uint8_t));
                return;
            }

            ASSERT(OutCardResult.cbCardCount > 0);

            return;
        }
            //如果只剩炸弹
        else
        {
            uint8_t Index = 0;
            uint8_t cbLogicCardValue = GetCardLogicValue(0x4F) + 1;
            //最小炸弹
            for (uint8_t i = 0; i < CardTypeResult[CT_BOMB_CARD].cbCardTypeCount; ++i)
                if (cbLogicCardValue > GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]))
                {
                    cbLogicCardValue = GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]);
                    Index = i;
                }

            //设置变量
            OutCardResult.cbCardCount = CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index];
            memcpy(OutCardResult.cbResultCard, CardTypeResult[CT_BOMB_CARD].cbCardData[Index],
                   CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] * sizeof(uint8_t));

            return;
        }

        //如果都没有搜索到就出最小的一张
        OutCardResult.cbCardCount = 1;
        OutCardResult.cbResultCard[0] = cbHandCardData[cbHandCardCount - 1];
        return;
    }

//地主上家（后出牌）
    void CLandLogic::UpsideOfBankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint16_t wOutCardUser,
                                           const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount,
                                           tagOutCardResult &OutCardResult) {

        //零下标没用
        tagOutCardTypeResult CardTypeResult[12 + 1];
        memset(CardTypeResult, 0, sizeof(CardTypeResult));

        //初始变量
        memset(&OutCardResult, 0, sizeof(OutCardResult));

        //出牌类型
        uint8_t cbOutCardType = GetCardType(cbTurnCardData, cbTurnCardCount);

        //搜索可出牌
        tagOutCardTypeResult BankerOutCardTypeResult[13];
        memset(BankerOutCardTypeResult, 0, sizeof(BankerOutCardTypeResult));

        AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], BankerOutCardTypeResult);
        AnalyseOutCardType(cbHandCardData, cbHandCardCount, cbTurnCardData, cbTurnCardCount, CardTypeResult);

        //只剩炸弹
        if (cbHandCardCount == CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0])
        {
            OutCardResult.cbCardCount = CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0];
            memcpy(OutCardResult.cbResultCard, CardTypeResult[CT_BOMB_CARD].cbCardData, OutCardResult.cbCardCount);

            return;
        }
            //双王炸弹和一手
        else if (cbHandCardCount > 2 && cbHandCardData[0] == 0x4f && cbHandCardData[1] == 0x4e &&
                 CT_ERROR != GetCardType(cbHandCardData + 2, cbHandCardCount - 2))
        {
            OutCardResult.cbCardCount = 2;
            OutCardResult.cbResultCard[0] = 0x4f;
            OutCardResult.cbResultCard[1] = 0x4e;
            return;
        }

        //如果庄家没有此牌型了则不压对家牌
        if (m_cbUserCardCount[m_wBankerUser] <= 5 && wOutCardUser != m_wBankerUser &&
            (BankerOutCardTypeResult[cbOutCardType].cbCardTypeCount == 0 ||
             GetCardLogicValue(BankerOutCardTypeResult[cbOutCardType].cbCardData[0][0]) <=
             GetCardLogicValue(cbTurnCardData[0])) &&
            CardTypeResult[cbOutCardType].cbEachHandCardCount[0] != cbHandCardCount)//不能一次出完
        {
            //放弃出牌
            return;
        }

        //下家为地主，而且地主扑克少于5张
        if (m_cbUserCardCount[m_wBankerUser] <= 5 && CardTypeResult[cbOutCardType].cbCardTypeCount > 0 &&
            cbOutCardType != CT_BOMB_CARD &&
            ((GetCardLogicValue(cbTurnCardData[0]) < 12 && wOutCardUser != m_wBankerUser &&
              BankerOutCardTypeResult[cbOutCardType].cbCardTypeCount > 0) ||//对家出牌
             (wOutCardUser == m_wBankerUser)))//地主出牌
        {
            uint8_t Index = 0;
            //寻找可以压住地主的最小一手牌
            uint8_t cbThisOutTypeMinSingleCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,
                                                                        CardTypeResult[cbOutCardType].cbCardData[0],
                                                                        CardTypeResult[cbOutCardType].cbEachHandCardCount[0]);
            uint8_t cbBestIndex = 255;
            for (uint8_t i = 0; i < CardTypeResult[cbOutCardType].cbCardTypeCount; ++i)
            {
                uint8_t cbTmpSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,
                                                                     CardTypeResult[cbOutCardType].cbCardData[i],
                                                                     CardTypeResult[cbOutCardType].cbEachHandCardCount[i]);

                if ((BankerOutCardTypeResult[cbOutCardType].cbCardTypeCount > 0 &&
                     GetCardLogicValue(CardTypeResult[cbOutCardType].cbCardData[i][0]) >=
                     GetCardLogicValue(BankerOutCardTypeResult[cbOutCardType].cbCardData[0][0]))
                    || (BankerOutCardTypeResult[cbOutCardType].cbCardTypeCount == 0 &&
                        cbTmpSingleCardCount <= cbThisOutTypeMinSingleCount))
                {
                    cbBestIndex = i;
                    cbThisOutTypeMinSingleCount = cbTmpSingleCardCount;
                }

                if ((BankerOutCardTypeResult[cbOutCardType].cbCardTypeCount > 0 &&
                     GetCardLogicValue(CardTypeResult[cbOutCardType].cbCardData[i][0]) >=
                     GetCardLogicValue(BankerOutCardTypeResult[cbOutCardType].cbCardData[0][0])) ||
                    BankerOutCardTypeResult[cbOutCardType].cbCardTypeCount == 0)
                    Index = i;
                else break;
            }

            if (cbBestIndex != 255)
            {
                OutCardResult.cbCardCount = CardTypeResult[cbOutCardType].cbEachHandCardCount[cbBestIndex];
                memcpy(OutCardResult.cbResultCard, CardTypeResult[cbOutCardType].cbCardData[cbBestIndex],
                       OutCardResult.cbCardCount);
            }
            else
            {
                OutCardResult.cbCardCount = CardTypeResult[cbOutCardType].cbEachHandCardCount[Index];
                memcpy(OutCardResult.cbResultCard, CardTypeResult[cbOutCardType].cbCardData[Index],
                       OutCardResult.cbCardCount);
            }

            return;
        }

        //取出四个最小单牌
        uint8_t cbMinSingleCardCount[4];
        cbMinSingleCardCount[0] = MAX_LAND_COUNT;
        cbMinSingleCardCount[1] = MAX_LAND_COUNT;
        cbMinSingleCardCount[2] = MAX_LAND_COUNT;
        cbMinSingleCardCount[3] = MAX_LAND_COUNT;
        uint8_t cbIndex[4] = {0};
        uint8_t cbMinSingleCountInFour = MAX_LAND_COUNT;

        //可出扑克（这里已经过滤掉炸弹了）
        tagOutCardTypeResult const &CanOutCard = CardTypeResult[cbOutCardType];

        for (uint8_t i = 0; i < CanOutCard.cbCardTypeCount; ++i)
        {
            //最小单牌
            uint8_t cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, CanOutCard.cbCardData[i],
                                                       CanOutCard.cbEachHandCardCount[i]);
            uint8_t cbMaxValue = 0;
            uint8_t Index = 0;

            //搜索cbMinSingleCardCount[4]的最大值
            for (uint8_t j = 0; j < 4; ++j)
            {
                if (cbMinSingleCardCount[j] >= cbTmpCount)
                {
                    cbMinSingleCardCount[j] = cbTmpCount;
                    cbIndex[j] = i;
                    break;
                }
            }

        }

        for (uint8_t i = 0; i < 4; ++i)
            if (cbMinSingleCountInFour > cbMinSingleCardCount[i]) cbMinSingleCountInFour = cbMinSingleCardCount[i];


        //原始单牌数
        uint8_t cbOriginSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, NULL, 0);

        //朋友出牌
        bool bFriendOut = m_wBankerUser != wOutCardUser;
        if (bFriendOut)
        {
            if (CanOutCard.cbCardTypeCount > 0)
            {
                //分析地主单牌
                uint8_t cbBankerSingleCardData[MAX_LAND_COUNT];
                uint8_t cbBankerSingleCardCount = AnalyseSinleCardCount(m_cbAllCardData[m_wBankerUser],
                                                                        m_cbUserCardCount[m_wBankerUser], NULL, 0,
                                                                        cbBankerSingleCardData);
                uint8_t cbBankerSingleCardLogic = 0;
                if (cbBankerSingleCardCount > 2 &&
                    GetCardLogicValue(cbBankerSingleCardData[1]) <= 10)
                    cbBankerSingleCardLogic = GetCardLogicValue(cbBankerSingleCardData[1]);
                else if (cbBankerSingleCardCount > 0 &&
                         GetCardLogicValue(cbBankerSingleCardData[0]) <= 10)
                    cbBankerSingleCardLogic = GetCardLogicValue(cbBankerSingleCardData[0]);

                uint8_t cbMinLogicCardValue = GetCardLogicValue(0x4F) + 1;
                bool bFindCard = false;
                uint8_t cbCanOutIndex = 0;
                for (uint8_t i = 0; i < 4; ++i)
                {
                    uint8_t Index = cbIndex[i];

                    bool bCanOut = false;
                    if (cbOutCardType == CT_SINGLE && GetCardLogicValue(cbTurnCardData[0]) < cbBankerSingleCardLogic
                        &&
                        GetCardLogicValue(CanOutCard.cbCardData[Index][0]) <= 14 &&
                        cbMinSingleCardCount[i] < cbOriginSingleCardCount + 2)
                        bCanOut = true;

                    //小于J的牌，或者小于K而且是散牌
                    if (bCanOut ||
                        ((cbMinSingleCardCount[i] < cbOriginSingleCardCount + 4 &&
                          cbMinSingleCardCount[i] <= cbMinSingleCountInFour &&
                          (GetCardLogicValue(CanOutCard.cbCardData[Index][0]) <= 11 ||
                           ((cbMinSingleCardCount[i] < cbOriginSingleCardCount) &&
                            GetCardLogicValue(CanOutCard.cbCardData[Index][0]) <= 13))) &&
                         cbMinLogicCardValue > GetCardLogicValue(CanOutCard.cbCardData[Index][0]) &&
                         cbHandCardCount > 5))
                    {
                        //搜索有没有大牌（针对飞机带翅膀后面的带牌）
                        bool bNoLargeCard = true;
                        for (uint8_t k = 3; k < CanOutCard.cbEachHandCardCount[Index]; ++k)
                        {
                            //有大牌而且不能一次出完
                            if (GetCardLogicValue(CanOutCard.cbCardData[Index][k]) >= 15 &&
                                CanOutCard.cbEachHandCardCount[Index] != cbHandCardCount)
                                bNoLargeCard = false;
                        }
                        if (bNoLargeCard)
                        {
                            bFindCard = true;
                            cbCanOutIndex = Index;
                            cbMinLogicCardValue = GetCardLogicValue(CanOutCard.cbCardData[Index][0]);
                        }
                    }
                    else if (cbHandCardCount < 5 && cbMinSingleCardCount[i] < cbOriginSingleCardCount + 4 &&
                             cbMinSingleCardCount[i] <= cbMinSingleCountInFour &&
                             cbMinLogicCardValue > GetCardLogicValue(CanOutCard.cbCardData[Index][0]))
                    {
                        bFindCard = true;
                        cbCanOutIndex = Index;
                        cbMinLogicCardValue = GetCardLogicValue(CanOutCard.cbCardData[Index][0]);
                    }
                }

                if (bFindCard)
                {

                    //设置变量
                    OutCardResult.cbCardCount = CanOutCard.cbEachHandCardCount[cbCanOutIndex];
                    memcpy(OutCardResult.cbResultCard, CanOutCard.cbCardData[cbCanOutIndex],
                           CanOutCard.cbEachHandCardCount[cbCanOutIndex] * sizeof(uint8_t));

                    return;
                }
                    //手上少于五张牌
                else if (cbHandCardCount <= 5)
                {
                    uint8_t cbMinLogicCard = GetCardLogicValue(0x4f) + 1;
                    uint8_t cbCanOutIndex = 0;
                    for (uint8_t i = 0; i < 4; ++i)
                        if (cbMinSingleCardCount[i] < MAX_LAND_COUNT &&
                            cbMinSingleCardCount[i] <= cbMinSingleCountInFour &&
                            cbMinLogicCard > GetCardLogicValue(CanOutCard.cbCardData[cbIndex[i]][0]) &&
                            GetCardLogicValue(CanOutCard.cbCardData[cbIndex[i]][0]) <= 14)
                        {
                            cbMinLogicCard = GetCardLogicValue(CanOutCard.cbCardData[cbIndex[i]][0]);
                            cbCanOutIndex = cbIndex[i];
                        }

                    if (cbMinLogicCard != (GetCardLogicValue(0x4f) + 1))
                    {
                        //设置变量
                        OutCardResult.cbCardCount = CanOutCard.cbEachHandCardCount[cbCanOutIndex];
                        memcpy(OutCardResult.cbResultCard, CanOutCard.cbCardData[cbCanOutIndex],
                               CanOutCard.cbEachHandCardCount[cbCanOutIndex] * sizeof(uint8_t));

                        return;
                    }
                }

                return;
            }
            else
            {
                return;
            }

        }
            //地主出牌
        else
        {
            if (CanOutCard.cbCardTypeCount > 0)
            {
                uint8_t cbMinLogicCardValue = GetCardLogicValue(0x4F) + 1;
                bool bFindCard = false;
                uint8_t cbCanOutIndex = 0;
                for (uint8_t i = 0; i < 4; ++i)
                {
                    uint8_t Index = cbIndex[i];

                    if ((cbMinSingleCardCount[i] < cbOriginSingleCardCount + 4) &&
                        cbMinSingleCardCount[i] <= cbMinSingleCountInFour &&
                        cbMinLogicCardValue > GetCardLogicValue(CanOutCard.cbCardData[Index][0]))
                    {
                        //针对大牌
                        bool bNoLargeCard = true;

                        //当地主手上牌数大于4，而且地主出的是小于K的牌而且不是地主手上的最大牌时，不能出2去打
                        if (m_cbUserCardCount[m_wBankerUser] >= 4 && cbHandCardCount >= 5 &&
                            CanOutCard.cbEachHandCardCount[Index] >= 2 &&
                            GetCardLogicValue(CanOutCard.cbCardData[Index][0]) >= 15 &&
                            GetCardLogicValue(cbTurnCardData[0]) < 13 &&
                            GetCardLogicValue(cbTurnCardData[0]) <
                            GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) &&
                            CanOutCard.cbEachHandCardCount[Index] != cbHandCardCount)
                            bNoLargeCard = false;

                        //搜索有没有大牌（针对飞机带翅膀后面的带牌）
                        for (uint8_t k = 3; k < CanOutCard.cbEachHandCardCount[Index]; ++k)
                        {
                            if (GetCardLogicValue(CanOutCard.cbCardData[Index][k]) >= 15 &&
                                CanOutCard.cbEachHandCardCount[Index] != cbHandCardCount)
                                bNoLargeCard = false;
                        }
                        if (bNoLargeCard)
                        {
                            bFindCard = true;
                            cbCanOutIndex = Index;
                            cbMinLogicCardValue = GetCardLogicValue(CanOutCard.cbCardData[Index][0]);
                        }
                    }
                }

                if (bFindCard)
                {
                    //地主的最大牌
                    uint8_t cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]);
                    bool bCanOut = true;

                    //王只压2
                    if (GetCardLogicValue(cbTurnCardData[0]) < cbLargestLogicCard)
                    {
                        if (GetCardColor(CanOutCard.cbCardData[cbCanOutIndex][0]) == 0x40 &&
                            GetCardLogicValue(cbTurnCardData[0]) <= 14 && cbHandCardCount > 5)
                        {
                            bCanOut = false;
                        }
                    }

                    if (bCanOut)
                    {
                        //设置变量
                        OutCardResult.cbCardCount = CanOutCard.cbEachHandCardCount[cbCanOutIndex];
                        memcpy(OutCardResult.cbResultCard, CanOutCard.cbCardData[cbCanOutIndex],
                               CanOutCard.cbEachHandCardCount[cbCanOutIndex] * sizeof(uint8_t));

                        return;
                    }
                }

                if (cbOutCardType == CT_SINGLE)
                {
                    //地主的最大牌
                    uint8_t cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]);

                    if (GetCardLogicValue(cbTurnCardData[0]) == 14 ||
                        GetCardLogicValue(cbTurnCardData[0]) >= cbLargestLogicCard ||
                        (GetCardLogicValue(cbTurnCardData[0]) < cbLargestLogicCard - 1) ||
                        m_cbUserCardCount[m_wBankerUser] <= 5)
                    {
                        //取一张大于等于2而且要比地主出的牌大的牌，
                        uint8_t cbIndex = MAX_LAND_COUNT;
                        for (uint8_t i = 0; i < cbHandCardCount; ++i)
                            if (GetCardLogicValue(cbHandCardData[i]) > GetCardLogicValue(cbTurnCardData[0]) &&
                                GetCardLogicValue(cbHandCardData[i]) >= 15)
                            {
                                cbIndex = i;
                            }
                        if (cbIndex != MAX_LAND_COUNT)
                        {
                            //设置变量
                            OutCardResult.cbCardCount = 1;
                            OutCardResult.cbResultCard[0] = cbHandCardData[cbIndex];

                            return;
                        }
                    }
                }
            }

            //还要考虑炸弹
            if (CardTypeResult[CT_BOMB_CARD].cbCardTypeCount > 0 && cbHandCardCount <= 10)
            {
                tagOutCardTypeResult const &BomCard = CardTypeResult[CT_BOMB_CARD];
                uint8_t cbMinLogicValue = GetCardLogicValue(BomCard.cbCardData[0][0]);
                uint8_t Index = 0;
                for (uint8_t i = 0; i < BomCard.cbCardTypeCount; ++i)
                {
                    if (cbMinLogicValue > GetCardLogicValue(BomCard.cbCardData[i][0]))
                    {
                        cbMinLogicValue = GetCardLogicValue(BomCard.cbCardData[i][0]);
                        Index = i;
                    }
                }

                //判断出了炸弹后的单牌数
                uint8_t cbSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,
                                                                  BomCard.cbCardData[Index],
                                                                  BomCard.cbEachHandCardCount[Index]);
                if (cbSingleCardCount >= 3 ||
                    (cbOutCardType == CT_SINGLE && GetCardLogicValue(cbTurnCardData[0]) < 15))
                    return;

                //设置变量
                OutCardResult.cbCardCount = BomCard.cbEachHandCardCount[Index];
                memcpy(OutCardResult.cbResultCard, BomCard.cbCardData[Index],
                       BomCard.cbEachHandCardCount[Index] * sizeof(uint8_t));

                return;
            }

            return;
        }
        return;
    }

//地主下家（先出牌）
    void CLandLogic::UndersideOfBankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint16_t wMeChairID,
                                              tagOutCardResult &OutCardResult) {
        //零下标没用
        tagOutCardTypeResult CardTypeResult[12 + 1];
        memset(CardTypeResult, 0, sizeof(CardTypeResult));

        //初始变量
        memset(&OutCardResult, 0, sizeof(OutCardResult));

        uint8_t cbLineCard[MAX_LAND_COUNT];
        uint8_t cbThreeLineCard[MAX_LAND_COUNT];
        uint8_t cbDoubleLineCard[MAX_LAND_COUNT];
        uint8_t cbLineCardCount;
        uint8_t cbThreeLineCardCount;
        uint8_t cbDoubleLineCount;
        GetAllLineCard(cbHandCardData, cbHandCardCount, cbLineCard, cbLineCardCount);
        GetAllThreeCard(cbHandCardData, cbHandCardCount, cbThreeLineCard, cbThreeLineCardCount);
        GetAllDoubleCard(cbHandCardData, cbHandCardCount, cbDoubleLineCard, cbDoubleLineCount);

        //如果有顺牌和单只或一对，而且单只或对比地主的小，则先出顺
        {
            if (cbLineCardCount + 1 == cbHandCardCount && CT_SINGLE == GetCardType(cbLineCard, cbLineCardCount))
            {
                OutCardResult.cbCardCount = cbLineCardCount;
                memcpy(OutCardResult.cbResultCard, cbLineCard, cbLineCardCount);
            }
            else if (cbThreeLineCardCount + 1 == cbHandCardCount &&
                     CT_THREE_LINE == GetCardType(cbThreeLineCard, cbThreeLineCardCount))
            {
                OutCardResult.cbCardCount = cbThreeLineCardCount;
                memcpy(OutCardResult.cbResultCard, cbThreeLineCard, cbThreeLineCardCount);
            }
            else if (cbDoubleLineCount + 1 == cbHandCardCount &&
                     CT_DOUBLE_LINE == GetCardType(cbDoubleLineCard, cbDoubleLineCount))
            {
                OutCardResult.cbCardCount = cbDoubleLineCount;
                memcpy(OutCardResult.cbResultCard, cbDoubleLineCard, cbDoubleLineCount);
            }
                //双王炸弹和一手
            else if (cbHandCardCount > 2 && cbHandCardData[0] == 0x4f && cbHandCardData[1] == 0x4e &&
                     CT_ERROR != GetCardType(cbHandCardData + 2, cbHandCardCount - 2))
            {
                OutCardResult.cbCardCount = 2;
                OutCardResult.cbResultCard[0] = 0x4f;
                OutCardResult.cbResultCard[1] = 0x4e;
            }

            if (OutCardResult.cbCardCount > 0)
            {
                return;
            }
        }
        //对王加一只
        if (cbHandCardCount == 3 && GetCardColor(cbHandCardData[0]) == 0x40
            && GetCardColor(cbHandCardData[1]) == 0x40)
        {
            OutCardResult.cbCardCount = 2;
            OutCardResult.cbResultCard[0] = 0x4f;
            OutCardResult.cbResultCard[1] = 0x4e;
            return;
        }
            //对王
        else if (cbHandCardCount == 2 && GetCardColor(cbHandCardData[0]) == 0x40 &&
                 GetCardColor(cbHandCardData[1]) == 0x40)
        {
            OutCardResult.cbCardCount = 2;
            OutCardResult.cbResultCard[0] = 0x4f;
            OutCardResult.cbResultCard[1] = 0x4e;
            return;
        }
            //只剩一手牌
        else if (CT_ERROR != GetCardType(cbHandCardData, cbHandCardCount))
        {
            OutCardResult.cbCardCount = cbHandCardCount;
            memcpy(OutCardResult.cbResultCard, cbHandCardData, cbHandCardCount);
            return;
        }

        //只剩一张和一手
        if (cbHandCardCount >= 2)
        {
            //地主扑克
            tagOutCardTypeResult BankerCanOutCardType1[13];
            memset(BankerCanOutCardType1, 0, sizeof(BankerCanOutCardType1));
            tagOutCardTypeResult BankerCanOutCardType2[13];
            memset(BankerCanOutCardType2, 0, sizeof(BankerCanOutCardType2));

            uint8_t cbFirstHandCardType = GetCardType(cbHandCardData, cbHandCardCount - 1);
            uint8_t cbSecondHandCardType = GetCardType(cbHandCardData + 1, cbHandCardCount - 1);

            //地主可以出的牌
            if (cbFirstHandCardType != CT_ERROR)
                AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], cbHandCardData,
                                   cbHandCardCount - 1, BankerCanOutCardType1);
            if (cbSecondHandCardType != CT_ERROR)
                AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser],
                                   cbHandCardData + 1,
                                   cbHandCardCount - 1, BankerCanOutCardType2);

            if (cbSecondHandCardType != CT_ERROR && cbSecondHandCardType != CT_FOUR_TAKE_ONE &&
                cbSecondHandCardType != CT_FOUR_TAKE_TWO &&
                BankerCanOutCardType2[cbSecondHandCardType].cbCardTypeCount == 0 &&
                BankerCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount == 0)
            {
                OutCardResult.cbCardCount = cbHandCardCount - 1;
                memcpy(OutCardResult.cbResultCard, cbHandCardData + 1, cbHandCardCount - 1);
                return;
            }

            if (cbFirstHandCardType != CT_ERROR && cbFirstHandCardType != CT_FOUR_TAKE_ONE &&
                cbFirstHandCardType != CT_FOUR_TAKE_TWO &&
                BankerCanOutCardType1[cbFirstHandCardType].cbCardTypeCount == 0 &&
                BankerCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount == 0)
            {
                OutCardResult.cbCardCount = cbHandCardCount - 1;
                memcpy(OutCardResult.cbResultCard, cbHandCardData, cbHandCardCount - 1);
                return;
            }

            if (GetCardLogicValue(cbHandCardData[0]) >= GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) &&
                CT_ERROR != cbSecondHandCardType && cbSecondHandCardType != CT_FOUR_TAKE_ONE &&
                cbSecondHandCardType != CT_FOUR_TAKE_TWO &&
                BankerCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount == 0)
            {
                OutCardResult.cbCardCount = 1;
                OutCardResult.cbResultCard[0] = cbHandCardData[0];
                return;
            }

            if (CT_ERROR != cbSecondHandCardType && cbSecondHandCardType != CT_FOUR_TAKE_ONE &&
                cbSecondHandCardType != CT_FOUR_TAKE_TWO &&
                BankerCanOutCardType2[CT_BOMB_CARD].cbCardTypeCount == 0)
            {
                OutCardResult.cbCardCount = cbHandCardCount - 1;
                memcpy(OutCardResult.cbResultCard, cbHandCardData + 1, cbHandCardCount - 1);
                return;
            }
        }


        //下家为地主，而且地主扑克少于5张
        //	if(m_cbUserCardCount[m_wBankerUser]<=5)
        {
            //分析扑克
            tagOutCardTypeResult MeOutCardTypeResult[13];
            memset(MeOutCardTypeResult, 0, sizeof(MeOutCardTypeResult));
            AnalyseOutCardType(cbHandCardData, cbHandCardCount, MeOutCardTypeResult);

            //对家扑克
            uint16_t wFriendID;
            for (uint16_t wChairID = 0; wChairID < GAME_LAND_PLAYER; ++wChairID)
                if (wChairID != m_wBankerUser && wMeChairID != wChairID) wFriendID = wChairID;

            /*
    		uint8 cbFirstCard=0 ;
    		//过滤王和2
    		for(uint8 i=0; i<cbHandCardCount; ++i)
    		if(GetCardLogicValue(cbHandCardData[i])<15)
    		{
    		cbFirstCard = i ;
    		break ;
    		}


    		if(cbFirstCard<cbHandCardCount-1)
    		AnalyseOutCardType(cbHandCardData+cbFirstCard, cbHandCardCount-cbFirstCard, MeOutCardTypeResult) ;
    		else
    		AnalyseOutCardType(cbHandCardData, cbHandCardCount, MeOutCardTypeResult) ;*/


            //计算单牌
            uint8_t cbMinSingleCardCount[4];
            cbMinSingleCardCount[0] = MAX_LAND_COUNT;
            cbMinSingleCardCount[1] = MAX_LAND_COUNT;
            cbMinSingleCardCount[2] = MAX_LAND_COUNT;
            cbMinSingleCardCount[3] = MAX_LAND_COUNT;
            uint8_t cbIndex[4] = {0};
            uint8_t cbOutcardType[4] = {CT_ERROR};
            uint8_t cbMinValue = MAX_LAND_COUNT;
            uint8_t cbMinSingleCountInFour = MAX_LAND_COUNT;
            uint8_t cbMinCardType = CT_ERROR;
            uint8_t cbMinIndex = 0;

            //除炸弹外的牌
            for (uint8_t cbCardType = CT_DOUBLE; cbCardType < CT_BOMB_CARD; ++cbCardType)
            {
                tagOutCardTypeResult const &tmpCardResult = MeOutCardTypeResult[cbCardType];

                //相同牌型，相同长度，单连，对连等相同牌型可能长度不一样
                uint8_t cbThisHandCardCount = MAX_LAND_COUNT;

                //地主扑克
                tagOutCardTypeResult BankerCanOutCard[13];
                memset(BankerCanOutCard, 0, sizeof(BankerCanOutCard));

                tagOutCardTypeResult FriendOutCardTypeResult[13];
                memset(FriendOutCardTypeResult, 0, sizeof(FriendOutCardTypeResult));

                for (uint8_t i = 0; i < tmpCardResult.cbCardTypeCount; ++i)
                {
                    uint8_t cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,
                                                               tmpCardResult.cbCardData[i],
                                                               tmpCardResult.cbEachHandCardCount[i]);

                    //重新分析
                    if (tmpCardResult.cbEachHandCardCount[i] != cbThisHandCardCount)
                    {
                        AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser],
                                           tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i],
                                           BankerCanOutCard);
                        AnalyseOutCardType(m_cbAllCardData[wFriendID], m_cbUserCardCount[wFriendID],
                                           tmpCardResult.cbCardData[i], tmpCardResult.cbEachHandCardCount[i],
                                           FriendOutCardTypeResult);
                    }

                    uint8_t cbMaxValue = 0;
                    uint8_t Index = 0;

                    //地主可以压牌，而且队友不可以压地主
                    if ((BankerCanOutCard[cbCardType].cbCardTypeCount > 0 &&
                         FriendOutCardTypeResult[cbCardType].cbCardTypeCount == 0) ||
                        (BankerCanOutCard[cbCardType].cbCardTypeCount > 0 &&
                         FriendOutCardTypeResult[cbCardType].cbCardTypeCount > 0 &&
                         GetCardLogicValue(FriendOutCardTypeResult[cbCardType].cbCardData[0][0]) <=
                         GetCardLogicValue(BankerCanOutCard[cbCardType].cbCardData[0][0])))
                    {
                        continue;
                    }
                    //是否有大牌
                    if (tmpCardResult.cbEachHandCardCount[i] != cbHandCardCount)
                    {
                        bool bHaveLargeCard = false;
                        for (uint8_t j = 0; j < tmpCardResult.cbEachHandCardCount[i]; ++j)
                            if (GetCardLogicValue(tmpCardResult.cbCardData[i][j]) >= 15) bHaveLargeCard = true;

                        if (cbCardType != CT_SINGLE_LINE && cbCardType != CT_DOUBLE_LINE &&
                            GetCardLogicValue(tmpCardResult.cbCardData[i][0]) == 14)
                            bHaveLargeCard = true;

                        if (bHaveLargeCard) continue;
                    }

                    //地主是否可以走掉，这里都没有考虑炸弹
                    if (tmpCardResult.cbEachHandCardCount[i] == m_cbUserCardCount[m_wBankerUser] &&
                        GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) >
                        GetCardLogicValue(tmpCardResult.cbCardData[i][0]))
                        continue;

                    //搜索cbMinSingleCardCount[4]的最大值
                    for (uint8_t j = 0; j < 4; ++j)
                    {
                        if (cbMinSingleCardCount[j] >= cbTmpCount)
                        {
                            cbMinSingleCardCount[j] = cbTmpCount;
                            cbIndex[j] = i;
                            cbOutcardType[j] = cbCardType;
                            break;
                        }
                    }

                    //保存最小值
                    if (cbMinSingleCountInFour >= cbTmpCount)
                    {
                        //最小牌型
                        cbMinCardType = cbCardType;
                        //最小牌型中的最小单牌
                        cbMinSingleCountInFour = cbTmpCount;
                        //最小牌型中的最小牌
                        cbMinIndex = i;
                    }
                }
            }

            if (cbMinSingleCountInFour >= AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, NULL, 0) + 3 &&
                m_cbUserCardCount[m_wBankerUser] > 4)
                cbMinSingleCountInFour = MAX_LAND_COUNT;

            if (cbMinSingleCountInFour != MAX_LAND_COUNT)
            {
                uint8_t Index = cbMinIndex;

                //选择最小牌
                for (uint8_t i = 0; i < 4; ++i)
                {
                    if (cbOutcardType[i] == cbMinCardType && cbMinSingleCardCount[i] <= cbMinSingleCountInFour &&
                        GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0]) <
                        GetCardLogicValue(MeOutCardTypeResult[cbMinCardType].cbCardData[Index][0]))
                        Index = cbIndex[i];
                }

                //对王加一只
                if (cbHandCardCount == 3 && GetCardColor(cbHandCardData[0]) == 0x40 &&
                    GetCardColor(cbHandCardData[1]) == 0x40)
                {
                    OutCardResult.cbCardCount = 2;
                    OutCardResult.cbResultCard[0] = 0x4f;
                    OutCardResult.cbResultCard[1] = 0x4e;
                    return;
                }
                    //对王
                else if (cbHandCardCount == 2 && GetCardColor(cbHandCardData[0]) == 0x40 &&
                         GetCardColor(cbHandCardData[1]) == 0x40)
                {
                    OutCardResult.cbCardCount = 2;
                    OutCardResult.cbResultCard[0] = 0x4f;
                    OutCardResult.cbResultCard[1] = 0x4e;
                    return;
                }
                else
                {
                    //设置变量
                    OutCardResult.cbCardCount = MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
                    memcpy(OutCardResult.cbResultCard, MeOutCardTypeResult[cbMinCardType].cbCardData[Index],
                           MeOutCardTypeResult[cbMinCardType].cbEachHandCardCount[Index] * sizeof(uint8_t));
                    return;
                }

                ASSERT(OutCardResult.cbCardCount > 0);

                return;
            }

            //如果地主扑克少于5，还没有找到适合的牌则从大出到小
            if (OutCardResult.cbCardCount <= 0 && m_cbUserCardCount[m_wBankerUser] <= 5)
            {
                //只有一张牌时不能放地主走
                if (m_cbUserCardCount[m_wBankerUser] == 1 && MeOutCardTypeResult[CT_SINGLE].cbCardTypeCount > 0)
                {
                    uint8_t Index = MAX_LAND_COUNT;
                    for (uint8_t i = 0; i < MeOutCardTypeResult[CT_SINGLE].cbCardTypeCount; ++i)
                    {
                        if (GetCardLogicValue(MeOutCardTypeResult[CT_SINGLE].cbCardData[i][0]) >=
                            GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]))
                        {
                            Index = i;
                        }
                        else break;
                    }

                    if (MAX_LAND_COUNT != Index)
                    {
                        OutCardResult.cbCardCount = MeOutCardTypeResult[CT_SINGLE].cbEachHandCardCount[Index];
                        memcpy(OutCardResult.cbResultCard, MeOutCardTypeResult[CT_SINGLE].cbCardData[Index],
                               OutCardResult.cbCardCount);
                        return;
                    }
                }
            }
        }

        uint8_t cbFirstCard = 0;
        //过滤王和2
        for (uint8_t i = 0; i < cbHandCardCount; ++i)
            if (GetCardLogicValue(cbHandCardData[i]) < 15)
            {
                cbFirstCard = i;
                break;
            }

        if (cbFirstCard < cbHandCardCount - 1)
            AnalyseOutCardType(cbHandCardData + cbFirstCard, cbHandCardCount - cbFirstCard, CardTypeResult);
        else
            AnalyseOutCardType(cbHandCardData, cbHandCardCount, CardTypeResult);

        //计算单牌
        uint8_t cbMinSingleCardCount[4];
        cbMinSingleCardCount[0] = MAX_LAND_COUNT;
        cbMinSingleCardCount[1] = MAX_LAND_COUNT;
        cbMinSingleCardCount[2] = MAX_LAND_COUNT;
        cbMinSingleCardCount[3] = MAX_LAND_COUNT;
        uint8_t cbIndex[4] = {0};
        uint8_t cbOutcardType[4] = {CT_ERROR};
        uint8_t cbMinValue = MAX_LAND_COUNT;
        uint8_t cbMinSingleCountInFour = MAX_LAND_COUNT;
        uint8_t cbMinCardType = CT_ERROR;
        uint8_t cbMinIndex = 0;

        //除炸弹外的牌
        for (uint8_t cbCardType = CT_SINGLE; cbCardType < CT_BOMB_CARD; ++cbCardType)
        {
            tagOutCardTypeResult const &tmpCardResult = CardTypeResult[cbCardType];
            for (uint8_t i = 0; i < tmpCardResult.cbCardTypeCount; ++i)
            {
                uint8_t cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, tmpCardResult.cbCardData[i],
                                                           tmpCardResult.cbEachHandCardCount[i]);

                uint8_t cbMaxValue = 0;
                uint8_t Index = 0;

                //搜索cbMinSingleCardCount[4]的最大值
                for (uint8_t j = 0; j < 4; ++j)
                {
                    if (cbMinSingleCardCount[j] >= cbTmpCount)
                    {
                        cbMinSingleCardCount[j] = cbTmpCount;
                        cbIndex[j] = i;
                        cbOutcardType[j] = cbCardType;
                        break;
                    }
                }

                //保存最小值
                if (cbMinSingleCountInFour >= cbTmpCount)
                {
                    //最小牌型
                    cbMinCardType = cbCardType;
                    //最小牌型中的最小单牌
                    cbMinSingleCountInFour = cbTmpCount;
                    //最小牌型中的最小牌
                    cbMinIndex = i;
                }
            }
        }

        if (cbMinSingleCountInFour != MAX_LAND_COUNT)
        {
            uint8_t Index = cbMinIndex;

            //选择最小牌
            for (uint8_t i = 0; i < 4; ++i)
            {
                if (cbOutcardType[i] == cbMinCardType && cbMinSingleCardCount[i] <= cbMinSingleCountInFour &&
                    GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[cbIndex[i]][0]) <
                    GetCardLogicValue(CardTypeResult[cbMinCardType].cbCardData[Index][0]))
                    Index = cbIndex[i];
            }

            //对王加一只
            if (cbHandCardCount == 3 && GetCardColor(cbHandCardData[0]) == 0x40 &&
                GetCardColor(cbHandCardData[1]) == 0x40)
            {
                OutCardResult.cbCardCount = 2;
                OutCardResult.cbResultCard[0] = 0x4f;
                OutCardResult.cbResultCard[1] = 0x4e;
                return;
            }
                //对王
            else if (cbHandCardCount == 2 && GetCardColor(cbHandCardData[0]) == 0x40 &&
                     GetCardColor(cbHandCardData[1]) == 0x40)
            {
                OutCardResult.cbCardCount = 2;
                OutCardResult.cbResultCard[0] = 0x4f;
                OutCardResult.cbResultCard[1] = 0x4e;
                return;
            }
            else
            {
                //设置变量
                OutCardResult.cbCardCount = CardTypeResult[cbMinCardType].cbEachHandCardCount[Index];
                memcpy(OutCardResult.cbResultCard, CardTypeResult[cbMinCardType].cbCardData[Index],
                       CardTypeResult[cbMinCardType].cbEachHandCardCount[Index] * sizeof(uint8_t));
                return;
            }

            ASSERT(OutCardResult.cbCardCount > 0);

            return;
        }
            //如果只剩炸弹
        else
        {
            uint8_t Index = 0;
            uint8_t cbLogicCardValue = GetCardLogicValue(0x4F) + 1;
            //最小炸弹
            for (uint8_t i = 0; i < CardTypeResult[CT_BOMB_CARD].cbCardTypeCount; ++i)
                if (cbLogicCardValue > GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]))
                {
                    cbLogicCardValue = GetCardLogicValue(CardTypeResult[CT_BOMB_CARD].cbCardData[i][0]);
                    Index = i;
                }

            //设置变量
            OutCardResult.cbCardCount = CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index];
            memcpy(OutCardResult.cbResultCard, CardTypeResult[CT_BOMB_CARD].cbCardData[Index],
                   CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] * sizeof(uint8_t));

            return;
        }

        //如果都没有搜索到就出最小的一张
        OutCardResult.cbCardCount = 1;
        OutCardResult.cbResultCard[0] = cbHandCardData[cbHandCardCount - 1];
        return;
    }

//地主下家（后出牌）
    void CLandLogic::UndersideOfBankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint16_t wOutCardUser,
                                              const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount,
                                              tagOutCardResult &OutCardResult) {
        //初始变量
        memset(&OutCardResult, 0, sizeof(OutCardResult));

        //零下标没用
        tagOutCardTypeResult CardTypeResult[12 + 1];
        memset(CardTypeResult, 0, sizeof(CardTypeResult));

        //出牌类型
        uint8_t cbOutCardType = GetCardType(cbTurnCardData, cbTurnCardCount);

        //搜索可出牌
        tagOutCardTypeResult BankerOutCardTypeResult[13];
        memset(BankerOutCardTypeResult, 0, sizeof(BankerOutCardTypeResult));

        AnalyseOutCardType(m_cbAllCardData[m_wBankerUser], m_cbUserCardCount[m_wBankerUser], BankerOutCardTypeResult);
        AnalyseOutCardType(cbHandCardData, cbHandCardCount, cbTurnCardData, cbTurnCardCount, CardTypeResult);

        //只剩炸弹
        if (cbHandCardCount == CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0])
        {
            OutCardResult.cbCardCount = CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[0];
            memcpy(OutCardResult.cbResultCard, CardTypeResult[CT_BOMB_CARD].cbCardData, OutCardResult.cbCardCount);

            return;
        }
            //双王炸弹和一手
        else if (cbHandCardCount > 2 && cbHandCardData[0] == 0x4f && cbHandCardData[1] == 0x4e &&
                 CT_ERROR != GetCardType(cbHandCardData + 2, cbHandCardCount - 2))
        {
            OutCardResult.cbCardCount = 2;
            OutCardResult.cbResultCard[0] = 0x4f;
            OutCardResult.cbResultCard[1] = 0x4e;
            return;
        }

        //取出四个最小单牌
        uint8_t cbMinSingleCardCount[4];
        cbMinSingleCardCount[0] = MAX_LAND_COUNT;
        cbMinSingleCardCount[1] = MAX_LAND_COUNT;
        cbMinSingleCardCount[2] = MAX_LAND_COUNT;
        cbMinSingleCardCount[3] = MAX_LAND_COUNT;
        uint8_t cbIndex[4] = {0};
        uint8_t cbMinSingleCountInFour = MAX_LAND_COUNT;

        //可出扑克（这里已经过滤掉炸弹了）
        tagOutCardTypeResult const &CanOutCard = CardTypeResult[cbOutCardType];

        for (uint8_t i = 0; i < CanOutCard.cbCardTypeCount; ++i)
        {
            //最小单牌
            uint8_t cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, CanOutCard.cbCardData[i],
                                                       CanOutCard.cbEachHandCardCount[i]);
            uint8_t cbMaxValue = 0;
            uint8_t Index = 0;

            //搜索cbMinSingleCardCount[4]的最大值
            for (uint8_t j = 0; j < 4; ++j)
            {
                if (cbMinSingleCardCount[j] >= cbTmpCount)
                {
                    cbMinSingleCardCount[j] = cbTmpCount;
                    cbIndex[j] = i;
                    break;
                }
            }

        }

        for (uint8_t i = 0; i < 4; ++i)
            if (cbMinSingleCountInFour > cbMinSingleCardCount[i]) cbMinSingleCountInFour = cbMinSingleCardCount[i];


        //原始单牌数
        uint8_t cbOriginSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount, NULL, 0);

        //朋友出牌
        bool bFriendOut = m_wBankerUser != wOutCardUser;
        if (bFriendOut)
        {
            if (CanOutCard.cbCardTypeCount > 0)
            {
                uint8_t cbMinLogicCardValue = GetCardLogicValue(0x4F) + 1;
                bool bFindCard = false;
                uint8_t cbCanOutIndex = 0;
                for (uint8_t i = 0; i < 4; ++i)
                {
                    uint8_t Index = cbIndex[i];
                    //小于J的牌，或者小于K而且是散牌
                    if ((cbMinSingleCardCount[i] < cbOriginSingleCardCount + 4 &&
                         cbMinSingleCardCount[i] <= cbMinSingleCountInFour &&
                         (GetCardLogicValue(CanOutCard.cbCardData[Index][0]) <= 11 ||
                          ((cbMinSingleCardCount[i] < cbOriginSingleCardCount) &&
                           GetCardLogicValue(CanOutCard.cbCardData[Index][0]) <= 13))) &&
                        cbMinLogicCardValue > GetCardLogicValue(CanOutCard.cbCardData[Index][0]) &&
                        cbHandCardCount > 5)
                    {
                        //搜索有没有大牌（针对飞机带翅膀后面的带牌）
                        bool bNoLargeCard = true;
                        for (uint8_t k = 3; k < CanOutCard.cbEachHandCardCount[Index]; ++k)
                        {
                            //有大牌而且不能一次出完
                            if (GetCardLogicValue(CanOutCard.cbCardData[Index][k]) >= 15 &&
                                CanOutCard.cbEachHandCardCount[Index] != cbHandCardCount)
                                bNoLargeCard = false;
                        }
                        if (bNoLargeCard)
                        {
                            bFindCard = true;
                            cbCanOutIndex = Index;
                            cbMinLogicCardValue = GetCardLogicValue(CanOutCard.cbCardData[Index][0]);
                        }
                    }
                    else if (cbHandCardCount < 5 && cbMinSingleCardCount[i] < cbOriginSingleCardCount + 4 &&
                             cbMinSingleCardCount[i] <= cbMinSingleCountInFour &&
                             cbMinLogicCardValue > GetCardLogicValue(CanOutCard.cbCardData[Index][0]))
                    {
                        bFindCard = true;
                        cbCanOutIndex = Index;
                        cbMinLogicCardValue = GetCardLogicValue(CanOutCard.cbCardData[Index][0]);
                    }
                }

                if (bFindCard)
                {

                    //设置变量
                    OutCardResult.cbCardCount = CanOutCard.cbEachHandCardCount[cbCanOutIndex];
                    memcpy(OutCardResult.cbResultCard, CanOutCard.cbCardData[cbCanOutIndex],
                           CanOutCard.cbEachHandCardCount[cbCanOutIndex] * sizeof(uint8_t));

                    return;
                }
                    //手上少于五张牌
                else if (cbHandCardCount <= 5)
                {
                    uint8_t cbMinLogicCard = GetCardLogicValue(0x4f) + 1;
                    uint8_t cbCanOutIndex = 0;
                    for (uint8_t i = 0; i < 4; ++i)
                        if (cbMinSingleCardCount[i] < MAX_LAND_COUNT &&
                            cbMinSingleCardCount[i] <= cbMinSingleCountInFour &&
                            cbMinLogicCard > GetCardLogicValue(CanOutCard.cbCardData[cbIndex[i]][0]) &&
                            GetCardLogicValue(CanOutCard.cbCardData[cbIndex[i]][0]) <= 14)
                        {
                            cbMinLogicCard = GetCardLogicValue(CanOutCard.cbCardData[cbIndex[i]][0]);
                            cbCanOutIndex = cbIndex[i];
                        }

                    if (cbMinLogicCard != (GetCardLogicValue(0x4f) + 1))
                    {
                        //设置变量
                        OutCardResult.cbCardCount = CanOutCard.cbEachHandCardCount[cbCanOutIndex];
                        memcpy(OutCardResult.cbResultCard, CanOutCard.cbCardData[cbCanOutIndex],
                               CanOutCard.cbEachHandCardCount[cbCanOutIndex] * sizeof(uint8_t));

                        return;
                    }
                }

                return;
            }
            else
            {
                return;
            }

        }
            //地主出牌
        else
        {
            if (CanOutCard.cbCardTypeCount > 0)
            {
                uint8_t cbMinLogicCardValue = GetCardLogicValue(0x4F) + 1;
                bool bFindCard = false;
                uint8_t cbCanOutIndex = 0;
                for (uint8_t i = 0; i < 4; ++i)
                {
                    uint8_t Index = cbIndex[i];

                    if ((cbMinSingleCardCount[i] < cbOriginSingleCardCount + 4) &&
                        cbMinSingleCardCount[i] <= cbMinSingleCountInFour &&
                        cbMinLogicCardValue > GetCardLogicValue(CanOutCard.cbCardData[Index][0]))
                    {
                        //针对大牌
                        bool bNoLargeCard = true;

                        //当地主手上牌数大于4，而且地主出的是小于K的牌而且不是地主手上的最大牌时，不能出2去打
                        if (m_cbUserCardCount[m_wBankerUser] >= 4 && cbHandCardCount >= 5 &&
                            CanOutCard.cbEachHandCardCount[Index] >= 2 &&
                            GetCardLogicValue(CanOutCard.cbCardData[Index][0]) >= 15 &&
                            GetCardLogicValue(cbTurnCardData[0]) < 13 &&
                            GetCardLogicValue(cbTurnCardData[0]) <
                            GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]) &&
                            CanOutCard.cbEachHandCardCount[Index] != cbHandCardCount)
                            bNoLargeCard = false;

                        //搜索有没有大牌（针对飞机带翅膀后面的带牌）
                        for (uint8_t k = 3; k < CanOutCard.cbEachHandCardCount[Index]; ++k)
                        {
                            if (GetCardLogicValue(CanOutCard.cbCardData[Index][k]) >= 15 &&
                                CanOutCard.cbEachHandCardCount[Index] != cbHandCardCount)
                                bNoLargeCard = false;
                        }
                        if (bNoLargeCard)
                        {
                            bFindCard = true;
                            cbCanOutIndex = Index;
                            cbMinLogicCardValue = GetCardLogicValue(CanOutCard.cbCardData[Index][0]);
                        }
                    }
                }

                if (bFindCard)
                {
                    //地主的最大牌
                    uint8_t cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]);
                    bool bCanOut = true;

                    //王只压2
                    if (GetCardLogicValue(cbTurnCardData[0]) < cbLargestLogicCard)
                    {
                        if (GetCardColor(CanOutCard.cbCardData[cbCanOutIndex][0]) == 0x40 &&
                            GetCardLogicValue(cbTurnCardData[0]) <= 14 && cbHandCardCount > 5)
                        {
                            bCanOut = false;
                        }
                    }

                    if (bCanOut)
                    {
                        //设置变量
                        OutCardResult.cbCardCount = CanOutCard.cbEachHandCardCount[cbCanOutIndex];
                        memcpy(OutCardResult.cbResultCard, CanOutCard.cbCardData[cbCanOutIndex],
                               CanOutCard.cbEachHandCardCount[cbCanOutIndex] * sizeof(uint8_t));

                        return;
                    }
                }

                if (cbOutCardType == CT_SINGLE)
                {
                    //地主的最大牌
                    uint8_t cbLargestLogicCard = GetCardLogicValue(m_cbAllCardData[m_wBankerUser][0]);

                    if (GetCardLogicValue(cbTurnCardData[0]) == 14 ||
                        GetCardLogicValue(cbTurnCardData[0]) >= cbLargestLogicCard ||
                        (GetCardLogicValue(cbTurnCardData[0]) < cbLargestLogicCard - 1) ||
                        m_cbUserCardCount[m_wBankerUser] <= 5)
                    {
                        //取一张大于等于2而且要比地主出的牌大的牌，
                        uint8_t cbIndex = MAX_LAND_COUNT;
                        for (uint8_t i = 0; i < cbHandCardCount; ++i)
                            if (GetCardLogicValue(cbHandCardData[i]) > GetCardLogicValue(cbTurnCardData[0]) &&
                                GetCardLogicValue(cbHandCardData[i]) >= 15)
                            {
                                cbIndex = i;
                            }
                        if (cbIndex != MAX_LAND_COUNT)
                        {
                            //设置变量
                            OutCardResult.cbCardCount = 1;
                            OutCardResult.cbResultCard[0] = cbHandCardData[cbIndex];

                            return;
                        }
                    }
                }

                //当朋友不能拦截地主时
                uint16_t wMeChairID = (m_wBankerUser + 1) % GAME_LAND_PLAYER;
                uint16_t wFriendID = (wMeChairID + 1) % GAME_LAND_PLAYER;

                tagOutCardTypeResult FriendCardTypeResult[13];
                memset(FriendCardTypeResult, 0, sizeof(FriendCardTypeResult));
                AnalyseOutCardType(m_cbAllCardData[wFriendID], m_cbUserCardCount[wFriendID], cbTurnCardData,
                                   cbTurnCardCount, FriendCardTypeResult);

                //当朋友不能拦截地主时
                if (m_cbUserCardCount[m_wBankerUser] <= 4 && FriendCardTypeResult[cbOutCardType].cbCardTypeCount == 0
                    &&
                    CardTypeResult[cbOutCardType].cbCardTypeCount > 0)
                {
                    uint8_t cbMinSingleCount = MAX_LAND_COUNT;
                    uint8_t Index = 0;
                    for (uint8_t i = 0; i < CardTypeResult[cbOutCardType].cbCardTypeCount; ++i)
                    {
                        uint8_t cbTmpCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,
                                                                   CardTypeResult[cbOutCardType].cbCardData[i],
                                                                   CardTypeResult[cbOutCardType].cbEachHandCardCount[i]);
                        if (cbMinSingleCount >= cbTmpCount)
                        {
                            cbMinSingleCount = cbTmpCount;
                            Index = i;
                        }
                    }
                    //设置变量
                    OutCardResult.cbCardCount = CardTypeResult[cbOutCardType].cbEachHandCardCount[Index];
                    memcpy(OutCardResult.cbResultCard, CardTypeResult[cbOutCardType].cbCardData[Index],
                           OutCardResult.cbCardCount);

                    return;
                }
            }

            //还要考虑炸弹
            if (CardTypeResult[CT_BOMB_CARD].cbCardTypeCount > 0 && cbHandCardCount <= 10)
            {
                tagOutCardTypeResult const &BomCard = CardTypeResult[CT_BOMB_CARD];
                uint8_t cbMinLogicValue = GetCardLogicValue(BomCard.cbCardData[0][0]);
                uint8_t Index = 0;
                for (uint8_t i = 0; i < BomCard.cbCardTypeCount; ++i)
                {
                    if (cbMinLogicValue > GetCardLogicValue(BomCard.cbCardData[i][0]))
                    {
                        cbMinLogicValue = GetCardLogicValue(BomCard.cbCardData[i][0]);
                        Index = i;
                    }
                }

                //判断出了炸弹后的单牌数
                uint8_t cbSingleCardCount = AnalyseSinleCardCount(cbHandCardData, cbHandCardCount,
                                                                  BomCard.cbCardData[Index],
                                                                  BomCard.cbEachHandCardCount[Index]);
                if (cbSingleCardCount >= 3 ||
                    (cbOutCardType == CT_SINGLE && GetCardLogicValue(cbTurnCardData[0]) < 15))
                    return;

                //设置变量
                OutCardResult.cbCardCount = BomCard.cbEachHandCardCount[Index];
                memcpy(OutCardResult.cbResultCard, BomCard.cbCardData[Index],
                       BomCard.cbEachHandCardCount[Index] * sizeof(uint8_t));

                return;
            }

            return;
        }
        return;
    }

//出牌搜索
    bool CLandLogic::SearchOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, const uint8_t cbTurnCardData[],
                                   uint8_t cbTurnCardCount, uint16_t wOutCardUser, uint16_t wMeChairID,
                                   tagOutCardResult &OutCardResult) {
        //玩家判断
        uint16_t wUndersideOfBanker = (m_wBankerUser + 1) % GAME_LAND_PLAYER;        //地主下家
        uint16_t wUpsideOfBanker = (wUndersideOfBanker + 1) % GAME_LAND_PLAYER;    //地主上家

        //初始变量
        memset(&OutCardResult, 0, sizeof(OutCardResult));

        //先出牌
        if (cbTurnCardCount == 0)
        {
            //地主出牌
            if (wMeChairID == m_wBankerUser) BankerOutCard(cbHandCardData, cbHandCardCount, OutCardResult);
                //地主下家
            else if (wMeChairID == wUndersideOfBanker)
                UndersideOfBankerOutCard(cbHandCardData, cbHandCardCount, wMeChairID, OutCardResult);
                //地主上家
            else if (wMeChairID == wUpsideOfBanker)
                UpsideOfBankerOutCard(cbHandCardData, cbHandCardCount, wMeChairID, OutCardResult);
                //错误 ID
            else
            {
                ASSERT(false);
            }
        }
            //压牌
        else
        {
            //地主出牌
            if (wMeChairID == m_wBankerUser)
                BankerOutCard(cbHandCardData, cbHandCardCount, wOutCardUser, cbTurnCardData, cbTurnCardCount,
                              OutCardResult);
                //地主下家
            else if (wMeChairID == wUndersideOfBanker)
                UndersideOfBankerOutCard(cbHandCardData, cbHandCardCount, wOutCardUser, cbTurnCardData,
                                         cbTurnCardCount,
                                         OutCardResult);
                //地主上家
            else if (wMeChairID == wUpsideOfBanker)
                UpsideOfBankerOutCard(cbHandCardData, cbHandCardCount, wOutCardUser, cbTurnCardData, cbTurnCardCount,
                                      OutCardResult);
                //错误 ID
            else
            {
                ASSERT(false);
            }

        }
        return true;
    }

//叫分判断
    uint8_t CLandLogic::LandScore(uint16_t wMeChairID, uint8_t cbCurrentLandScore) {
        //大牌数目
        uint8_t cbLargeCardCount = 0;
        uint8_t Index = 0;
        while (GetCardLogicValue(m_cbLandScoreCardData[wMeChairID][Index++]) >= 15) ++cbLargeCardCount;

        //单牌个数
        uint8_t cbSingleCardCount = AnalyseSinleCardCount(m_cbLandScoreCardData[wMeChairID],
                                                          sizeof(m_cbLandScoreCardData[wMeChairID]), NULL, 0);

        //叫两分
        if (cbLargeCardCount >= 4 && cbSingleCardCount <= 4) return 2;

        //放弃叫分
        if (cbLargeCardCount <= 2 || cbCurrentLandScore == 1)
        {
            //放弃叫分
            return 0;
        }

        //其他单牌
        uint8_t cbMinSingleCardCount = MAX_LAND_COUNT;
        for (uint16_t wChairID = 0, i = 0; wChairID < GAME_LAND_PLAYER; ++wChairID)
        {
            uint8_t cbTmpSingleCardCount = AnalyseSinleCardCount(m_cbAllCardData[wChairID], NORMAL_COUNT, NULL, 0);
            if (wChairID != wMeChairID && cbTmpSingleCardCount < cbMinSingleCardCount)
                cbTmpSingleCardCount = cbMinSingleCardCount;
        }

        //叫一分
        if (cbLargeCardCount >= 3 && cbSingleCardCount < cbMinSingleCardCount - 3) return 1;

        //放弃叫分
        return 0;
    }

};






