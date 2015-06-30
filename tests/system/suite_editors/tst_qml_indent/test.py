#############################################################################
##
## Copyright (C) 2015 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company.  For licensing terms and
## conditions see http://www.qt.io/terms-conditions.  For further information
## use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file.  Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, The Qt Company gives you certain additional
## rights.  These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

source("../../shared/qtcreator.py")

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    # using a temporary directory won't mess up a potentially existing
    createNewQtQuickApplication(tempDir(), "untitled")
    originalText = prepareQmlFile()
    if originalText:
        testReIndent(originalText)
    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")

def prepareQmlFile():
    if not openDocument("untitled.Resources.qml\.qrc./.main\\.qml"):
        test.fatal("Could not open main.qml")
        return None
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    for i in range(3):
        content = "%s" % editor.plainText
        start = content.find("MouseArea {")
        if not placeCursorToLine(editor, "MouseArea {"):
            test.fatal("Couldn't find line(s) I'm looking for - QML file seems to "
                       "have changed!\nLeaving test...")
            return None
        type(editor, "<Right>")
        type(editor, "<Up>")
        # mark until the end of file
        if platform.system() == 'Darwin':
            markText(editor, "End")
        else:
            markText(editor, "Ctrl+End")
        # unmark the 2 last lines
        for _ in range(2):
            type(editor, "<Shift+Up>")
        type(editor, "<Ctrl+c>")
        for j in range(10):
            type(editor, "<Ctrl+v>")
    # assume the current editor content to be indented correctly
    originalText = "%s" % editor.plainText
    indented = editor.plainText
    lines = str(indented).splitlines()
    test.log("Using %d lines..." % len(lines))
    editor.plainText = "\n".join([line.lstrip() for line in lines]) + "\n"
    return originalText

def testReIndent(originalText):
    editor = waitForObject(":Qt Creator_QmlJSEditor::QmlJSTextEditorWidget")
    correctIndented = len(originalText)
    incorrectIndented = correctIndented + 4004
    type(editor, "<Ctrl+a>")
    test.log("calling re-indent")
    starttime = datetime.utcnow()
    type(editor, "<Ctrl+i>")
    waitFor("len(str(editor.plainText)) in (incorrectIndented, correctIndented)", 25000)
    endtime = datetime.utcnow()
    test.xverify(originalText == str(editor.plainText),
                 "Verify that indenting restored the original text. "
                 "This may fail when empty lines are being indented.")
    invokeMenuItem("File", "Save All")
    waitFor("originalText == str(editor.plainText)", 25000)
    textAfterReIndent = "%s" % editor.plainText
    if originalText==textAfterReIndent:
        test.passes("Text successfully re-indented after saving (indentation took: %d seconds)" % (endtime-starttime).seconds)
    else:
        # shrink the texts - it's huge output that takes long time to finish & screenshot is taken as well
        originalText = shrinkText(originalText, 20)
        textAfterReIndent = shrinkText(textAfterReIndent, 20)
        test.fail("Re-indent of text unsuccessful...",
                  "Original (first 20 lines):\n%s\n\n______________________\nAfter re-indent (first 20 lines):\n%s"
                  % (originalText, textAfterReIndent))

def shrinkText(txt, lines=10):
    return "".join(txt.splitlines(True)[0:lines])
