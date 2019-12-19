//
// Created by toney on 16/4/6.
//
// �ܵÿ�������߼�

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
// �ܵÿ���Ϸ����
    class CGameKuaipaoTable : public CGameCoinTable {
    public:
        CGameKuaipaoTable(std::shared_ptr<CGameRoom> pRoom, int64_t tableID);

        virtual ~CGameKuaipaoTable();

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
        // �û�Ʈ��
        bool OnUserPiaoScore(uint16_t chairID, uint8_t score);

        void NotifyPiaoScore(uint16_t chairID);

        void CheckUserPiaoScore();

        // �û�����
        bool OnUserPassCard(uint16_t chairID);

        // �û�����
        bool OnUserOutCard(uint16_t chairID, uint8_t cardData[], uint8_t cardCount);

        // ����ʧ��
        void NotifyOperFail(uint16_t chairID, uint8_t reason);

        void OnOutCardTimeOut();

        // �йܳ���
        void OnUserAutoCard();

        // ����Ƿ�Ҫ����
        bool CheckUserPass();

        // һ�ֽ����Զ���(û��ը��)
        void CheckLastHandAutoCard();

        // ����ʱ����е���ʱ��
        uint32_t GetOutCardTime();

        // ������Ϸ����
        void ResetGameData();

        // ϴ�Ʒ���
        void DispatchCard(uint8_t cardNum);

        // ��������
        void FlushSortHandCard();

        // ͳ�Ʒ���ը����������
        void CountDispBomb();

        // ����
        void ChangeUserHandCard(uint16_t chairID, uint8_t card);

        // ����3ׯ��
        uint16_t SetBankerBy3();

        // ��С��ׯ��
        uint16_t SetBankerMinCard();

        uint8_t GetMinCard(uint16_t chairID);

        // �Ƿ��к���3
        bool IsHaveHei3(uint16_t chairID);

        // �¼���������
        uint16_t GetNextPlayerCardNum(uint16_t chairID);

        bool IsCanStart();

        // ����Ʈ��
        void StartPiaoScore();

        // �ܷ��ٴ�
        bool IsCanLessOutCard(uint16_t chairID, uint8_t cardNum);

        // �Ĵ�
        bool IsCanFourTakeThree();

        bool IsCanFourTakeTwo();

        // ը��
        bool IsCanSpiltBomb();

        // �Ƿ�Ʈ��
        bool IsOpenPiaoScore();

        // ���ը��Ʈ��
        bool CheckBombCalc();

        // ���ը���Ƿ����
        bool CheckSplitBomb(uint16_t chairID, uint8_t cardData[], uint8_t cardCount);

        //��Ϸ����
    protected:
        uint8_t m_fristOutType;                         // �׳�����
        uint8_t m_fristHei3;                            // �׳�����3
        uint8_t m_playCardNum;                          // ��������(16��15��)
        bool m_hong10FanBei;                            // ��10����
        bool m_threeOutLess;
        bool m_threePassLess;
        bool m_feijiOutLess;
        bool m_feijiPassLess;

        uint8_t m_hong10User;                           // ��10���

        bool m_fourTakeThree;                           // 4��3
        bool m_fourTakeTwo;                             // 4��2
        bool m_splitBomb;                               // ը���ɲ�
        bool m_openPiaoScore;                           // ����Ʈ��

        uint16_t m_bankerUser;                            // ׯ���û�
        uint16_t m_curUser;                               // ��ǰ�û�
        uint8_t m_outCardCount[GAME_KUAIPAO_PLAYER];     // ���ƴ���
        uint8_t m_operCount[GAME_KUAIPAO_PLAYER];        // ��������
        uint8_t m_outCardTotal;                          // �����ܴ���
        uint8_t m_playerCount;                           // �������

        // ը����Ϣ
    protected:
        uint8_t m_eachBombCount[GAME_KUAIPAO_PLAYER];  // ը������
        int64_t m_gameScore[GAME_KUAIPAO_PLAYER];      // ��Ϸ�÷�
        uint8_t m_piaoScore[GAME_KUAIPAO_PLAYER];      // Ʈ��

        // ������Ϣ
    protected:
        uint16_t m_turnWiner;                             // ʤ�����
        uint8_t m_turnCardCount;                          // ������Ŀ
        uint8_t m_turnCardData[MAX_KUAIPAO_COUNT];        // ��������
        uint8_t m_turnCardType;                           // ��������
        bool m_autoOutCard;                               // �Զ�����

        // �˿���Ϣ
    protected:
        uint8_t m_handCardCount[GAME_KUAIPAO_PLAYER];      // �˿���Ŀ
        uint8_t m_handCardData[GAME_KUAIPAO_PLAYER][MAX_KUAIPAO_COUNT];   // �����˿�

    protected:
        CKuaipaoLogic m_gameLogic;                    // ��Ϸ�߼�

    };
};

