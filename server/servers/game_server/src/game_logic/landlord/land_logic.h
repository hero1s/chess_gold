
#pragma once

#include "svrlib.h"
#include "poker/poker_logic.h"

using namespace std;
using namespace svrlib;

namespace game_land {

//��Ŀ����
    static const int GAME_LAND_PLAYER = 3;                                    // ��������Ϸ����
    static const int MAX_LAND_COUNT = 20;                                     // �����Ŀ

//�߼���Ŀ
    static const int NORMAL_COUNT = 17;                                       //������Ŀ
    static const int DISPATCH_COUNT = 51;                                     //�ɷ���Ŀ
    static const int GOOD_CARD_COUTN = 38;                                    //������Ŀ

    static const int MAX_TYPE_COUNT = 254;

//�߼�����
    enum emLAND_CARD_TYPE {
        CT_ERROR = 0,                                 //��������
        CT_SINGLE,                                    //��������
        CT_DOUBLE,                                    //��������
        CT_THREE,                                     //��������
        CT_SINGLE_LINE,                               //��������
        CT_DOUBLE_LINE,                               //��������
        CT_THREE_LINE,                                //��������
        CT_THREE_TAKE_ONE,                            //����һ��
        CT_THREE_TAKE_TWO,                            //����һ��
        CT_FOUR_TAKE_ONE,                             //�Ĵ�����
        CT_FOUR_TAKE_TWO,                             //�Ĵ�����
        CT_BOMB_CARD,                                 //ը������
        CT_MISSILE_CARD,                              //�������
    };

//��������
#define ST_ORDER                    1                                    //��С����
#define ST_COUNT                    2                                    //��Ŀ����
#define ST_CUSTOM                   3                                    //�Զ�����

//////////////////////////////////////////////////////////////////////////////////

//�����ṹ
    struct tagAnalyseResult {
        uint8_t cbBlockCount[4];                      //�˿���Ŀ
        uint8_t cbCardData[4][MAX_LAND_COUNT];        //�˿�����
    };

//���ƽ��
    struct tagOutCardResult {
        uint8_t cbCardCount;                          //�˿���Ŀ
        uint8_t cbResultCard[MAX_LAND_COUNT];         //����˿�
    };

//�ֲ���Ϣ
    struct tagDistributing {
        uint8_t cbCardCount;                          //�˿���Ŀ
        uint8_t cbDistributing[15][6];                //�ֲ���Ϣ
    };

//�������
    struct tagSearchCardResult {
        uint8_t cbSearchCount;                                    //�����Ŀ
        uint8_t cbCardCount[MAX_LAND_COUNT];                      //�˿���Ŀ
        uint8_t cbResultCard[MAX_LAND_COUNT][MAX_LAND_COUNT];     //����˿�
    };

    struct tagOutCardTypeResult {
        uint8_t cbCardType;                                   //�˿�����
        uint8_t cbCardTypeCount;                              //������Ŀ
        uint8_t cbEachHandCardCount[MAX_TYPE_COUNT];          //ÿ�ָ���
        uint8_t cbCardData[MAX_TYPE_COUNT][MAX_LAND_COUNT];   //�˿�����
    };

//////////////////////////////////////////////////////////////////////////////////

//��Ϸ�߼���
    class CLandLogic : public CPokerLogic {
        //AI����
    public:
        uint8_t m_cbAllCardData[GAME_LAND_PLAYER][MAX_LAND_COUNT];                      //�����˿�
        uint8_t m_cbLandScoreCardData[GAME_LAND_PLAYER][MAX_LAND_COUNT];                //�����˿�
        uint8_t m_cbUserCardCount[GAME_LAND_PLAYER];                                    //�˿���Ŀ
        uint16_t m_wBankerUser;                                                         //�������

        //��������
    public:
        //���캯��
        CLandLogic();

        //��������
        virtual ~CLandLogic();

        //���ͺ���
    public:
        //��ȡ����
        uint8_t GetCardType(const uint8_t cbCardData[], uint8_t cbCardCount);
        //���ƺ���
    public:
        //�����˿�
        void RandCardList(uint8_t cbCardBuffer[], uint8_t cbBufferCount);

        //�����˿�
        void SortCardList(uint8_t cbCardData[], uint8_t cbCardCount, uint8_t cbSortType);

        //�߼�����
    public:
        //�Ա��˿�
        bool CompareCard(const uint8_t cbFirstCard[], const uint8_t cbNextCard[], uint8_t cbFirstCount, uint8_t cbNextCount);

