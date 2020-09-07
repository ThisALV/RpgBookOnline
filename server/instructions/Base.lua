local function isStr(var)
    return type(var) == "string"
end

local function isNum(var)
    return type(var) == "number"
end

function Rbo.Text(interface, args)
    assertArgs(isStr(args.text))

    interface:print(args.text)
    if args.wait == nil or args.wait == true then
        interface:askConfirm(ALL_PLAYERS)
    end
end

function Rbo.Goto(interface, args)
    assertArgs(isNum(args.scene))
    return args.scene
end

function Rbo.PathChoice(interface, args)
    assertArgs(type(args.wait) == "boolean" and type(args.paths) == "table")

    local list = StringVector:new():iterable()
    local options = {}
    for scene, text in pairs(args.paths) do
        assertArgs(isNum(scene) and isStr(text))

        table.insert(options, scene)
        list:add(text.." -> "..scene)
    end

    interface:printOptions(list)
    local selected = vote(interface:askReply(ALL_PLAYERS, 1, #list, args.wait))

    return options[selected]
end

function Rbo.Checkpoint(interface, args)
    assertArgs(isStr(args.name) and isNum(args.scene))

    local name = interface:checkpoint(args.name, args.scene)
    interface:print("Sauvegarde sur \""..name.."\".")
end

function Rbo.EventTo(interface, args)
    local target = args.target
    assertArgs((target == "global" or target == "all" or target == "leader" or isNum(target)) and type(args.effect) == "string")
    local effect = interface:game():effect(args.effect)

    if target == "global" then
        applyToGlobal(interface, effect)
        interface:checkGame()
        return
    end

    local targets = ByteVector:new():iterable()
    if target == "all" then
        targets = interface:players():iterable()
    else
        targets:add(target == "leader" and interface:leader() or target)
    end

    for i = 1, #targets do
        local target_id = targets:get(i)
        local target_p = interface:player(target_id)

        if effect:simulateItemsChanges(target_p) ~= SimulationResult.Ok and isNum(args.failure) then
            return args.failure
        end
        effect:apply(target_p)

        interface:sendInfos(target_id)
        interface:checkPlayer(target_id)
    end
end

function Rbo.ActionVote(interface, args)
    assertArgs(isStr(args.text) and isStr(args.effect))

    interface:print(args.text)
    local selected = vote(interface:askReply(ALL_PLAYERS, interface:players()))
    local effect = interface:game():effect(args.effect)

    effect:apply(interface:player(selected))
    interface:sendInfos(selected)
    interface:checkPlayer(selected)
end

function Rbo.Test(interface, args)
    local leader = args.target == "leader"
    local vote = args.target == "vote"
    assertArgs(isStr(args.text) and (leader or vote or isNum(args.target)) and isStr(args.stat) and isNum(args.dices) and isNum(args.success) and isNum(args.failure))
    
    interface:print(args.text)
    local target_id
    if leader then
        target_id = interface:leader()
    elseif vote then
        target_id = votePlayer()
    else
        target_id = tonumber(args.target)
    end
    if args.wait == nil or args.wait == true then
        interface:askConfirm(target_id)
    end
    
    local value = interface:player(target_id):stats():get(args.stat)
    return value >= dices(args.dices, 6) and args.success or args.failure
end

function Rbo.IfHas(interface, args)
    local global = args.target == "global"
    local leader = args.target == "leader"
    local vote = args.target == "vote"
    assertArgs(not(args.consumed and global) and isStr(args.text) and (global or leader or vote or isNum(args.target)) and isStr(args.inv) and isStr(args.item) and isNum(args.qty) and isNum(args.yes) and isNum(args.no))

    interface:print(args.text)
    local targets = ByteVector:new():iterable()
    if global then
        targets = interface:players():iterable()
    elseif leader then
        targets:add(interface:leader())
    elseif vote then
        targets:add(votePlayer(interface))
    else
        targets:add(args.target)
    end

    if args.wait == nil or args.wait == true then
        interface:askConfirm(global and ALL_PLAYERS or targets:get(1))
    end

    local count = 0
    for i = 1, #targets do
        count = count + interface:player(targets:get(i)):inventory(args.inv):count(args.item)
    end

    local has = count >= args.qty
    if has and args.consumed then
        interface:player(targets:get(1)):consume(args.inv, args.item, args.qty)
    end
    
    return has and args.yes or args.no
end
