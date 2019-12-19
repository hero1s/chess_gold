
#pragma once

#include "svrlib.h"

// ��һ�����Ϣ
struct stBaseInfo {
    string name;                    // �ǳ�
    uint32_t sex;                   // �Ա�
    int64_t coin;                   // ���
    int64_t safecoin;               // �������ʲ�
    uint32_t vip;                   // vip����ֵ
    uint32_t clogin;                // ������½����
    uint32_t weeklogin;             // �����ۼƵ�½����
    uint32_t login_ip;              // ��½IP
    uint32_t all_login_days;        // �ۼƵ�½����
    uint32_t offline_time;          // ����ʱ��
    double lon;                     // ��γ��
    double lat;                     // ��γ��

    stBaseInfo() {
        name = "";
        sex = 0;
        coin = 0;
        safecoin = 0;
        vip = 0;
        clogin = 0;
        weeklogin = 0;
        login_ip = 0;
        all_login_days = 0;
        offline_time = 0;
        lon = 0;
        lat = 0;
    }
};

enum emACC_VALUE_TYPE {
    emACC_VALUE_COIN        = 1, // ���
    emACC_VALUE_SAFECOIN    = 2, // ������ֵ

    emACC_VALUE_MAX,             // max
};

// ��������
enum emACCDATA_TYPE {
    emACCDATA_TYPE_BASE = 0,    // ������Ϣ
    emACCDATA_TYPE_MISS,        // ��������

    emACCDATA_TYPE_MAX,
};

// ����������Ϣ
struct stRoomCfg {
    uint16_t roomID;            // ����ID
    uint16_t playType;          // �淨
    int64_t enter_min;          // �����ż�
    int64_t enter_max;          // ��������
    int64_t baseScore;          // �׷�
    uint16_t tableNum;          // ����������
    uint8_t marry;              // ƥ�䷽ʽ
    uint8_t limitEnter;         // ��������(ͨ����СЯ��)
    uint32_t showonline;        // ��ʾ����
    int64_t sitdown;            // ��������
    uint8_t feeType;            // ̨������
    int32_t feeValue;           // ̨��ֵ
    uint16_t seatNum;           // ��λ��

    stRoomCfg() {
        memset(this, 0, sizeof(stRoomCfg));
    }
};

//����������
struct stMissionPrizeCfg
{
    uint32_t poid;   //����id
    uint32_t qty;    //����
    stMissionPrizeCfg()
    {
        memset(this, 0, sizeof(stMissionPrizeCfg));
    }
};

//����������Ϣ
struct stMissionCfg
{
    uint32_t                    msid;        //����ID
    uint16_t                    type;        //��������
    uint8_t                     autoprize;   //�Զ��콱
    uint32_t                    cate1;       //����
    uint32_t                    cate2;       //����
    uint32_t                    cate3;       //����
    uint32_t                    cate4;       //����
    uint32_t                    mtimes;      //�ﵽ����
    uint8_t                     straight;     //�Ƿ�����
    uint8_t                     cycle;        //����
    uint32_t                    cycletimes;  //����ɴ���
    uint8_t                     status;       //����״̬
    vector<stMissionPrizeCfg> missionprize; //������
};

//���������Ϣ
struct stUserMission
{
    uint32_t msid;    // ID
    uint32_t rtimes;  // �������
    uint32_t ctimes;  // ������ɴ���
    uint32_t ptime;   // ����ʱ��
    uint8_t  update;  // �������ݿ�����
    uint32_t cptime;  // ���ʱ��
    stUserMission()
    {
        memset(this, 0, sizeof(stUserMission));
    }
};

enum emDBACTION
{
    emDB_ACTION_NONE = 0,           //������
    emDB_ACTION_UPDATE,             //�޸�
    emDB_ACTION_INSERT,             //����
    emDB_ACTION_DELETE,             //ɾ��
};





