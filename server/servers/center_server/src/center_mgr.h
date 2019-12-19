
#pragma once

#include <vector>
#include "svrlib.h"
#include <unordered_map>
#include <memory>
#include "packet/inner_protobuf_pkg.h"
#include "servers_msg.pb.h"
#include "server_connect/server_client.h"

using namespace std;
using namespace svrlib;
using namespace Network;

// ���ķ�������
class CCenterMgr : public CServerClientMgr, public AutoDeleteSingleton<CCenterMgr> {
public:
    CCenterMgr();

    virtual ~CCenterMgr();

    virtual bool Init();

    virtual void ShutDown();

private:


};

/*
 * ��Ϸ�������������ķ������Ĺ�����Ϣ����
*/


































