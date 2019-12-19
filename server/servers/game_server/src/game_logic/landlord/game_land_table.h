//
// Created by toney on 19/9/11.
//
// 斗地主的桌子逻辑

#pragma once

#include "game_table/game_table_coin.h"
#include "game_player.h"
#include "svrlib.h"
#include "msg_define.pb.h"
#include "land_logic.h"

using namespace svrlib;
using namespace std;

class CGamePlayer;

class CGameRoom;
namespace game_land {
// 斗地主游戏桌子
    class CGameLandTable : public CGameCoinTable {
    public:
        CGameLandTable(std::shared_ptr<CGameRoom> pRoom, int64_t tableID);

        virtual ~CGameLandTable();

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
        // 发送手中扑克牌
        void SendHandleCard(uint16_t chairID);

        // 用户放弃
        bool OnUserPassCard(uint16_t chairID);

        // 用户叫分
        bool OnUserCallScore(uint16_t chairID, uint8_t score);

        void OnCallScoreTimeOut();

        // 用户出牌
        bool OnUserOutCard(uint16_t chairID, uint8_t cardData[], uint8_t cardCount);

        void OnOutCardTimeOut();

        // 托管出牌
        void OnUserAutoCard();

        // 出牌时间跟叫地主时间
        uint32_t GetCallScoreTime();

        uint32_t GetOutCardTime();

        // 游戏重新开始
        void ReGameStart();

        // 重置游戏数据
        void ResetGameData();

    protected:
        // 重置扑克
        void ReInitPoker();

        // 洗牌
        void ShuffleTableCard();

        // 发牌并返回当前玩家位置
        uint16_t DispatchUserCard(uint16_t startUser, uint32_t cardIndex);

        // AI 函数
        //游戏开始
        bool OnSubGameStart();

        //庄家信息
        bool OnSubBankerInfo();

        //用户出牌
        bool OnSubOutCard();

        bool OnRobotCallScore();

        bool OnRobotOutCard();

        //游戏变量
    protected:
        uint16_t m_firstUser;                                         // 首叫用户
        uint16_t m_bankerUser;                                        // 庄家用户
        uint16_t m_curUser;                                           // 当前用户
        uint8_t m_outCardCount[GAME_LAND_PLAYER];                     // 出牌次数
        uint8_t m_firstUserType;                                      // 叫牌方式
        uint16_t m_maxBeiShu;                                         // 最大倍数

        // 炸弹信息
    protected:
        uint8_t m_bombCount;                                          // 炸弹个数
        uint8_t m_eachBombCount[GAME_LAND_PLAYER];                    // 炸弹个数
        // 叫分信息
    protected:
        uint8_t m_callScore[GAME_LAND_PLAYER];                        // 叫分数
        uint8_t m_callCount;                                          // 叫分流局次数
        // 出牌信息
    protected:
        uint16_t m_turnWiner;                                         // 胜利玩家
        uint8_t m_turnCardCount;                                      // 出牌数目
        uint8_t m_turnCardData[MAX_LAND_COUNT];                       // 出牌数据
        // 扑克信息
    protected:
        uint8_t m_bankerCard[3];                                      // 游戏底牌
        uint8_t m_handCardCount[GAME_LAND_PLAYER];                    // 扑克数目
        uint8_t m_handCardData[GAME_LAND_PLAYER][MAX_LAND_COUNT];     // 手上扑克

    protected:
        CLandLogic m_gameLogic;                                       // 游戏逻辑
        uint8_t m_randCard[FULL_POKER_COUNT];                         // 洗牌数据

    };

};
