import re;

# places the cursor inside the given editor into the given line
# (leading and trailing whitespaces are ignored!)
# and goes to the end of the line
# line can be a regex - but if so, remember to set isRegex to True
# the function returns True if this went fine, False on error
def placeCursorToLine(editor,line,isRegex=False):
    cursor = editor.textCursor()
    oldPosition = 0
    cursor.setPosition(oldPosition)
    found = False
    if isRegex:
        regex = re.compile(line)
    while not found:
        currentLine = str(lineUnderCursor(editor)).strip()
        if isRegex:
            if regex.match(currentLine):
                found = True
            else:
                type(editor, "<Down>")
                if oldPosition==editor.textCursor().position():
                    break
                oldPosition = editor.textCursor().position()
        else:
            if currentLine==line:
                found = True
            else:
                type(editor, "<Down>")
                if oldPosition==editor.textCursor().position():
                    break
                oldPosition = editor.textCursor().position()
    if not found:
        test.fatal("Couldn't find line matching\n\n%s\n\nLeaving test..." % line)
        return False
    cursor=editor.textCursor()
    cursor.movePosition(QTextCursor.EndOfLine, QTextCursor.MoveAnchor)
    editor.setTextCursor(cursor)
    return True

# this function simply opens the context menu inside the given editor
# at the same position where the text cursor is located at
def openContextMenuOnTextCursorPosition(editor):
    rect = editor.cursorRect(editor.textCursor())
    openContextMenu(editor, rect.x+rect.width/2, rect.y+rect.height/2, 0)

# this function marks/selects the text inside the given editor from position
# startPosition to endPosition (both inclusive)
def markText(editor, startPosition, endPosition):
    cursor = editor.textCursor()
    cursor.setPosition(startPosition)
    cursor.movePosition(QTextCursor.StartOfLine)
    editor.setTextCursor(cursor)
    cursor.movePosition(QTextCursor.Right, QTextCursor.KeepAnchor, endPosition-startPosition)
    cursor.movePosition(QTextCursor.EndOfLine, QTextCursor.KeepAnchor)
    cursor.setPosition(endPosition, QTextCursor.KeepAnchor)
    editor.setTextCursor(cursor)

# works for all standard editors
def replaceEditorContent(editor, newcontent):
    type(editor, "<Ctrl+A>")
    type(editor, "<Delete>")
    type(editor, newcontent)
