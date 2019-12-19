//
// Created by toney on 16/4/19.
//

#include "poker_logic.h"
#include "common_logic.h"

//扑克数据
const uint8_t    CPokerLogic::m_cbFullCardData[FULL_POKER_COUNT] =
        {
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,    //方块 A - K
                0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,    //梅花 A - K
                0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D,    //红桃 A - K
                0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D,    //黑桃 A - K
                0x4E, 0x4F,                                                                      //大小王
        };
const string   s_ClientCardString[FULL_POKER_COUNT] =
        {
                "dA", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "dT", "dJ", "dQ", "dK",    //方块 A - K
                "cA", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "cT", "cJ", "cQ", "cK",    //梅花 A - K
                "hA", "h2", "h3", "h4", "h5", "h6", "h7", "h8", "h9", "hT", "hJ", "hQ", "hK",    //红桃 A - K
                "sA", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "sT", "sJ", "sQ", "sK",    //黑桃 A - K
                "wL", "wB",                                                                      //大小王
        };

CPokerLogic::CPokerLogic() {
}

CPokerLogic::~CPokerLogic() {
}

//获取扑克数值
uint8_t CPokerLogic::GetCardValue(uint8_t cbCardData) {
    return cbCardData & MASK_VALUE;
}

//获取扑克花色
uint8_t CPokerLogic::GetCardColor(uint8_t cbCardData) {
    return cbCardData & MASK_COLOR;
}

//获取扑克花色值
uint8_t CPokerLogic::GetCardColorValue(uint8_t cbCardData) {
    return (cbCardData & MASK_COLOR) >> 4;
}

//逻辑数值
uint8_t CPokerLogic::GetCardLogicValue(uint8_t cbCardData) {
    //扑克属性
    uint8_t cbCardColor = GetCardColor(cbCardData);
    uint8_t cbCardValue = GetCardValue(cbCardData);

    //转换数值
    if (cbCardColor == 0x40) return cbCardValue + 2;
    return (cbCardValue <= 2) ? (cbCardValue + 13) : cbCardValue;
}

//有效判断
bool CPokerLogic::IsValidCard(uint8_t cbCardData) {
    //获取属性
    uint8_t cbCardColor = GetCardColor(cbCardData);
    uint8_t cbCardValue = GetCardValue(cbCardData);
    //有效判断
    if ((cbCardData == 0x4E) || (cbCardData == 0x4F)) return true;
    if ((cbCardColor <= 0x30) && (cbCardValue >= 0x01) && (cbCardValue <= 0x0D)) return true;

    return false;
}

//构造扑克
uint8_t CPokerLogic::MakeCardData(uint8_t cbValueIndex, uint8_t cbColorIndex) {
    return (cbColorIndex << 4) | (cbValueIndex + 1);
}

//按牌值排列扑克
void CPokerLogic::SortCardListByLogicValue(uint8_t bCardData[], uint8_t bCardCount) {
    std::sort(bCardData, bCardData + bCardCount, [&](uint8_t a, uint8_t b)mutable
    {
        uint8_t al = GetCardLogicValue(a);
        uint8_t bl = GetCardLogicValue(b);
        if (al > bl || (al == bl && a > b))
        {
            return true;
        }
        return false;
    });
}

void CPokerLogic::SortCardListByCardValue(uint8_t bCardData[], uint8_t bCardCount) {
    std::sort(bCardData, bCardData + bCardCount, [&](uint8_t a, uint8_t b)mutable
    {
        uint8_t al = GetCardValue(a);
        uint8_t bl = GetCardValue(b);
        if (al > bl || (al == bl && a > b))
        {
            return true;
        }
        return false;
    });
}

//按牌数量排序
void CPokerLogic::SortCardListByCardNum(uint8_t bCardData[], uint8_t bCardCount) {
    vector<uint8_t> cards;
    for (uint8_t i = 0; i < bCardCount; ++i)
    {
        cards.push_back(bCardData[i]);
    }
    std::sort(cards.begin(), cards.end(), [&](uint8_t a, uint8_t b)
    {
        return CountCardNum(bCardData, bCardCount, a) > CountCardNum(bCardData, bCardCount, b);
    });
    for (uint8_t i = 0; i < bCardCount; ++i)
    {
        bCardData[i] = cards[i];
    }
}

