
#pragma once

#include "svrlib.h"
#include "poker/poker_logic.h"

using namespace std;
using namespace svrlib;

namespace game_kuaipao {

    class CGameKuaipaoTable;

//��Ŀ����
    static const int GAME_KUAIPAO_PLAYER = 3;                           // ������Ϸ����
    static const int MAX_KUAIPAO_COUNT = 16;                            // �����Ŀ
    static const int MAX_COUNT = 16;                                    // �����Ŀ

//�׳�����
    enum emFristOutType {
        emFristOutType_BankerOne = 0,        // �׾ֺ���3��ׯ
        emFristOutType_BankerAll = 1,        // ÿ�ֺ���3��ׯ
        emFristOutType_BankerRandOne = 2,    // ���������ׯ
        emFristOutType_BankerMin = 3,        // ��С�����ȳ�
        emFristOutType_BankerRandAll = 4,    // ÿ�������ׯ
    };

//�߼�����	  //�˿�����
    enum emLAND_CARD_TYPE {
        CT_ERROR = 0,                               //��������
        CT_SINGLE = 1,                              //��������
        CT_SINGLE_LINE = 2,                         //��������
        CT_DOUBLE_LINE = 3,                         //��������
        CT_THREE_LINE = 4,                          //��������
        CT_THREE_LINE_TAKE_SINGLE = 5,              //����һ��
        CT_THREE_LINE_TAKE_DOUBLE = 6,              //����һ��
        CT_BOMB = 7,                                //ը������
        CT_FOUR_TAKE_THREE = 8,                     //�Ĵ���
        CT_FOUR_TAKE_TWO = 9,                       //�Ĵ���

    };

//��������
#define ST_ORDER                    1                                    //��С����
#define ST_COUNT                    2                                    //��Ŀ����
#define ST_CUSTOM                   3                                    //�Զ�����

//////////////////////////////////////////////////////////////////////////////////

//�����ṹ
    struct tagAnalyseResult {
        uint8_t bFourCount;                         //������Ŀ
        uint8_t bThreeCount;                        //������Ŀ
        uint8_t bDoubleCount;                       //������Ŀ
        uint8_t bSignedCount;                       //������Ŀ
        uint8_t bFourLogicVolue[4];                 //�����б�
        uint8_t bThreeLogicVolue[5];                //�����б�
        uint8_t bDoubleLogicVolue[8];               //�����б�
        uint8_t bSignedLogicVolue[16];              //�����б�
        uint8_t bFourCardData[MAX_COUNT];           //�����б�
        uint8_t bThreeCardData[MAX_COUNT];          //�����б�
        uint8_t bDoubleCardData[MAX_COUNT];         //�����б�
        uint8_t bSignedCardData[MAX_COUNT];         //������Ŀ
    };

//���ƽ��
    struct tagOutCardResult {
        uint8_t cbCardCount;                        //�˿���Ŀ
        uint8_t cbResultCard[MAX_COUNT];            //����˿�
    };

//////////////////////////////////////////////////////////////////////////////////

//��Ϸ�߼���
    class CKuaipaoLogic : public CPokerLogic {
    protected:
        CGameKuaipaoTable *m_pHostTable;            //�齫����

        //��������
    public:

        //���캯��
        CKuaipaoLogic();

        //��������
        virtual ~CKuaipaoLogic();

        void SetKuaiPaoTable(CGameKuaipaoTable *pTable);
        //���ͺ���
    public:

        //��ȡ����
        uint8_t GetCardType(uint8_t bCardData[], uint8_t bCardCount, bool lastHand);
        //���ƺ���
    public:

        //��ó�ʼ�˿�
        void GetInitCardList(uint8_t cardNum, vector<uint8_t> &poolCards);

        //�߼�����
    public:

        //�Ա��˿�
        int CompareCard(uint8_t bFirstList[], uint8_t bNextList[], uint8_t bFirstCount, uint8_t bNextCount, bool firstLastHand,
                        bool nextlastHand);

        //�����˿�
        void AnalysebCardData(const uint8_t bCardData[], uint8_t bCardCount, tagAnalyseResult &AnalyseResult);

        //ը��������
        void SpiltBombToThree(tagAnalyseResult &AnalyseResult);

        //��ȡ��������������ʼֵ
        void GetThreeLine(tagAnalyseResult &AnalyseResult, uint8_t &lines, uint8_t &value);

        //�����ж�
        bool SearchOutCard(uint8_t bCardData[], uint8_t bCardCount, uint8_t bTurnCardData[], uint8_t bTurnCardCount,
                           tagOutCardResult &OutCardResult, bool lastHand);

        //���������ж�
        bool SearchActionOutCard(uint8_t bCardData[], uint8_t bCardCount, uint16_t nextPlayerCardNum,
                                 tagOutCardResult &OutCardResult);

        //������������
        bool SearchSingleLineOutCard(uint8_t bCardData[], uint8_t bCardCount, tagOutCardResult &OutCardResult);

        //��������
        bool SearchDoubleLineOutCard(uint8_t bCardData[], uint8_t bCardCount,uint8_t lineNum, tagOutCardResult &OutCardResult);

        //����������ɻ�
        bool SearchThreeTakeTwoOutCard(uint8_t bCardData[], uint8_t bCardCount,uint8_t lineNum,uint8_t takeNum, tagOutCardResult &OutCardResult);

        //����ը�������
        bool SearchBombOutCard(uint8_t bCardData[], uint8_t bCardCount,uint8_t bSameNum, tagOutCardResult &OutCardResult);

    };
};

