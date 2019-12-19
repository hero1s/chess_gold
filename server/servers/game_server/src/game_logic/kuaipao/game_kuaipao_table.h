//
// Created by toney on 16/4/6.
//
// 跑得快的桌子逻辑

#pragma once

#include "game_table/game_table_coin.h"
#include "game_player.h"
#include "svrlib.h"
#include "msg_define.pb.h"
#include "kuaipao_logic_msg.pb.h"
#include "kuaipao_logic.h"

using namespace svrlib;
using namespace std;

class CGamePlayer;

class CGameRoom;
namespace game_kuaipao {
// 跑得快游戏桌子
    class CGameKuaipaoTable : public CGameCoinTable {
    public:
        CGameKuaipaoTable(std::shared_ptr<CGameRoom> pRoom, int64_t tableID);

        virtual ~CGameKuaipaoTable();

        virtual void GetTableFaceInfo(net::table_info *pInfo);

    public:
        //配置桌子
        virtual bool Init();

        //复位桌子
        virtual void ResetTable();

        virtual void OnTimeTick();

        //游戏消息
        virtual int OnGameMessage(std::shared_ptr<CGamePlayer> pPlayer, uint16_t cmdID, const uint8_t *pkt_buf, uint16_t buf_len);

    public:
        // 游戏开始
        virtual bool OnGameStart();

        // 游戏结束
        virtual bool OnGameEnd(uint16_t chairID, uint8_t reason);

        // 发送场景信息(断线重连)
        virtual void SendGameScene(std::shared_ptr<CGamePlayer> pPlayer);

    public:
        // 用户飘分
        bool OnUserPiaoScore(uint16_t chairID, uint8_t score);

        void NotifyPiaoScore(uint16_t chairID);

        void CheckUserPiaoScore();

        // 用户放弃
        bool OnUserPassCard(uint16_t chairID);

        // 用户出牌
        bool OnUserOutCard(uint16_t chairID, uint8_t cardData[], uint8_t cardCount);

        // 操作失败
        void NotifyOperFail(uint16_t chairID, uint8_t reason);

        void OnOutCardTimeOut();

        // 托管出牌
        void OnUserAutoCard();

        // 检测是否要不起
        bool CheckUserPass();

        // 一手接完自动打(没有炸弹)
        void CheckLastHandAutoCard();

        // 出牌时间跟叫地主时间
        uint32_t GetOutCardTime();

        // 重置游戏数据
        void ResetGameData();

        // 洗牌发牌
        void DispatchCard(uint8_t cardNum);

        // 排序手牌
        void FlushSortHandCard();

        // 统计发牌炸弹数并换牌
        void CountDispBomb();

        // 换牌
        void ChangeUserHandCard(uint16_t chairID, uint8_t card);

        // 黑桃3庄家
        uint16_t SetBankerBy3();

        // 最小牌庄家
        uint16_t SetBankerMinCard();

        uint8_t GetMinCard(uint16_t chairID);

        // 是否有黑桃3
        bool IsHaveHei3(uint16_t chairID);

        // 下家手牌数量
        uint16_t GetNextPlayerCardNum(uint16_t chairID);

        bool IsCanStart();

        // 开启飘分
        void StartPiaoScore();

        // 能否少带
        bool IsCanLessOutCard(uint16_t chairID, uint8_t cardNum);

        // 四带
        bool IsCanFourTakeThree();

        bool IsCanFourTakeTwo();

        // 炸弹
        bool IsCanSpiltBomb();

        // 是否飘分
        bool IsOpenPiaoScore();

        // 检查炸弹飘分
        bool CheckBombCalc();

        // 检查炸弹是否拆了
        bool CheckSplitBomb(uint16_t chairID, uint8_t cardData[], uint8_t cardCount);

        //游戏变量
    protected:
        uint8_t m_fristOutType;                         // 首出类型
        uint8_t m_fristHei3;                            // 首出黑桃3
        uint8_t m_playCardNum;                          // 打牌张数(16张15张)
        bool m_hong10FanBei;                            // 红10翻倍
        bool m_threeOutLess;
        bool m_threePassLess;
        bool m_feijiOutLess;
        bool m_feijiPassLess;

        uint8_t m_hong10User;                           // 红10玩家

        bool m_fourTakeThree;                           // 4带3
        bool m_fourTakeTwo;                             // 4带2
        bool m_splitBomb;                               // 炸弹可拆
        bool m_openPiaoScore;                           // 开启飘分

        uint16_t m_bankerUser;                            // 庄家用户
        uint16_t m_curUser;                               // 当前用户
        uint8_t m_outCardCount[GAME_KUAIPAO_PLAYER];     // 出牌次数
        uint8_t m_operCount[GAME_KUAIPAO_PLAYER];        // 操作次数
        uint8_t m_outCardTotal;                          // 出牌总次数
        uint8_t m_playerCount;                           // 玩家人数

        // 炸弹信息
    protected:
        uint8_t m_eachBombCount[GAME_KUAIPAO_PLAYER];  // 炸弹个数
        int64_t m_gameScore[GAME_KUAIPAO_PLAYER];      // 游戏得分
        uint8_t m_piaoScore[GAME_KUAIPAO_PLAYER];      // 飘分

        // 出牌信息
    protected:
        uint16_t m_turnWiner;                             // 胜利玩家
        uint8_t m_turnCardCount;                          // 出牌数目
        uint8_t m_turnCardData[MAX_KUAIPAO_COUNT];        // 出牌数据
        uint8_t m_turnCardType;                           // 出牌牌型
        bool m_autoOutCard;                               // 自动出牌

        // 扑克信息
    protected:
        uint8_t m_handCardCount[GAME_KUAIPAO_PLAYER];      // 扑克数目
        uint8_t m_handCardData[GAME_KUAIPAO_PLAYER][MAX_KUAIPAO_COUNT];   // 手上扑克

    protected:
        CKuaipaoLogic m_gameLogic;                    // 游戏逻辑

    };
};

