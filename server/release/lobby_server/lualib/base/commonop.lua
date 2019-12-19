
local floor = math.floor
local random = math.random

loadfile_ex = function (sFileName, sMode, mEnv)
    sMode = sMode or "bt"
    mEnv = mEnv or _ENV
    local h = io.open(sFileName, "rb")
    assert(h, string.format("loadfile_ex fail %s", sFileName))
    local sData = h:read("*a")
    h:close()
    local f, s = load(sData, sFileName, sMode, mEnv)
    assert(f, string.format("loadfile_ex fail %s", s))
    return f
end

print = function ( ... )
    local lInfo = table.pack(...)
    local lResult = {}
    for i = 1, #lInfo do
        if type(lInfo[i]) == "table" then
            table.insert(lResult, require("base.extend").Table.serialize(lInfo[i]))
        elseif lInfo[i] == nil then
            table.insert(lResult, "nil")
        else
            table.insert(lResult, lInfo[i])
        end
    end
    log_debug(table.unpack(lResult))
end

lualib_path = function (sPath)
    return string.format("lualib.%s", sPath)
end

serialize_table = function (t)
    return require("base.extend").Table.serialize(t)
end

table_print = function (t)
    print(require("base.extend").Table.serialize(t))
end

table_print_pretty = function (t)
    print(require("base.extend").Table.pretty_serialize(t))
end

inherit = function (child, parent)
    setmetatable(child, parent)
end

super = function (child)
    return getmetatable(child)
end

local function Trace(sMsg)
    log_debug(debug.traceback(sMsg))
end

safe_call = function (func, ...)
    return xpcall(func, Trace, ...)
end

db_key = function (k)
    return tostring(k)
end

--只取3位小数,不提供其他可能性
decimal = function (val)
    return floor(val*1000)*0.001
end

my_floor = function (val)
    return floor(tonumber(string.format('%.14g', val)))
end

in_random = function (i, j)
    j = j or 100
    return random(j) <= i
end
