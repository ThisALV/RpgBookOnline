allPlayers = 0

function toBoolean(str)
    if str == "true" then
        return true
    elseif str == "false" then
        return false
    else
        error("string not convertible to bool")
    end
end

function assertArgs(condition)
    assert(condition, "Arguments invalides")
end

Rbo.Text = function (interface, args)
    assertArgs(#args == 1 or #args == 2)

    local confirm
    if #args == 2 then
        confirm = toBoolean(args:get(2))
    else
        confirm = true
    end

    interface:print(args:get(1), allPlayers)
    if confirm then
        interface:askConfirm(allPlayers, true)
    end
end

Rbo.Goto = function(interface, args)
    assertArgs(#args == 1)
    return tonumber(args:get(1))
end
