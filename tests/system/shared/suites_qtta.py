#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

import __builtin__

# appends to line, by typing <typeWhat> after <insertAfterLine> text into <codeArea> widget
def appendToLine(codeArea, insertAfterLine, typeWhat):
    if not placeCursorToLine(codeArea, insertAfterLine):
        return False
    type(codeArea, typeWhat)
    return True

# checks if error is properly reported, returns True if succeeded and False if not.
# Current implementation is focused on allowing different compilers, and it is enough if one of the expected messages
# is found in issues view. warnIfMoreIssues should warn if there are more than one issue, no matter how many
# expected texts are in array (because they are alternatives).
def checkSyntaxError(issuesView, expectedTextsArray, warnIfMoreIssues = True):
    issuesModel = issuesView.model()
    # wait for issues
    waitFor("issuesModel.rowCount() > 0", 5000)
    # warn if more issues reported
    if(warnIfMoreIssues and issuesModel.rowCount() > 1):
        test.warning("More than one expected issues reported")
    # iterate issues and check if there exists "Unexpected token" message
    for description, type in zip(dumpItems(issuesModel, role=Qt.UserRole + 3),
                                 dumpItems(issuesModel, role=Qt.UserRole + 5)):
        # enum Roles { File = Qt::UserRole, Line, MovedLine, Description, FileNotFound, Type, Category, Icon, Task_t };
        # check if at least one of expected texts found in issue text
        for expectedText in expectedTextsArray:
            if expectedText in description:
                # check if it is error and warn if not - returns False which leads to fail
                if type is not "1":
                    test.warning("Expected error text found, but is not of type: 'error'")
                    return False
                else:
                    return True
    return False

# change autocomplete options to manual
def changeAutocompleteToManual():
    invokeMenuItem("Tools", "Options...")
    mouseClick(waitForObjectItem(":Options_QListView", "Text Editor"), 5, 5, 0, Qt.LeftButton)
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Completion")
    ensureChecked(waitForObject(":Behavior.Autocomplete common prefix_QCheckBox"), False)
    selectFromCombo(":Behavior.completionTrigger_QComboBox", "Manually")
    verifyEnabled(":Options.OK_QPushButton")
    clickButton(waitForObject(":Options.OK_QPushButton"))

# wait and verify if object item exists/not exists
def checkIfObjectItemExists(object, item, timeout = 3000):
    try:
        waitForObjectItem(object, item, timeout)
        return True
    except:
        return False

# this function creates a string holding the real name of a Qml Item
# param type defines the Qml type (support is limited)
# param container defines the container of the Qml item - can be a real or symbolic name
# param clip defines the state of the clip property (true/false)
# param text a string holding the complete text property (e.g. "text='example'", "text~='ex.*'")
def getQmlItem(type, container, clip, text=""):
    if (container.startswith(":")):
        container = "'%s'" % container
    clip = ("%s" % __builtin__.bool(clip)).lower()
    return ("{clip='%s' container=%s enabled='true' %s type='%s' unnamed='1' visible='true'}"
            % (clip, container, text, type))