//
// Created by toney on 16/4/19.
//

#pragma once

#include "svrlib.h"

// 扑克数值掩码
#define    MASK_COLOR        0xF0    //花色掩码
#define    MASK_VALUE        0x0F    //数值掩码

#define    FULL_POKER_COUNT  54      // 一副扑克牌数目
#define    INVALID_CHAIR     0xFF    //

class CPokerLogic {
public:
    CPokerLogic();

    virtual ~CPokerLogic();

    //获取扑克数值
    virtual uint8_t GetCardValue(uint8_t cbCardData);

    //获取扑克花色
    virtual uint8_t GetCardColor(uint8_t cbCardData);

    //获取扑克花色值
    virtual uint8_t GetCardColorValue(uint8_t cbCardData);

    //逻辑数值(可能每个游戏不一样按需求重载)
    virtual uint8_t GetCardLogicValue(uint8_t cbCardData);

    //有效判断
    virtual bool IsValidCard(uint8_t cbCardData);

    //构造扑克
    virtual uint8_t MakeCardData(uint8_t cbValueIndex, uint8_t cbColorIndex);

    //按牌值排列扑克
    virtual void SortCardListByLogicValue(uint8_t bCardData[], uint8_t bCardCount);

    virtual void SortCardListByCardValue(uint8_t bCardData[], uint8_t bCardCount);

    //按牌数量排序
    virtual void SortCardListByCardNum(uint8_t bCardData[], uint8_t bCardCount);

    //随机取牌
    virtual void GetRandCardList(const uint8_t cbSrcCard[], uint8_t bSrcCount, uint8_t cbTarCard[], uint8_t bTarCount);

    //获取牌数量
    virtual uint16_t CountCardNum(const uint8_t bCardData[], const uint8_t bCardCount, uint8_t card);

    //删除扑克
    virtual bool RemoveCardList(const uint8_t cbRemoveCard[], uint8_t cbRemoveCount, uint8_t cbCardData[], uint8_t cbCardCount);

    virtual bool RemoveCardList(const vector<uint8_t> &cbRemoveCard, vector<uint8_t> &cbCardData);

    //获取一副扑克牌
    virtual void AddFullPokerCard(vector<uint8_t> &poolCards);

    //扑克牌字符串表示
    string ChangePokerCardToString(uint8_t card);

    uint8_t ChangePokerCardFromString(string card);
protected:
    static const uint8_t m_cbFullCardData[FULL_POKER_COUNT];  //一副扑克数据


};






















