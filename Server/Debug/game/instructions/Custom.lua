allPlayers = 0

assertArgs = assertArgs or function (condition)
    assert(condition, "Arguments invalides")
end

Rbo.YesNo = function (interface, args)
    assertArgs(#args == 2)
    local yes = tonumber(args:get(1))
    local no = tonumber(args:get(2))

    interface:printYesNo(allPlayers)
    return (interface:askYesNo(allPlayers, true):get(0) == 0 and yes) or no
end

Rbo.RangeOptions = function (interface, args)
    assertArgs(#args > 1)
    local begin = tonumber(args:get(1))

    local options = {}
    for i = 1, #args do
        options[i] = i + begin - 1
    end

    interface:printOptions(args, allPlayers, 2)
    local selected = interface:askReply(allPlayers, options, true)
end

Rbo.TextThen = function (interface, args)
    assertArgs(#args == 2)

    interface:print(args:get(1), allPlayers)
    interface:askConfirm(allPlayers, true)

    return tonumber(args:get(2))
end

Rbo.Error = function (interface, args)
    assertArgs(#args == 1)
    error(args:get(1))
end