//随机取牌
void CPokerLogic::GetRandCardList(const uint8_t cbSrcCard[], uint8_t bSrcCount, uint8_t cbTarCard[], uint8_t bTarCount) {
    vector<uint8_t> tmpData;
    for (uint8_t i = 0; i < bSrcCount; ++i)
    {
        tmpData.push_back(cbSrcCard[i]);
    }
    std::random_shuffle(tmpData.begin(), tmpData.end());
    for (uint8_t i = 0; i < bTarCount && i < bSrcCount; ++i)
    {
        cbTarCard[i] = tmpData[i];
    }
}

uint16_t CPokerLogic::CountCardNum(const uint8_t bCardData[], const uint8_t bCardCount, uint8_t card) {
    uint16_t num = 0;
    for (uint16_t i = 0; i < bCardCount; ++i)
    {
        if (GetCardValue(bCardData[i]) == GetCardValue(card))
        {
            num++;
        }
    }
    return num;
}

//删除扑克
bool CPokerLogic::RemoveCardList(const uint8_t cbRemoveCard[], uint8_t cbRemoveCount, uint8_t cbCardData[], uint8_t cbCardCount) {
    //检验数据
    if (cbRemoveCount > cbCardCount)return false;

    //定义变量
    uint8_t cbDeleteCount = 0;
    vector<uint8_t> cbTempCardData;
    cbTempCardData.resize(cbCardCount);

    memcpy(cbTempCardData.data(), cbCardData, cbCardCount * sizeof(cbCardData[0]));

    //置零扑克
    for (uint8_t i = 0; i < cbRemoveCount; i++)
    {
        for (uint8_t j = 0; j < cbCardCount; j++)
        {
            if (cbRemoveCard[i] == cbTempCardData[j])
            {
                cbDeleteCount++;
                cbTempCardData[j] = 0;
                break;
            }
        }
    }
    if (cbDeleteCount != cbRemoveCount) return false;

    //清理扑克
    uint8_t cbCardPos = 0;
    for (uint8_t i = 0; i < cbCardCount; i++)
    {
        if (cbTempCardData[i] != 0) cbCardData[cbCardPos++] = cbTempCardData[i];
    }

    return true;
}

bool CPokerLogic::RemoveCardList(const vector<uint8_t> &cbRemoveCard, vector<uint8_t> &cbCardData) {
    //检验数据
    if (cbRemoveCard.size() > cbCardData.size())return false;

    //定义变量
    uint8_t cbDeleteCount = 0;
    vector<uint8_t> cbTempCardData = cbCardData;

    //置零扑克
    for (uint8_t i = 0; i < cbRemoveCard.size(); i++)
    {
        for (uint8_t j = 0; j < cbCardData.size(); j++)
        {
            if (cbRemoveCard[i] == cbTempCardData[j])
            {
                cbDeleteCount++;
                cbTempCardData[j] = 0;
                break;
            }
        }
    }
    if (cbDeleteCount != cbRemoveCard.size()) return false;

    //清理扑克
    cbCardData.clear();
    for (uint8_t i = 0; i < cbTempCardData.size(); i++)
    {
        if (cbTempCardData[i] != 0) cbCardData.push_back(cbTempCardData[i]);
    }

    return true;
}

//获取一副扑克牌
void CPokerLogic::AddFullPokerCard(vector<uint8_t> &poolCards) {
    for (int32_t i = 0; i < getArrayLen(m_cbFullCardData); ++i)
    {
        poolCards.push_back(m_cbFullCardData[i]);
    }
}


string CPokerLogic::ChangePokerCardToString(uint8_t card) {
    for(uint8_t i=0;i<FULL_POKER_COUNT;++i){
        if(card == m_cbFullCardData[i])
            return s_ClientCardString[i];
    }
    return "";
}

uint8_t CPokerLogic::ChangePokerCardFromString(string card) {
    for(uint8_t i=0;i<FULL_POKER_COUNT;++i){
        if(card == s_ClientCardString[i])
            return m_cbFullCardData[i];
    }
    return 0;
}










