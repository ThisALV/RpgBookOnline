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
    -- TODO : Gérer les items changes correctement
    assertArgs(#args == 2)
    local target = args:get(1)
    local effect = interface:game():effect(args:get(2))
    
    if target == "global" then
        applyToGlobal(interface, effect)
        return
    end

    local target_id = target == "leader" and interface:leader() or tonumber(target)
    effect:apply(interface:player(target_id))

    interface:sendInfos(target_id)
    interface:checkPlayer(target_id)
end
