
#pragma once

#include "svrlib.h"
#include "poker/poker_logic.h"

using namespace std;
using namespace svrlib;

namespace game_land {

//数目定义
    static const int GAME_LAND_PLAYER = 3;                                    // 斗地主游戏人数
    static const int MAX_LAND_COUNT = 20;                                     // 最大数目

//逻辑数目
    static const int NORMAL_COUNT = 17;                                       //常规数目
    static const int DISPATCH_COUNT = 51;                                     //派发数目
    static const int GOOD_CARD_COUTN = 38;                                    //好牌数目

    static const int MAX_TYPE_COUNT = 254;

//逻辑类型
    enum emLAND_CARD_TYPE {
        CT_ERROR = 0,                                 //错误类型
        CT_SINGLE,                                    //单牌类型
        CT_DOUBLE,                                    //对牌类型
        CT_THREE,                                     //三条类型
        CT_SINGLE_LINE,                               //单连类型
        CT_DOUBLE_LINE,                               //对连类型
        CT_THREE_LINE,                                //三连类型
        CT_THREE_TAKE_ONE,                            //三带一单
        CT_THREE_TAKE_TWO,                            //三带一对
        CT_FOUR_TAKE_ONE,                             //四带两单
        CT_FOUR_TAKE_TWO,                             //四带两对
        CT_BOMB_CARD,                                 //炸弹类型
        CT_MISSILE_CARD,                              //火箭类型
    };

//排序类型
#define ST_ORDER                    1                                    //大小排序
#define ST_COUNT                    2                                    //数目排序
#define ST_CUSTOM                   3                                    //自定排序

//////////////////////////////////////////////////////////////////////////////////

//分析结构
    struct tagAnalyseResult {
        uint8_t cbBlockCount[4];                      //扑克数目
        uint8_t cbCardData[4][MAX_LAND_COUNT];        //扑克数据
    };

//出牌结果
    struct tagOutCardResult {
        uint8_t cbCardCount;                          //扑克数目
        uint8_t cbResultCard[MAX_LAND_COUNT];         //结果扑克
    };

//分布信息
    struct tagDistributing {
        uint8_t cbCardCount;                          //扑克数目
        uint8_t cbDistributing[15][6];                //分布信息
    };

//搜索结果
    struct tagSearchCardResult {
        uint8_t cbSearchCount;                                    //结果数目
        uint8_t cbCardCount[MAX_LAND_COUNT];                      //扑克数目
        uint8_t cbResultCard[MAX_LAND_COUNT][MAX_LAND_COUNT];     //结果扑克
    };

    struct tagOutCardTypeResult {
        uint8_t cbCardType;                                   //扑克类型
        uint8_t cbCardTypeCount;                              //牌型数目
        uint8_t cbEachHandCardCount[MAX_TYPE_COUNT];          //每手个数
        uint8_t cbCardData[MAX_TYPE_COUNT][MAX_LAND_COUNT];   //扑克数据
    };

//////////////////////////////////////////////////////////////////////////////////

//游戏逻辑类
    class CLandLogic : public CPokerLogic {
        //AI变量
    public:
        uint8_t m_cbAllCardData[GAME_LAND_PLAYER][MAX_LAND_COUNT];                      //所有扑克
        uint8_t m_cbLandScoreCardData[GAME_LAND_PLAYER][MAX_LAND_COUNT];                //叫牌扑克
        uint8_t m_cbUserCardCount[GAME_LAND_PLAYER];                                    //扑克数目
        uint16_t m_wBankerUser;                                                         //地主玩家

        //函数定义
    public:
        //构造函数
        CLandLogic();

        //析构函数
        virtual ~CLandLogic();

        //类型函数
    public:
        //获取类型
        uint8_t GetCardType(const uint8_t cbCardData[], uint8_t cbCardCount);
        //控制函数
    public:
        //混乱扑克
        void RandCardList(uint8_t cbCardBuffer[], uint8_t cbBufferCount);

        //排列扑克
        void SortCardList(uint8_t cbCardData[], uint8_t cbCardCount, uint8_t cbSortType);

        //逻辑函数
    public:
        //对比扑克
        bool CompareCard(const uint8_t cbFirstCard[], const uint8_t cbNextCard[], uint8_t cbFirstCount, uint8_t cbNextCount);

