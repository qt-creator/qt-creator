# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    for description, type in zip(dumpItems(issuesModel, role=Qt.UserRole),
                                 dumpItems(issuesModel, role=Qt.UserRole + 1)):
        # enum Roles { Description = Qt::UserRole, Type};
        # check if at least one of expected texts found in issue text
        for expectedText in expectedTextsArray:
            if expectedText in description:
                # check if it is error and warn if not - returns False which leads to fail
                if type is not "1":
                    test.warning("Expected error text found, but is not of type: 'error'")
                    return False
                else:
                    test.log("Found expected error (%s)" % expectedText)
                    return True
    return False

# change autocomplete options to manual
def changeAutocompleteToManual(toManual=True):
    invokeMenuItem("Edit", "Preferences...")
    mouseClick(waitForObjectItem(":Options_QListView", "Text Editor"))
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Completion")
    ensureChecked(waitForObject(":Behavior.Autocomplete common prefix_QCheckBox"), not toManual)
    activateCompletion = "Always"
    if toManual:
        activateCompletion = "Manually"
    selectFromCombo(":Activate completion:_QComboBox", activateCompletion)
    verifyEnabled(":Options.OK_QPushButton")
    clickButton(waitForObject(":Options.OK_QPushButton"))

# wait and verify if object item exists/not exists
def checkIfObjectItemExists(object, item, timeout = 3000):
    try:
        waitForObjectItem(object, item, timeout)
        return True
    except:
        return False
