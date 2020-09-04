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
    assertArgs((target == "global" or target == "all" or target == "leader" or isStr(target) == "number") and type(args.effect))
    local effect = interface:game():effect(args.effect)

    if target == "global" then
        applyToGlobal(interface, args.effect)
        return
    end

    local targets = ByteVector:new():iterable()
    if target == "all" then
        targets = getIDs()
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
    assertArgs(isStr(args.text) == "string" and type(args.effect))

    interface:print(args.text)
    local selected = vote(interface:askReply(ALL_PLAYERS, getIDs()))
    local effect = interface:effect(args.effect)

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
    assertArgs(isStr(args.text) and (args.target == "leader" or args.target == "vote" or isNum(args.target)) and isStr(args.stat) and isNum(args.dices) and isNum(args.success) and isNum(args.failure))
    
    interface:print(args.text)
    local target_id = evalTarget(interface, args.target)
    local value = interface:player(target_id):stats():get(args.stat)
    return value <= dices(args.dices) and args.success or args.failure
end

function Rbo.IfHas(interface, args)
    assertArgs(isStr(args.text) and (args.target == "leader" or args.target == "vote" or isNum(args.target)) and isStr(args.inv) and isStr(args.item) and isNum(args.qty) and isNum(args.yes) and isNum(args.no))
    
    interface:print(args.text)
    local target_id = evalTarget(interface, args.target)
    local count = interface:player(target_id):inventory(args.inv):count(args.item)
    return count >= args.qty and args.yes or args.no
end