        //出牌搜索
        bool SearchOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, const uint8_t cbTurnCardData[],
                           uint8_t cbTurnCardCount, tagOutCardResult &OutCardResult);
        //内部函数
    public:
        //分析扑克
        void AnalysebCardData(const uint8_t cbCardData[], uint8_t cbCardCount, tagAnalyseResult &AnalyseResult);

        //分析分布
        void AnalysebDistributing(const uint8_t cbCardData[], uint8_t cbCardCount, tagDistributing &Distributing);

        //AI函数
    public:
        //设置扑克
        void SetUserCard(uint16_t wChairID, uint8_t cbCardData[], uint8_t cbCardCount);

        //设置底牌
        void SetBackCard(uint16_t wChairID, uint8_t cbBackCardData[], uint8_t cbCardCount);

        //设置庄家
        void SetBanker(uint16_t wBanker);

        //叫牌扑克
        void SetLandScoreCardData(uint16_t wChairID, uint8_t cbCardData[], uint8_t cbCardCount);

        //删除扑克
        void RemoveUserCardData(uint16_t wChairID, uint8_t cbRemoveCardData[], uint8_t cbRemoveCardCount);

        //辅助函数
    public:
        //组合算法
        void Combination(uint8_t cbCombineCardData[], uint8_t cbResComLen, uint8_t cbResultCardData[254][5], uint8_t &cbResCardLen,
                         uint8_t cbSrcCardData[], uint8_t cbCombineLen1, uint8_t cbSrcLen, const uint8_t cbCombineLen2);

        //排列算法
        void Permutation(uint8_t *list, int m, int n, uint8_t result[][4], uint8_t &len);

        //分析炸弹
        void GetAllBomCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbBomCardData[],
                           uint8_t &cbBomCardCount);

        //分析顺子
        void GetAllLineCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbLineCardData[],
                            uint8_t &cbLineCardCount);

        //分析三条
        void GetAllThreeCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbThreeCardData[],
                             uint8_t &cbThreeCardCount);

        //分析对子
        void GetAllDoubleCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbDoubleCardData[],
                              uint8_t &cbDoubleCardCount);

        //分析单牌
        void GetAllSingleCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbSingleCardData[],
                              uint8_t &cbSingleCardCount);

        //主要函数
    public:
        //分析牌型（后出牌调用）
        void AnalyseOutCardType(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount,
                                uint8_t const cbTurnCardData[],
                                uint8_t const cbTurnCardCount, tagOutCardTypeResult CardTypeResult[12 + 1]);

        //分析牌牌（先出牌调用）
        void AnalyseOutCardType(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount,
                                tagOutCardTypeResult CardTypeResult[12 + 1]);

        //单牌个数
        uint8_t
        AnalyseSinleCardCount(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount,
                              uint8_t const cbWantOutCardData[],
                              uint8_t const cbWantOutCardCount, uint8_t cbSingleCardData[] = NULL);

        //出牌函数
    public:
        //地主出牌（先出牌）
        void BankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, tagOutCardResult &OutCardResult);

        //地主出牌（后出牌）
        void
        BankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint16_t wOutCardUser,
                      const uint8_t cbTurnCardData[],
                      uint8_t cbTurnCardCount, tagOutCardResult &OutCardResult);

        //地主上家（先出牌）
        void UpsideOfBankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint16_t wMeChairID,
                                   tagOutCardResult &OutCardResult);

        //地主上家（后出牌）
        void UpsideOfBankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint16_t wOutCardUser,
                                   const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount,
                                   tagOutCardResult &OutCardResult);

        //地主下家（先出牌）
        void UndersideOfBankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint16_t wMeChairID,
                                      tagOutCardResult &OutCardResult);

        //地主下家（后出牌）
        void UndersideOfBankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint16_t wOutCardUser,
                                      const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount,
                                      tagOutCardResult &OutCardResult);

        //出牌搜索
        bool SearchOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, const uint8_t cbTurnCardData[],
                           uint8_t cbTurnCardCount, uint16_t wOutCardUser, uint16_t wMeChairID,
                           tagOutCardResult &OutCardResult);

        //叫分函数
    public:
        //叫分判断
        uint8_t LandScore(uint16_t wMeChairID, uint8_t cbCurrentLandScore);

    };
};



