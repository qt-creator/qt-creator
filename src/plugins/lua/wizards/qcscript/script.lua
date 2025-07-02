T = require'TextEditor'
local editor = T.currentEditor()
local cursor = editor:cursor()
cursor:insertText('Hello <name>!')
local cursors = cursor:cursors()
for i,c in ipairs(cursors) do
    c:movePosition(T.TextCursor.MoveOperation.Left,
                   T.TextCursor.MoveMode.MoveAnchor,
                   1)
    c:movePosition(T.TextCursor.MoveOperation.Left,
                   T.TextCursor.MoveMode.KeepAnchor,
                   6)
end
cursor:setCursors(cursors)
editor:setCursor(cursor)
