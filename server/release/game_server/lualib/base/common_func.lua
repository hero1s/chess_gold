-------------------------------------------------------------------
-- 文  件:	CommonFunc.lua
-- 描  述:	实现公用的脚本函数
-------------------------------------------------------------------


-------------------------------------------------------------------
-- @ RequireFile : 加载脚本文件
-- @ FilaName    : 要加载的文件名(不包括文件后缀)
-------------------------------------------------------------------
function RequireFile(FilaName)
    print("Loading " .. FilaName .. ".lua")
    require(FilaName)
end


-------------------------------------------------------------------
-- CopyTable :拷贝表数据
-- @param b : 原始表
-- @param a : 新表
-------------------------------------------------------------------
-- lua的表默认是引用的，例如a={1,2} b=a b[1]=2 此时a[1]也会等于2
-- 这个函数支持将表真正的拷贝一份，各自可以独立修改
-- 同时支持嵌套的表拷贝,例如a={1,2,{3,4,5}} CopyTable(b,a)
-------------------------------------------------------------------
function CopyTable(a, b)
    for key, value in pairs(b) do
        if type(value) == 'table' and value["parent"] == nil then
            a[key] = {}
            CopyTable(a[key], value)
        else
            a[key] = value
        end
    end
end

-------------------------------------------------------------------
-- FormatFuncAndParam : 格式化调用的函数名和参数
-- @param FuncName : 脚本函数名
-- @param ParamTable : 调用脚本函数需要传入的参数列表
-- @Return : 返回格式化后的字符串
-------------------------------------------------------------------
function FormatFuncAndParam(FuncName, ParamTable)
    assert(FuncName ~= nil and FuncName ~= "")
    if FuncName == nil or FuncName == "" then
        return ""
    end

    if ParamTable == nil or next(ParamTable) == nil then
        return FuncName
    else
        local strFormat = FuncName .. "?"
        for Index, Value in pairs(ParamTable) do
            if type(Value) == 'string' then
                strFormat = strFormat .. "**" .. Value .. "&"
            elseif type(Value) == 'number' then
                strFormat = strFormat .. Value .. "&"
            else
                assert(false)
                print("错误的脚本函数参数")
            end
        end
        return strFormat
    end
end

-------------------------------------------------------------------
-- CallFuncByFormatString : 通过字符串分离要调用的函数及参数
-- @param FormatString : 格式化的字符串
-- @param WildValue : 可以为nil，主要用于参数中有通配参数的值
-------------------------------------------------------------------
function CallFuncByFormatString(FormatString, WildValue)
    assert(FormatString ~= nil and FormatString ~= "")
    if FormatString == nil or FormatString == "" then
        return
    end

    -- 没有函数参数
    local Pos = string.find(FormatString, "?")
    if Pos == nil then
        if _G[FunctionName] == nil then
            print("没有找到函数: " .. FunctionName)
        else
            _G[FormatString]()
        end
        return
    end

    local ParamList = {}
    local StringLen = string.len(FormatString)
    local FunctionName = string.sub(FormatString, 1, (Pos - 1))

    Pos = Pos + 1
    EndPos = string.find(FormatString, "&", Pos)
    while EndPos ~= nil or Pos <= StringLen do
        if EndPos == nil then
            EndPos = StringLen + 1
        end
        local ParamEle = string.sub(FormatString, Pos, (EndPos - 1))
        if ParamEle ~= nil then
            --- 分离字符串很数字类变量
            if string.byte(ParamEle, 1) == 42 and string.byte(ParamEle, 2) == 42 then
                local SubString = string.sub(ParamEle, 3)
                if SubString ~= PLAYERID_PARAM then
                    table.insert(ParamList, SubString)
                else
                    table.insert(ParamList, WildValue)
                end
            else
                table.insert(ParamList, tonumber(ParamEle))
            end
            Pos = EndPos + 1
            EndPos = string.find(FormatString, "&", Pos)
        end
    end

    --print("call "..FunctionName.." param: "..unpack(ParamList))
    -- 调用函数
    if _G[FunctionName] == nil then
        print("没有找到函数: " .. FunctionName)
    else
        _G[FunctionName](unpack(ParamList))
    end
end

-- 计数器
function newCounter(base, increase)
    local i = base or 0
    local delta = increase or 1
    return function()
        i = i + delta
        return i
    end
end

-- 合并表
function MergeTable(t1, t2)
    local t = {}
    for k, v in pairs(t1) do
        table.insert(t, v)
    end
    for k, v in pairs(t2) do
        local have = false
        for kk, vv in pairs(t1) do
            if vv == v then
                have = true
                break
            end
        end
        if not have then
            table.insert(t, v)
        end
    end
    return t
end

-- 调试输出
function info(msg)
    print(msg)
end



