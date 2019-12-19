//
// Created by toney on 2019/9/13.
//

#pragma once

#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "msg_define.pb.h"
#include "nlohmann/json_wrap.h"

using namespace svrlib;
using namespace std;

class CGamePlayer;

class CGameRoom;

// ��Ϸ��ҳ�����
class CGameCoinTable : public CGameTable, public enable_shared_from_this<CGameCoinTable> {
public:
    CGameCoinTable(std::shared_ptr<CGameRoom> pRoom, int64_t tableID);

    virtual ~CGameCoinTable();

public://���غ���
    virtual bool Init();

    virtual void ShutDown();

    //��λ����
    virtual void ResetTable();

    //��Ϸ��Ϣ
    virtual int OnMessage(std::shared_ptr<CGamePlayer> pPlayer, uint16_t cmdID, const uint8_t *pkt_buf, uint16_t buf_len);

    virtual bool EnterTable(std::shared_ptr<CGamePlayer> pPlayer);

    virtual bool LeaveTable(std::shared_ptr<CGamePlayer> pPlayer, bool bNotify = false, uint8_t leaveType = 0);

    // �ܷ����
    virtual int32_t CanEnterTable(std::shared_ptr<CGamePlayer> pPlayer);

    // �ܷ��뿪
    virtual bool CanLeaveTable(std::shared_ptr<CGamePlayer> pPlayer);

    // �Ƿ������Ϸ
    virtual bool IsJoinPlay(uint16_t chairID);

    // �ܷ�����
    virtual bool CanSitDown(std::shared_ptr<CGamePlayer> pPlayer, uint16_t chairID);

    // �ܷ�վ��
    virtual bool CanStandUp(std::shared_ptr<CGamePlayer> pPlayer);

    // ��Ҫ����
    virtual bool NeedSitDown();

    // ���׼��
    virtual bool PlayerReady(std::shared_ptr<CGamePlayer> pPlayer, bool bReady);

    //�û����߻�����
    virtual bool OnActionUserNetState(std::shared_ptr<CGamePlayer> pPlayer, bool bConnected, bool isJoin = true);

    //�û�����
    virtual bool OnActionUserSitDown(uint16_t wChairID, std::shared_ptr<CGamePlayer> pPlayer);

    //�û�����
    virtual bool OnActionUserStandUp(uint16_t wChairID, std::shared_ptr<CGamePlayer> pPlayer);

    //�û�ͬ��
    virtual bool OnActionUserOnReady(uint16_t wChairID, std::shared_ptr<CGamePlayer> pPlayer);

    //��ҽ�����뿪
    virtual void OnPlayerJoin(bool isJoin, uint16_t chairID, std::shared_ptr<CGamePlayer> pPlayer);

    // �Ƿ���Ҫ����
    virtual bool NeedRecover();

    // ��������
    virtual bool WantNeedRecover();

protected:
    // û���ֵ�����Զ�վ��
    virtual void StandUpNotScore();

    virtual void LeaveAllPlayer();

    // �۳���ʼ̨��
    virtual void DeductStartFee(bool bNeedReady);

    // �۳�����̨��
    virtual void DeducEndFee(uint32_t uid, int64_t &winScore);

    // �ϱ���Ϸս��
    virtual void ReportGameResult(shared_ptr<CGamePlayer> pPlayer, int64_t winScore);

    // ���������Ϣ
    virtual void CalcPlayerGameInfo(uint32_t uid, int64_t winScore);

    virtual int64_t ChangeScoreValueByUID(uint32_t uid, int64_t &score, uint16_t operType, uint16_t subType);

    // һ�ֿ�ʼ
    virtual void OnGameRoundStart();

    // һ�ֽ���
    virtual void OnGameRoundOver();

    virtual void OnGameRoundFlush();

    // ��ʱ�������
    void OnTimerClearPlayer();

protected:
    CCooling m_coolRobot;//������˼��CD
    MemberTimerEvent<CGameCoinTable, &CGameCoinTable::OnTimerClearPlayer> m_timerClear;   // ������Ҷ�ʱ��
};



