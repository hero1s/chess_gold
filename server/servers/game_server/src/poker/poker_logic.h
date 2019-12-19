//
// Created by toney on 16/4/19.
//

#pragma once

#include "svrlib.h"

// �˿���ֵ����
#define    MASK_COLOR        0xF0    //��ɫ����
#define    MASK_VALUE        0x0F    //��ֵ����

#define    FULL_POKER_COUNT  54      // һ���˿�����Ŀ
#define    INVALID_CHAIR     0xFF    //

class CPokerLogic {
public:
    CPokerLogic();

    virtual ~CPokerLogic();

    //��ȡ�˿���ֵ
    virtual uint8_t GetCardValue(uint8_t cbCardData);

    //��ȡ�˿˻�ɫ
    virtual uint8_t GetCardColor(uint8_t cbCardData);

    //��ȡ�˿˻�ɫֵ
    virtual uint8_t GetCardColorValue(uint8_t cbCardData);

    //�߼���ֵ(����ÿ����Ϸ��һ������������)
    virtual uint8_t GetCardLogicValue(uint8_t cbCardData);

    //��Ч�ж�
    virtual bool IsValidCard(uint8_t cbCardData);

    //�����˿�
    virtual uint8_t MakeCardData(uint8_t cbValueIndex, uint8_t cbColorIndex);

    //����ֵ�����˿�
    virtual void SortCardListByLogicValue(uint8_t bCardData[], uint8_t bCardCount);

    virtual void SortCardListByCardValue(uint8_t bCardData[], uint8_t bCardCount);

    //������������
    virtual void SortCardListByCardNum(uint8_t bCardData[], uint8_t bCardCount);

    //���ȡ��
    virtual void GetRandCardList(const uint8_t cbSrcCard[], uint8_t bSrcCount, uint8_t cbTarCard[], uint8_t bTarCount);

    //��ȡ������
    virtual uint16_t CountCardNum(const uint8_t bCardData[], const uint8_t bCardCount, uint8_t card);

    //ɾ���˿�
    virtual bool RemoveCardList(const uint8_t cbRemoveCard[], uint8_t cbRemoveCount, uint8_t cbCardData[], uint8_t cbCardCount);

    virtual bool RemoveCardList(const vector<uint8_t> &cbRemoveCard, vector<uint8_t> &cbCardData);

    //��ȡһ���˿���
    virtual void AddFullPokerCard(vector<uint8_t> &poolCards);

    //�˿����ַ�����ʾ
    string ChangePokerCardToString(uint8_t card);

    uint8_t ChangePokerCardFromString(string card);
protected:
    static const uint8_t m_cbFullCardData[FULL_POKER_COUNT];  //һ���˿�����


};






















