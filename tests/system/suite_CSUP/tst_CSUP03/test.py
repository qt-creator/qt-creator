############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

source("../../shared/qtcreator.py")

inputDialog = "{type='QDialog' unnamed='1' visible='1' windowTitle='Extract Function Refactoring'}"

def revertMainCpp():
    invokeMenuItem('File', 'Revert "main.cpp" to Saved')
    waitFor("object.exists(':Revert to Saved_QMessageBox')", 1000)
    clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))

def constructExpectedCode(original, codeLines, funcSuffix):
    withBraces = 'WithBraces' in funcSuffix
    myFuncCall = "    myFunc%s(dummy);" % funcSuffix
    if not withBraces:
        myFuncCall += "\n    "
    tmp = original.splitlines()
    tmp.insert(tmp.index("{") + 1, "\n".join(["    %s" % codeLines[1], myFuncCall]))
    insertHere = tmp.index("using namespace std;")
    if insertHere == -1:
        test.fatal("Template code seems to have changed.")
        return original
    generatedFunc = "\nvoid myFunc%s(int dummy)\n{" % funcSuffix
    for line in codeLines[2:]:
        if line.endswith(";") and not line.startswith("while"):
            generatedFunc += "\n        %s" % line
        else:
            generatedFunc += "\n    %s" % line
    if withBraces:
        generatedFunc += "\n        \n    }"
    # QTCREATORBUG-12118: last line has 4 additional blanks
    if JIRA.isBugStillOpen(12118):
        generatedFunc += "    "
    else:
        test.warning("Remove unnecessary code - QTCREATORBUG-12118 is closed.")
    generatedFunc += "\n}"
    tmp.insert(insertHere + 1, generatedFunc)
    return "\n".join(tmp) + "\n"

def main():
    if platform.system() == 'Darwin':
        home = '<Ctrl+Left>'
    else:
        home = '<Home>'

    code = {"if" : ["", "int dummy = 0;", "if (dummy < 10)", "++dummy;"],
            "if with braces" : ["", "int dummy = 0;", "if (dummy < 10) {", "++dummy;"],
            "if else" : ["", "int dummy = 0;", "if (dummy < 10)", "++dummy;", "else", "--dummy;"],
            "if else with braces" : ["", "int dummy = 0;", "if (dummy < 10) {",
                                     "++dummy;", "} else {", "--dummy;"],
            "while" : ["", "int dummy = 0;", "while (dummy < 10)", "++dummy;"],
            "while with braces" : ["", "int dummy = 0;", "while (dummy < 10) {", "++dummy;"],
            "do while" : ["", "int dummy = 0;", "do", "++dummy;", "while (dummy < 10);"]
            }
    for useClang in [False, True]:
        with TestSection(getCodeModelString(useClang)):
            if not startCreator(useClang):
                continue
            projectName = createNewNonQtProject()
            checkCodeModelSettings(useClang)
            openDocument("%s.Sources.main\\.cpp" % projectName)
            editor = getEditorForFileSuffix("main.cpp")
            if not editor:
                test.fatal("Failed to get an editor - leaving test.")
                invokeMenuItem("File", "Exit")
                return

            originalContent = str(editor.plainText)
            for case, codeLines in code.items():
                funcSuffix = case.title().replace(" ", "")
                test.log("Testing: Extract Function for '%s'" % case)
                if not placeCursorToLine(editor, "{"):
                    continue
                typeLines(editor, codeLines)
                if not placeCursorToLine(editor, codeLines[2]):
                    revertMainCpp()
                    continue
                type(editor, home)
                markText(editor, "Right", 2)
                snooze(1) # avoid timing issue with the parser
                invokeContextMenuItem(editor, 'Refactor', 'Extract Function')
                funcEdit = waitForObject("{buddy={text='Function name' type='QLabel' unnamed='1' "
                                         "visible='1' window=%s} type='Utils::FancyLineEdit' "
                                         "unnamed='1' visible='1'}" % inputDialog)
                replaceEditorContent(funcEdit, "myFunc%s" % funcSuffix)
                clickButton(waitForObject("{text='OK' type='QPushButton' unnamed='1' visible='1' window=%s}"
                                          % inputDialog))
                waitFor("'void myFunc%s' in str(editor.plainText)" % funcSuffix, 2500)
                # verify the change
                modifiedCode = str(editor.plainText)
                expectedCode = constructExpectedCode(originalContent, codeLines, funcSuffix)
                test.compare(modifiedCode, expectedCode, "Verifying whether code matches expected.")
                # reverting to initial state of main.cpp
                revertMainCpp()
            snooze(1)   # "Close All" might be disabled
            invokeMenuItem('File', 'Close All')
            invokeMenuItem('File', 'Exit')
