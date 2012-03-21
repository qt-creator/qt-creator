
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
    for row in range(issuesModel.rowCount()):
        # enum Roles { File = Qt::UserRole, Line, MovedLine, Description, FileNotFound, Type, Category, Icon, Task_t };
        index = issuesModel.index(row, 0)
        description = str(index.data(Qt.UserRole + 3).toString())
        type = str(index.data(Qt.UserRole + 5).toString())
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

# wait and verify if object exists/not exists
def checkIfObjectExists(name, shouldExist = True, timeout = 3000, verboseOnFail = False):
    result = waitFor("object.exists(name) == shouldExist", timeout)
    if verboseOnFail and not result:
        test.log("checkIfObjectExists() failed for '%s'" % name)
    return result

# change autocomplete options to manual
def changeAutocompleteToManual():
    invokeMenuItem("Tools", "Options...")
    mouseClick(waitForObjectItem(":Options_QListView", "Text Editor"), 5, 5, 0, Qt.LeftButton)
    clickTab(waitForObject(":Options.qt_tabwidget_tabbar_QTabBar"), "Completion")
    ensureChecked(waitForObject(":Behavior.Autocomplete common prefix_QCheckBox"), False)
    selectFromCombo(":Behavior.completionTrigger_QComboBox", "Manually")
    verifyEnabled(":Options.OK_QPushButton")
    clickButton(waitForObject(":Options.OK_QPushButton"))

