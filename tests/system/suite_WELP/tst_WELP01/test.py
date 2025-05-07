# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")


def clickItemVerifyHelpCombo(button, expectedHelpComboRegex, testDetails):
    mouseClick(button)
    helpCombo = waitForObject(":Qt Creator_HelpSelector_QComboBox")
    if not test.verify(waitFor('re.match(expectedHelpComboRegex, str(helpCombo.currentText))',
                               5000), testDetails):
        test.log("Found %s" % str(helpCombo.currentText))
    # select "Welcome" page from left toolbar again
    switchViewTo(ViewConstants.WELCOME)

    return test.verify(checkIfObjectExists(getWelcomeScreenSideBarButton('Projects'), timeout=1000),
                       "Verifying: 'Projects' button is being displayed.")

def buttonActive(button):
    return waitForObject(button).checked

def waitForButtonsState(projectsActive, examplesActive, tutorialsActive, timeout=5000):
    projButton = getWelcomeScreenSideBarButton('Projects')
    exmpButton = getWelcomeScreenSideBarButton('Examples')
    tutoButton = getWelcomeScreenSideBarButton('Tutorials')
    if not all(map(object.exists, (projButton, exmpButton, tutoButton))):
        return False
    return waitFor('buttonActive(projButton) == projectsActive '
                   'and buttonActive(exmpButton) == examplesActive '
                   'and buttonActive(tutoButton) == tutorialsActive', timeout)

def checkTableViewForContent(tableViewStr, expectedRegExTitle, section, atLeastOneText):
    try:
        tableView = findObject(tableViewStr) # waitForObject does not work - visible is 0?
        model = tableView.model()

        children = [ch for ch in object.children(tableView) if className(ch) == 'QModelIndex']
        for child in children:
            match = re.match(expectedRegExTitle, str(model.data(child).toString()))
            if match:
                test.passes(atLeastOneText, "Found '%s'" % match.group())
                return
        test.fail("No %s are displayed on Welcome page (%s)" % (section.lower(), section))
    except:
        test.fail("Failed to get tableview to check content of Welcome page (%s)" % section)

def main():
    # open Qt Creator
    startQC()
    if not startedWithoutPluginError():
        return

    setFixedHelpViewer(HelpViewer.HELPMODE)
    addCurrentCreatorDocumentation()

    buttonsAndState = {'Projects':False, 'Examples':True, 'Tutorials':False}
    for button, state in buttonsAndState.items():
        wsButton = getWelcomeScreenSideBarButton(button)
        if test.verify(object.exists(wsButton),
                       "Verified whether '%s' button is shown." % button):
            test.compare(buttonActive(wsButton), state,
                         "Verifying whether '%s' button is active (%s)." % (button, state))

    # select Projects and roughly check this
    switchToSubMode('Projects')
    for button in ['Create Project...', 'Open Project...']:
        wsButton = getWelcomeScreenSideBarButton(button)
        if test.verify(object.exists(wsButton),
                       "Verified whether '%s' button is shown." % button):
            test.verify(not buttonActive(wsButton),
                        "Verifying whether '%s' button is inactive." % button)

    # select Examples and roughly check them
    switchToSubMode('Examples')
    test.verify(waitForButtonsState(False, True, False), "Buttons' states have changed.")

    expect = (("QListView", "unnamed='1' visible='1' window=':Qt Creator_Core::Internal::MainWindow'",
               "examples list"),
              ("QLineEdit", "placeholderText='Search in Examples...'", "examples search line edit"),
              ("QComboBox", "currentText~='.*Qt.*'", "Qt version combo box"))
    search = "{type='%s' %s}"
    for (qType, prop, info) in expect:
        test.verify(checkIfObjectExists(search % (qType, prop)),
                    "Verifying whether %s is shown" % info)
    checkTableViewForContent(search % (expect[0][0], expect[0][1]), ".*Example", "Examples",
                             "Verifying that at least one example is displayed.")

    # select Tutorials and roughly check them
    switchToSubMode('Tutorials')
    test.verify(waitForButtonsState(False, False, True), "Buttons' states have changed.")
    expect = (("QListView", "unnamed='1' visible='1' window=':Qt Creator_Core::Internal::MainWindow'",
               "tutorials list"),
              ("QLineEdit", "placeholderText='Search in Tutorials...'",
               "tutorials search line edit"))
    for (qType, prop, info) in expect:
        test.verify(checkIfObjectExists(search % (qType, prop)),
                    "Verifying whether %s is shown" % info)
    checkTableViewForContent(search % (expect[0][0], expect[0][1]), "^Qt .*", "Tutorials",
                             "Verifying that at least one tutorial is displayed.")
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
