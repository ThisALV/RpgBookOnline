local function isNum(var)
    return type(var) == "number"
end

local function isStr(var)
    return type(var) == "string"
end

local function isTable(var)
    return type(var) == "table"
end

function Rbo.DrinkMagicPotion(interface, args)
    assertArgs((args.target == "all" or args.target == "leader" or args.target == nil or isNum(args.target)) and isNum(args.max))

    local effect = interface:game():effect("heal")
    local targets

    if args.target == "all" or args.target == nil then
        targets = ACTIVE_PLAYERS
    elseif args.target == "leader" then
        targets = interface:leader()
    else
        targets = interface:leader()
    end

    local replies = interface:askNumber(targets, "How many magical potions you use ? ("..args.max.." max)", 0, args.max):iterable()

    for playerID, consumedPotions in replies:pairs() do
        local target = interface:player(playerID)
        
        local targetPotions = target:inventory("Bag"):count("Magical potion")
        if targetPotions < consumedPotions then
            interface:printImportant("You haven't enough. Only "..targetPotions.." will be consumed !", playerID)
            consumedPotions = targetPotions
        end

        target:consume("Bag", "Magical potion", consumedPotions)
        interface:printNote("Used "..consumedPotions.." magical potions.", playerID)

        for _ = 1, consumedPotions do
            effect:apply(target)
        end

        interface:sendPlayerUpdate(playerID)
    end
end

function Rbo.FirstReply_WaitForAll_PathChoice(interface, args)
    assertArgs(isStr(args.message) and isTable(args.paths))

    local paths = ByteVector:new():iterable()
    local options = StringVector:new():iterable()
    for scene, text in pairs(args.paths) do
        assertArgs(isNum(scene) and isStr(text))

        paths:add(scene)
        options:add(text)
    end

    local selected = vote(interface:ask(ACTIVE_PLAYERS, args.message, options, true, true))

    if selected ~= nil then -- S'il n'y a aucune réponse, alors il n'y a plus aucun joueur vivant, inutile de continuer la partie
        return paths:get(selected)
    end
end

function Rbo.SurpriseAttack(interface, args)
    assertArgs(isStr(args.enemiesGroup))

    local enemiesGroup = EnemiesGroup:new(args.enemiesGroup, interface:game())

    if enemiesGroup:count() == 0 then
        return
    end

    repeat
        local enemy = enemiesGroup:current()
        local skill = enemy:skill()

        interface:printImportant("["..enemy:name().."] attacks you by surprise and deal "..skill.." dmg pts.")

        local players = interface:activePlayers():iterable()

        for i = 1, #players do
            local targetID = players:get(i)
            local target = interface:player(targetID)

            target:stats():change("HP", -skill)
            interface:sendPlayerUpdate(targetID)

            interface:checkPlayer(targetID)
        end
    until enemiesGroup:nextAlive(false)
end
