
function reload_file(filename)
    --卸载旧文件
    package.loaded[filename] = nil
    --装载新文件
    require(filename);
    log_debug("load lua file:" .. filename .. ".lua");
    return true;
end

function init_lua_service(lua_svr)
    log_debug("init lua service");
    lua_svr:set_start(function()
        log_debug(" lua_service start call back " .. curTimeStr());
    end);
    lua_svr:set_exit(function()
        log_debug(" lua_service exit call back " .. curTimeStr());
    end);
    lua_svr:set_dispatch(function(cmd,msg)
        log_debug("lua_service dispatch msg " .. cmd .. msg);
    end);
    math.randomseed(os.time());
end
