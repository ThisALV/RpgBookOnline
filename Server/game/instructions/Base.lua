function Rbo.Text(interface, args)
    assertArgs(#args == 1 or #args == 2)

    local confirm
    if #args == 2 then
        confirm = toBoolean(args:get(2))
    else
        confirm = true
    end

    interface:print(args:get(1))
    if confirm then
        interface:askConfirm(ALL_PLAYERS)
    end
end

function Rbo.Goto(interface, args)
    assertArgs(#args == 1)
    return tonumber(args:get(1))
end

function Rbo.PathChoice(interface, args)
    assertArgs(#args ~= 0 and #args % 2 == 1)

    local wait = toBoolean(args:get(1))
    local list = StringVector:new():iterable()
    
    local options = {}
    for i = 2, #args, 2 do
        local txt = args:get(i)
        local path = tonumber(args:get(i + 1))

        table.insert(options, path)
        list:add(txt.." -> "..path)
    end

    interface:printOptions(list)
    local selected = vote(interface:askReply(ALL_PLAYERS, 1, #list, wait))

    return options[selected]
end

function Rbo.EventTo(interface, args)
    assertArgs(#args == 2 or #args == 3)
    local target = args:get(1)
    local effect = interface:game():effect(args:get(2))
    
    if target == "global" then
        applyToGlobal(interface, effect)
        return
    end

    local targets = ByteVector:new():iterable()
    if target == "all" then
        targets = getIDs()
    else
        targets:add(target == "leader" and interface:leader() or tonumber(target))
    end

    for i = 1, #targets do
        local target_id = targets:get(i)
        local target_p = interface:player(target_id)

        if effect:simulateItemsChanges(target_p) ~= SimulationResult.Ok and #args == 3 then
            return tonumber(args:get(3))
        end
        effect:apply(target_p)

        interface:sendInfos(target_id)
        interface:checkPlayer(target_id)
    end
end

function Rbo.ActionVote(interface, args)
    assertArgs(#args == 2)

    interface:print(args:get(1))
    local selected = vote(interface:askReply(ALL_PLAYERS, getIDs()))
    local effect = interface:effect(args:get(2))

    effect:apply(interface:player(selected))
end

local function dices(i)
    local result = 0
    for j = 1, i do
        result = result + math.random(1, 6)
    end

    return result
end

local function evalTarget(interface, target)
    local target_id
    if target == "leader" then
        target_id = interface:leader()
    elseif target == "vote" then
        target_id = vote(interface:askReply(ALL_PLAYERS, getIDs()))
    else
        target_id = tonumber(target)
    end

    return target_id
end

function Rbo.Test(interface, args)
    assertArgs(#args == 6)
    local nb_dices = tonumber(args:get(4))
    local success = tonumber(args:get(5))
    local failure = tonumber(args:get(6))
    
    interface:print(args:get(1))
    local target_id = evalTarget(interface, args:get(2))
    local stat = interface:player(target_id):stats():get(args:get(3))
    return stat <= dices(nb_dices) and success or failure
end

function Rbo.IfHas(interface, args)
    assertArgs(#args == 7)
    local qty = tonumber(args:get(5))
    local success = tonumber(args:get(6))
    local failure = tonumber(args:get(7))
    
    interface:print(args:get(1))
    local target_id = evalTarget(interface, args:get(2))
    local count = interface:player(target_id):inventory(args:get(3)):count(args:get(4))
    return count >= qty and success or failure
end
