local function tst_embedWidget()
    T = require 'TextEditor'
    G = require 'Gui'

    local editor = T.currentEditor()
    if not editor then
        print("No editor found")
        return
    end

    local cursor = editor:cursor()

    local embed
    local optionals = G.Group {
        visible = false,
        G.Row {
            G.Label {
                text = "Optional 1",
            },
            G.Label {
                text = "Optional 2",
            },
        }
    }

    local layout = G.Group {
        G.Column {
            "Hello", G.br,
            "World",
            G.br,
            G.PushButton {
                text = "Show optionals",
                onClicked = function()
                    optionals.visible = not optionals.visible
                    embed:resize()
                end,
            },
            optionals,
            G.PushButton {
                text = "Close",
                onClicked = function()
                    embed:close()
                end,
            },
        }
    }

    embed = editor:addEmbeddedWidget(layout, cursor:mainCursor():position())
    embed:onShouldClose(function()
        embed:close()
    end)
end

local function setup()
    Action = require 'Action'
    Action.create("LuaTests.textEditorEmbedDemo", {
        text = "Lua TextEditor Embed Demo",
        onTrigger = tst_embedWidget,
    })
end

return {
    setup = setup,
}
