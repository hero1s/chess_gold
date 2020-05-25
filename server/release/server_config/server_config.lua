
--*********************************************
--************ 服务器配置
--*********************************************
--数据库配置(改成只使用一个数据库地址toney)
local database_config = { ip = "127.0.0.1", port = 3306, user = "root", passwd = "e23456" };
local database_dbname = {}
database_dbname[0] = "test_toney";
database_dbname[1] = "test_toney";
database_dbname[2] = "test_toney";
database_dbname[3] = "test_toney";

--redis配置
local redis_config = { host = "127.0.0.1",port = 13000,passwd="e2345" };
--local redis_config = { host = "139.199.209.147",port = 13000,passwd="e2345" };
--local redis_config = { host = "192.168.1.153",port = 13000,passwd="e2345" };
--开启mysql的服务器id
local open_sql_sids = {1,2};

--全局配置信息
server_config =
{
    center = { ip = "127.0.0.1",port = 4888,name = "中心服"},
    lobby  = {  [1] = { ip = "0.0.0.0",lanip = "127.0.0.1",port = 4777,in_port = 4778,php_port = 4779,name = "大厅服1"},
                [2] = { ip = "0.0.0.0",lanip = "127.0.0.1",port = 3777,in_port = 3778,php_port = 3779,name = "大厅服2"},
             },
}
--游戏服配置
game_server_config =
{
    [11] = { game_type = 1,name="斗地主"},--斗地主
    [21] = { game_type = 2,name="跑得快"},--跑得快
}
--key配置
server_key =
{
    loginkey = "qiangshouhudong",
}

-- 中心服务器配置
function center_config(sid, gameConfig)
    load_db_config(sid, gameConfig);
    load_redis_config(sid, gameConfig);
    return true
end

-- 大厅服务器配置
function lobby_config(sid, lobbyConfig)
    load_db_config(sid, lobbyConfig);
    load_redis_config(sid, lobbyConfig);
    return true;
end

-- 游戏服务器配置
function game_config(sid, gameConfig)
    load_db_config(sid, gameConfig);
    load_redis_config(sid, gameConfig);
    return true;
end

-- 加载数据库
function load_db_config(sid, serviceConfig)
    for k, v in pairs(database_dbname) do
        local cfg = serviceConfig:GetDBConf(k);
        cfg:SetDBInfo(database_config.ip, database_config.port, v, database_config.user, database_config.passwd);
    end
end
-- 加载redis
function load_redis_config(sid,serviceConfig)
    local cfg = serviceConfig:GetRedisConf(0);
    cfg:SetRedisHost(redis_config.host, redis_config.port,redis_config.passwd);
end
-- 设置服务器基础信息
function init_server_cfg(sid,serverCfg)

    local _loglv        = 0;
    local _logsize      = 52428800;
    local _logdays      = 5;
    local _logasync     = 0;
    local _logname      = sid.."_log.txt";
    local _logmysql     = sid.."_mysql_error.txt";
    if(is_open_sql(sid) == false) then
        _logmysql = "";
    end

    serverCfg:SetLogInfo(_loglv,_logsize,_logdays,_logasync,_logname,_logmysql);
end
-- 是否需要开启mysql日志
function is_open_sql(sid)
    for k, v in pairs(open_sql_sids) do
        if v == sid then
            return true
        end
    end
    return true
end
-- 游戏服连接大厅服
function init_lobby_mgr()
    local cfg = server_config["lobby"];
    for k,v in pairs(cfg) do
        local ip = v["lanip"];
        local port = v["in_port"];
        ConnectLobby(ip,port,k);
    end
end



