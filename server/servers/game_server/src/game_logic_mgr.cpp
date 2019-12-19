//
// Created by toney on 2019/9/11.
//

#include "game_logic_mgr.h"
#include <data_cfg_mgr.h>
#include "game_table/game_table.h"
#include "game_table/game_table_coin.h"
#include "game_table/game_room.h"
#include "game_table/game_room_mgr.h"
#include "error_code.pb.h"
#include "mysql_mgr/dbmysql_mgr.h"
#include "game_logic/landlord/game_land_table.h"
#include "game_logic/kuaipao/game_kuaipao_table.h"

CGameLogicMgr::CGameLogicMgr() {

}

CGameLogicMgr::~CGameLogicMgr() {

}

bool CGameLogicMgr::Init() {
    auto gameType = CDataCfgMgr::Instance().GetGameType();

    // ��ͨ�����÷���
    vector<stRoomCfg> vecRooms;
    bool bRet = CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadRoomCfg(gameType, vecRooms);
    if (!bRet)return false;
    for (auto &cfg:vecRooms)
    {
        switch (gameType)
        {
            case net::GAME_CATE_LAND:
            {
                InitLandRoom(gameType, cfg);
                break;
            }
            case net::GAME_CATE_KUAIPAO:
            {
                InitKuaiPaoRoom(gameType, cfg);
                break;
            }
            default:
            {
                LOG_ERROR("error gameType:{}", gameType);
                return false;
            }
        }
    }
    return true;
}

void CGameLogicMgr::ShutDown() {

}

//��ʼ������������
void CGameLogicMgr::InitLandRoom(uint16_t gameType, stRoomCfg &cfg) {
    LOG_DEBUG("��ʼ������������");
    auto pRoom = std::make_shared<CGameRoom>();
    pRoom->SetCreateTableFunc([pRoom](int64_t tableID)
                              {
                                  return std::make_shared<game_land::CGameLandTable>(pRoom, tableID);
                              });
    pRoom->SetRoomCfg(cfg);
    if (pRoom->Init(gameType) == false)
    {
        LOG_ERROR("room config error :{}", cfg.roomID);
        return;
    }
    CGameRoomMgr::Instance().AddRoom(pRoom);
}

//��ʼ���ܵÿ췿��
void CGameLogicMgr::InitKuaiPaoRoom(uint16_t gameType, stRoomCfg &cfg) {
    LOG_DEBUG("��ʼ���ܵÿ췿��");
    auto pRoom = std::make_shared<CGameRoom>();
    pRoom->SetCreateTableFunc([pRoom](int64_t tableID)
                              {
                                  return std::make_shared<game_kuaipao::CGameKuaipaoTable>(pRoom, tableID);
                              });
    pRoom->SetRoomCfg(cfg);
    if (pRoom->Init(gameType) == false)
    {
        LOG_ERROR("room config error :{}", cfg.roomID);
        return;
    }
    CGameRoomMgr::Instance().AddRoom(pRoom);
}




























