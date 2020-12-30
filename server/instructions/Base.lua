local function isStr(var)
    return type(var) == "string"
end

local function isNum(var)
    return type(var) == "number"
end

local function isBool(var)
    return type(var) == "boolean"
end

local function isTable(var)
    return type(var) == "table"
end

function Rbo.Text(interface, args)
    assertArgs(isStr(args.text))

    interface:print(args.text)
    if args.wait then
        interface:askConfirm(ACTIVE_PLAYERS)
    end
end

function Rbo.Alert(interface, args)
    assertArgs(isStr(args.text))

    interface:printImportant(args.text)
    if args.wait then
        interface:askConfirm(ACTIVE_PLAYERS)
    end
end

function Rbo.Title(interface, args)
    assertArgs(isStr(args.text))

    interface:printTitle(args.text)
    if args.wait then
        interface:askConfirm(ACTIVE_PLAYERS)
    end
end

function Rbo.Note(interface, args)
    assertArgs(isStr(args.text))

    interface:printNote(args.text)
    if args.wait then
        interface:askConfirm(ACTIVE_PLAYERS)
    end
end

function Rbo.Goto(interface, args)
    assertArgs(isNum(args.scene))
    return args.scene
end

function Rbo.YesOrNoDecision(interface, args)
    local all = args.target == "all"
    local leader = args.target == "leader"
    assertArgs((all or leader or isNum(args.target)) and isStr(args.question) and isNum(args.yes) and isNum(args.no))

    local target
    if all then
        target = ACTIVE_PLAYERS
    elseif leader then
        target = interface:leader()
    else
        target = args.target
    end

    local decision = vote(interface:askYesNo(target, args.question))
    return decision == YES and args.yes or args.no
end

function Rbo.PathChoice(interface, args)
    assertArgs(isStr(args.message) and isTable(args.paths) and (isBool(args.wait) or args.wait == nil))

    if type(args.wait) ~= "boolean" then
        args.wait = true
    end

    local paths = ByteVector:new():iterable()
    local options = StringVector:new():iterable()
    for scene, text in pairs(args.paths) do
        assertArgs(isNum(scene) and isStr(text))
        
        paths:add(scene)
        options:add(text)
    end

    local selected = vote(interface:ask(ACTIVE_PLAYERS, args.message, options, not args.wait))
    return paths:get(selected)
end

function Rbo.Checkpoint(interface, args)
    assertArgs(isStr(args.name) and isNum(args.scene) and (isStr(args.message) or args.message == nil))

    local message = args.message or "Saved checkpoint \"%s\"."
    local name = interface:checkpoint(args.name, args.scene)
    interface:printNote(message:format(name))
    interface:askConfirm(interface:leader())
end

function Rbo.EventTo(interface, args)
    local target = args.target
    assertArgs((target == "global" or target == "all" or target == "leader" or isNum(target)) and isStr(args.effect))
    local effect = interface:game():effect(args.effect)

    if target == "global" then
        applyToGlobal(interface, effect)
        interface:checkGame()
        return
    end

    local targets = ByteVector:new():iterable()
    if target == "all" then
        targets = interface:activePlayers():iterable()
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

        interface:sendPlayerUpdate(target_id)
        interface:checkPlayer(target_id)
    end
end

function Rbo.ActionVote(interface, args)
    assertArgs(isStr(args.text) and isStr(args.effect))

    local selected = votePlayer(interface, args.text)
    local effect = interface:game():effect(args.effect)

    effect:apply(interface:player(selected))
    interface:sendPlayerUpdate(selected)
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
        targets:add(votePlayer(interface, "Qui aurait un(e) "..args.item.." ?"))
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
        local target = targets:get(1)

        interface:player(target):consume(args.inv, args.item, args.qty)
        interface:sendPlayerUpdate(target)
        interface:checkPlayer(target)
    end
    
    return has and args.yes or args.no
end