        //��������
        bool SearchOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, const uint8_t cbTurnCardData[],
                           uint8_t cbTurnCardCount, tagOutCardResult &OutCardResult);
        //�ڲ�����
    public:
        //�����˿�
        void AnalysebCardData(const uint8_t cbCardData[], uint8_t cbCardCount, tagAnalyseResult &AnalyseResult);

        //�����ֲ�
        void AnalysebDistributing(const uint8_t cbCardData[], uint8_t cbCardCount, tagDistributing &Distributing);

        //AI����
    public:
        //�����˿�
        void SetUserCard(uint16_t wChairID, uint8_t cbCardData[], uint8_t cbCardCount);

        //���õ���
        void SetBackCard(uint16_t wChairID, uint8_t cbBackCardData[], uint8_t cbCardCount);

        //����ׯ��
        void SetBanker(uint16_t wBanker);

        //�����˿�
        void SetLandScoreCardData(uint16_t wChairID, uint8_t cbCardData[], uint8_t cbCardCount);

        //ɾ���˿�
        void RemoveUserCardData(uint16_t wChairID, uint8_t cbRemoveCardData[], uint8_t cbRemoveCardCount);

        //��������
    public:
        //����㷨
        void Combination(uint8_t cbCombineCardData[], uint8_t cbResComLen, uint8_t cbResultCardData[254][5], uint8_t &cbResCardLen,
                         uint8_t cbSrcCardData[], uint8_t cbCombineLen1, uint8_t cbSrcLen, const uint8_t cbCombineLen2);

        //�����㷨
        void Permutation(uint8_t *list, int m, int n, uint8_t result[][4], uint8_t &len);

        //����ը��
        void GetAllBomCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbBomCardData[],
                           uint8_t &cbBomCardCount);

        //����˳��
        void GetAllLineCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbLineCardData[],
                            uint8_t &cbLineCardCount);

        //��������
        void GetAllThreeCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbThreeCardData[],
                             uint8_t &cbThreeCardCount);

        //��������
        void GetAllDoubleCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbDoubleCardData[],
                              uint8_t &cbDoubleCardCount);

        //��������
        void GetAllSingleCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbSingleCardData[],
                              uint8_t &cbSingleCardCount);

        //��Ҫ����
    public:
        //�������ͣ�����Ƶ��ã�
        void AnalyseOutCardType(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount,
                                uint8_t const cbTurnCardData[],
                                uint8_t const cbTurnCardCount, tagOutCardTypeResult CardTypeResult[12 + 1]);

        //�������ƣ��ȳ��Ƶ��ã�
        void AnalyseOutCardType(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount,
                                tagOutCardTypeResult CardTypeResult[12 + 1]);

        //���Ƹ���
        uint8_t
        AnalyseSinleCardCount(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount,
                              uint8_t const cbWantOutCardData[],
                              uint8_t const cbWantOutCardCount, uint8_t cbSingleCardData[] = NULL);

        //���ƺ���
    public:
        //�������ƣ��ȳ��ƣ�
        void BankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, tagOutCardResult &OutCardResult);

        //�������ƣ�����ƣ�
        void
        BankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint16_t wOutCardUser,
                      const uint8_t cbTurnCardData[],
                      uint8_t cbTurnCardCount, tagOutCardResult &OutCardResult);

        //�����ϼң��ȳ��ƣ�
        void UpsideOfBankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint16_t wMeChairID,
                                   tagOutCardResult &OutCardResult);

        //�����ϼң�����ƣ�
        void UpsideOfBankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint16_t wOutCardUser,
                                   const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount,
                                   tagOutCardResult &OutCardResult);

        //�����¼ң��ȳ��ƣ�
        void UndersideOfBankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint16_t wMeChairID,
                                      tagOutCardResult &OutCardResult);

        //�����¼ң�����ƣ�
        void UndersideOfBankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint16_t wOutCardUser,
                                      const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount,
                                      tagOutCardResult &OutCardResult);

        //��������
        bool SearchOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, const uint8_t cbTurnCardData[],
                           uint8_t cbTurnCardCount, uint16_t wOutCardUser, uint16_t wMeChairID,
                           tagOutCardResult &OutCardResult);

        //�зֺ���
    public:
        //�з��ж�
        uint8_t LandScore(uint16_t wMeChairID, uint8_t cbCurrentLandScore);

    };
};



