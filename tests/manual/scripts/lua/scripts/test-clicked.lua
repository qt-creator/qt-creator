G = require 'Gui'

local counter = G.Label {
    text = "0"
}

G.Column {
    G.Row {
        G.Label {
            text = "Counter:"
        },
        counter
    },
    G.Row {
        G.PushButton {
            text = "+",
            onClicked = function()
                counter.text = tostring(tonumber(counter.text) + 1)
            end
        },
        G.PushButton {
            text = "-",
            onClicked = function()
                counter.text = tostring(tonumber(counter.text) - 1)
            end
        }
    }
}:show()
