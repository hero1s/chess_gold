//
// Created by toney on 19/9/11.
//
// �������������߼�

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
// ��������Ϸ����
    class CGameLandTable : public CGameCoinTable {
    public:
        CGameLandTable(std::shared_ptr<CGameRoom> pRoom, int64_t tableID);

        virtual ~CGameLandTable();

        virtual void GetTableFaceInfo(net::table_info *pInfo);

    public:
        //��������
        virtual bool Init();

        //��λ����
        virtual void ResetTable();

        virtual void OnTimeTick();

        //��Ϸ��Ϣ
        virtual int OnGameMessage(std::shared_ptr<CGamePlayer> pPlayer, uint16_t cmdID, const uint8_t *pkt_buf, uint16_t buf_len);

    public:
        // ��Ϸ��ʼ
        virtual bool OnGameStart();

        // ��Ϸ����
        virtual bool OnGameEnd(uint16_t chairID, uint8_t reason);

        // ���ͳ�����Ϣ(��������)
        virtual void SendGameScene(std::shared_ptr<CGamePlayer> pPlayer);

    public:
        // ���������˿���
        void SendHandleCard(uint16_t chairID);

        // �û�����
        bool OnUserPassCard(uint16_t chairID);

        // �û��з�
        bool OnUserCallScore(uint16_t chairID, uint8_t score);

        void OnCallScoreTimeOut();

        // �û�����
        bool OnUserOutCard(uint16_t chairID, uint8_t cardData[], uint8_t cardCount);

        void OnOutCardTimeOut();

        // �йܳ���
        void OnUserAutoCard();

        // ����ʱ����е���ʱ��
        uint32_t GetCallScoreTime();

        uint32_t GetOutCardTime();

        // ��Ϸ���¿�ʼ
        void ReGameStart();

        // ������Ϸ����
        void ResetGameData();

    protected:
        // �����˿�
        void ReInitPoker();

        // ϴ��
        void ShuffleTableCard();

        // ���Ʋ����ص�ǰ���λ��
        uint16_t DispatchUserCard(uint16_t startUser, uint32_t cardIndex);

        // AI ����
        //��Ϸ��ʼ
        bool OnSubGameStart();

        //ׯ����Ϣ
        bool OnSubBankerInfo();

        //�û�����
        bool OnSubOutCard();

        bool OnRobotCallScore();

        bool OnRobotOutCard();

        //��Ϸ����
    protected:
        uint16_t m_firstUser;                                         // �׽��û�
        uint16_t m_bankerUser;                                        // ׯ���û�
        uint16_t m_curUser;                                           // ��ǰ�û�
        uint8_t m_outCardCount[GAME_LAND_PLAYER];                     // ���ƴ���
        uint8_t m_firstUserType;                                      // ���Ʒ�ʽ
        uint16_t m_maxBeiShu;                                         // �����

        // ը����Ϣ
    protected:
        uint8_t m_bombCount;                                          // ը������
        uint8_t m_eachBombCount[GAME_LAND_PLAYER];                    // ը������
        // �з���Ϣ
    protected:
        uint8_t m_callScore[GAME_LAND_PLAYER];                        // �з���
        uint8_t m_callCount;                                          // �з����ִ���
        // ������Ϣ
    protected:
        uint16_t m_turnWiner;                                         // ʤ�����
        uint8_t m_turnCardCount;                                      // ������Ŀ
        uint8_t m_turnCardData[MAX_LAND_COUNT];                       // ��������
        // �˿���Ϣ
    protected:
        uint8_t m_bankerCard[3];                                      // ��Ϸ����
        uint8_t m_handCardCount[GAME_LAND_PLAYER];                    // �˿���Ŀ
        uint8_t m_handCardData[GAME_LAND_PLAYER][MAX_LAND_COUNT];     // �����˿�

    protected:
        CLandLogic m_gameLogic;                                       // ��Ϸ�߼�
        uint8_t m_randCard[FULL_POKER_COUNT];                         // ϴ������

    };

};
