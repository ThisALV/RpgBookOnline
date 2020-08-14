Rbo.YesNo = function (interface, args)
    assertArgs(#args == 2)
    local yes = tonumber(args:get(1))
    local no = tonumber(args:get(2))

    interface:printYesNo(allPlayers)
    return (vote(interface:askYesNo(allPlayers, true)) == 0 and yes) or no
end

Rbo.EventTo = function (interface, args)
   assertArgs(#args == 2)
   local target_id = tonumber(args:get(1))
   local target = interface:player(target_id)
   local effect = args:get(2)

   interface:game().effects:iterable():get(effect):apply(target)
   interface:sendInfos(target_id)
end

Rbo.RangeOptions = function (interface, args)
    assertArgs(#args > 1)
    local begin = tonumber(args:get(1))

    local options = ByteVector:new():iterable()
    for i = 1, (#args - 1) do
        options:add(i + begin - 1)
    end

    args:erase(0)

    interface:printOptions(args, allPlayers, 2)
    return vote(interface:askReply(allPlayers, options, true))
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
