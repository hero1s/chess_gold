
#include "common_logic.h"
#include "crypt/md5.h"
#include <sstream>
#include "game_define.h"
#include <algorithm>
#include "msg_define.pb.h"
#include "mysql_mgr/dbmysql_mgr.h"
#include "redis_mgr/redis_mgr.h"
#include "data_cfg_mgr.h"

using namespace std;
using namespace svrlib;

// 检测时间是否在区间
bool CCommonLogic::IsJoinTime(string startTime, string endTime) {
    time_t time;
    if (time::get_time_from_str(startTime, time) == false)
    {
        LOG_ERROR("start time error:{}", startTime);
        return false;
    }
    uint32_t startTick = time;
    if (time::get_time_from_str(endTime, time) == false)
    {
        LOG_ERROR("end time error:{}", endTime);
        return false;
    }
    uint32_t endTick = time;

    return (startTick < time::getSysTime() && time::getSysTime() < endTick);
}
//获得出牌字符串
void CCommonLogic::LogCardString(uint8_t cardData[], uint8_t cardCount)
{
    string   str;
    for (int i = 0; i < cardCount; ++i)
    {
        str += CStringUtility::FormatToString("%x ", cardData[i]);
    }
    LOG_DEBUG("{}", str);
}
// 获得校验码
bool CCommonLogic::VerifyPasswd(string& loginKey,uint32_t uid, uint32_t checkTime) {
    string strDecy = CStringUtility::FormatToString("%s%d%d",CDataCfgMgr::Instance().GetMd5Key().c_str(),uid, checkTime);
    strDecy = MD5(strDecy).toStr();
    if(strDecy != loginKey)
    {
        LOG_ERROR("check passwd error {}-PHP:{}--c++:{}", uid, loginKey, strDecy);
    }
    return strDecy == loginKey;
}

// 判断是否重置信息
bool CCommonLogic::IsNeedReset(time_t lastTime, time_t curTime) {
    if (lastTime == curTime)
        return false;

    static tm tm_early;
    static tm tm_late;
    memset(&tm_early, 0, sizeof(tm_early));
    memset(&tm_late, 0, sizeof(tm_late));
    time::localtime(lastTime, &tm_early);
    time::localtime(curTime, &tm_late);

    uint32_t diffDay = 0;
    //同年同日
    if (tm_early.tm_year != tm_late.tm_year)
    {
        diffDay = 365;
    }
    else if (tm_early.tm_yday != tm_late.tm_yday)
    {
        diffDay = abs(tm_late.tm_yday - tm_early.tm_yday);
    }
    static uint32_t reset_time = 6 * HOUR;// 早上6  点重置
    uint32_t lastDayTime = tm_early.tm_hour * HOUR + tm_early.tm_min * 60 + tm_early.tm_sec;
    uint32_t curDayTime = tm_late.tm_hour * HOUR + tm_late.tm_min * 60 + tm_late.tm_sec;

    if (diffDay >= 2)
    {
        return true;
    }
    else if (diffDay == 1)
    {
        if (curDayTime > reset_time || lastDayTime <= reset_time)
        {
            return true;
        }
    }
    else
    {
        if (curDayTime > reset_time && lastDayTime <= reset_time)
        {
            return true;
        }
    }

    return false;
}
// 原子修改离线玩家数据
bool CCommonLogic::AtomChangeOfflineAccData(uint32_t uid, uint16_t operType, uint16_t subType, int64_t coin, int64_t safecoin,
                                            const string& chessid)
{
    int64_t curCoin = 0, curSafeCoin = 0;
    bool  bRet    = CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_ACC).AtomChangeAccountValue(uid, coin,
                                                                                                    safecoin, curCoin,
                                                                                                    curSafeCoin);
    if (bRet)
    {
        CDBMysqlMgr::Instance().OfflineAccountTransctionCoin(uid, operType, subType, coin, safecoin, curCoin,
                                                             curSafeCoin, chessid);
    }
    return bRet;
}
















