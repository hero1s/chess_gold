
-- 新的一天回调
function new_day(player)
    log_debug("new day call back "..player:GetUID());

    return true;
end
-- 新的一周回调
function new_week(player)
    log_debug("new week call back "..player:GetUID());

    return true;
end
-- 新的一月回调
function new_month(player)
    log_debug("new month call back "..player:GetUID());

    return true;
end
-- 登录回调
function login_on(player)
    log_debug("login on call back "..player:GetUID());

    return true;
end
-- 登出回调
function login_out(player)
    log_debug("login out call back "..player:GetUID());

    return true;
end




