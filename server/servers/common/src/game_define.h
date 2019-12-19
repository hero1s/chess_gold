
#pragma once

#include "svrlib.h"
#include <vector>
#include "error_code.pb.h"

using namespace std;

#define PRO_DENO_100w    1000000
#define PRO_DENO_10w     100000
#define PRO_DENO_10000   10000
#define PRO_DENO_100     100

// ���ݿ��ʾ
enum DB_INDEX_TYPE {
    DB_INDEX_TYPE_ACC = 0,
    DB_INDEX_TYPE_CFG = 1,
    DB_INDEX_TYPE_CENTER = 2,
    DB_INDEX_TYPE_LOG = 3,

    DB_INDEX_TYPE_MAX = 4,
};

// ������״̬
enum emSERVER_STATE {
    emSERVER_STATE_NORMAL = 0,        // ����״̬
    emSERVER_STATE_REPAIR,            // ά��״̬

};
// ����������
enum emSERVER_TYPE {
    emSERVER_TYPE_LOBBY = 1, // ������
    emSERVER_TYPE_GAME = 2, // ��Ϸ��
    emSERVER_TYPE_CENTER = 3, // ���ķ�

};
// ·������
enum emROUTE_TYPE {
    emROUTE_TYPE_ALL_SERVER = 1,// ȫ����Ϸ��
    emROUTE_TYPE_ONE_SERVER = 2,// ָ����Ϸ��


};

// �ƾ���־
struct stBlingUser {
    uint32_t uid;
    int64_t oldValue;
    int64_t newValue;
    int64_t win;
    int64_t fee;           // ̨��
    uint16_t chairid;

    stBlingUser() {
        memset(this, 0, sizeof(stBlingUser));
    }
};

struct stGameBlingLog {
    uint32_t roomID;               // ����ID
    int64_t tableID;               // ����ID
    uint16_t gameType;             // ��Ϸ����
    uint16_t roomType;             // ��������
    uint8_t consume;               // ��������
    uint8_t playType;              // �淨����
    int64_t baseScore;             // �׷�
    uint32_t startTime;            // ��ʼʱ��
    uint32_t endTime;              // ����ʱ��
    vector<stBlingUser> users;     // �û�����
    stringstream operLog;          // ������־
    string chessid;                // �ƾ�ID

    stGameBlingLog() {
        Reset();
    }

    void Reset() {
        roomID = 0;
        tableID = 0;            // ����ID
        gameType = 0;           // ��Ϸ����
        roomType = 0;           // ��������
        consume = 0;            // ��������
        playType = 0;           // �淨����
        baseScore = 0;          // �׷�
        startTime = 0;          // ��ʼʱ��
        endTime = 0;            // ����ʱ��
        users.clear();          // �û�����
        operLog.str("");
        chessid = "";
    }
};

#ifndef PARSE_MSG_FROM_ARRAY
#define PARSE_MSG_FROM_ARRAY(msg)                   \
    if(!pkt_buf)                                    \
        return -1;                                  \
    if (!msg.ParseFromArray(pkt_buf, buf_len)) {    \
        LOG_ERROR("unpack fail");                   \
        return -1;                                  \
    }

#endif // PARSE_MSG_FROM_ARRAY





























