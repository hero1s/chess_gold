
#pragma once

#include "svrlib.h"
#include "poker/poker_logic.h"

using namespace std;
using namespace svrlib;

namespace game_kuaipao {

    class CGameKuaipaoTable;

//数目定义
    static const int GAME_KUAIPAO_PLAYER = 3;                           // 快跑游戏人数
    static const int MAX_KUAIPAO_COUNT = 16;                            // 最大数目
    static const int MAX_COUNT = 16;                                    // 最大数目

//首出类型
    enum emFristOutType {
        emFristOutType_BankerOne = 0,        // 首局黑桃3定庄
        emFristOutType_BankerAll = 1,        // 每局黑桃3定庄
        emFristOutType_BankerRandOne = 2,    // 首轮随机定庄
        emFristOutType_BankerMin = 3,        // 最小手牌先出
        emFristOutType_BankerRandAll = 4,    // 每局随机定庄
    };

//逻辑类型	  //扑克类型
    enum emLAND_CARD_TYPE {
        CT_ERROR = 0,                               //错误类型
        CT_SINGLE = 1,                              //单牌类型
        CT_SINGLE_LINE = 2,                         //单连类型
        CT_DOUBLE_LINE = 3,                         //对连类型
        CT_THREE_LINE = 4,                          //三连类型
        CT_THREE_LINE_TAKE_SINGLE = 5,              //三带一单
        CT_THREE_LINE_TAKE_DOUBLE = 6,              //三带一对
        CT_BOMB = 7,                                //炸弹类型
        CT_FOUR_TAKE_THREE = 8,                     //四带三
        CT_FOUR_TAKE_TWO = 9,                       //四带二

    };

//排序类型
#define ST_ORDER                    1                                    //大小排序
#define ST_COUNT                    2                                    //数目排序
#define ST_CUSTOM                   3                                    //自定排序

//////////////////////////////////////////////////////////////////////////////////

//分析结构
    struct tagAnalyseResult {
        uint8_t bFourCount;                         //四张数目
        uint8_t bThreeCount;                        //三张数目
        uint8_t bDoubleCount;                       //两张数目
        uint8_t bSignedCount;                       //单张数目
        uint8_t bFourLogicVolue[4];                 //四张列表
        uint8_t bThreeLogicVolue[5];                //三张列表
        uint8_t bDoubleLogicVolue[8];               //两张列表
        uint8_t bSignedLogicVolue[16];              //单张列表
        uint8_t bFourCardData[MAX_COUNT];           //四张列表
        uint8_t bThreeCardData[MAX_COUNT];          //三张列表
        uint8_t bDoubleCardData[MAX_COUNT];         //两张列表
        uint8_t bSignedCardData[MAX_COUNT];         //单张数目
    };

//出牌结果
    struct tagOutCardResult {
        uint8_t cbCardCount;                        //扑克数目
        uint8_t cbResultCard[MAX_COUNT];            //结果扑克
    };

//////////////////////////////////////////////////////////////////////////////////

//游戏逻辑类
    class CKuaipaoLogic : public CPokerLogic {
    protected:
        CGameKuaipaoTable *m_pHostTable;            //麻将桌子

        //函数定义
    public:

        //构造函数
        CKuaipaoLogic();

        //析构函数
        virtual ~CKuaipaoLogic();

        void SetKuaiPaoTable(CGameKuaipaoTable *pTable);
        //类型函数
    public:

        //获取类型
        uint8_t GetCardType(uint8_t bCardData[], uint8_t bCardCount, bool lastHand);
        //控制函数
    public:

        //获得初始扑克
        void GetInitCardList(uint8_t cardNum, vector<uint8_t> &poolCards);

        //逻辑函数
    public:

        //对比扑克
        int CompareCard(uint8_t bFirstList[], uint8_t bNextList[], uint8_t bFirstCount, uint8_t bNextCount, bool firstLastHand,
                        bool nextlastHand);

        //分析扑克
        void AnalysebCardData(const uint8_t bCardData[], uint8_t bCardCount, tagAnalyseResult &AnalyseResult);

        //炸弹拆三张
        void SpiltBombToThree(tagAnalyseResult &AnalyseResult);

        //获取三连的数量及起始值
        void GetThreeLine(tagAnalyseResult &AnalyseResult, uint8_t &lines, uint8_t &value);

        //出牌判断
        bool SearchOutCard(uint8_t bCardData[], uint8_t bCardCount, uint8_t bTurnCardData[], uint8_t bTurnCardCount,
                           tagOutCardResult &OutCardResult, bool lastHand);

        //主动出牌判断
        bool SearchActionOutCard(uint8_t bCardData[], uint8_t bCardCount, uint16_t nextPlayerCardNum,
                                 tagOutCardResult &OutCardResult);

        //搜索单连出牌
        bool SearchSingleLineOutCard(uint8_t bCardData[], uint8_t bCardCount, tagOutCardResult &OutCardResult);

        //搜索连对
        bool SearchDoubleLineOutCard(uint8_t bCardData[], uint8_t bCardCount,uint8_t lineNum, tagOutCardResult &OutCardResult);

        //搜索三带或飞机
        bool SearchThreeTakeTwoOutCard(uint8_t bCardData[], uint8_t bCardCount,uint8_t lineNum,uint8_t takeNum, tagOutCardResult &OutCardResult);

        //搜索炸弹或对子
        bool SearchBombOutCard(uint8_t bCardData[], uint8_t bCardCount,uint8_t bSameNum, tagOutCardResult &OutCardResult);

    };
};

