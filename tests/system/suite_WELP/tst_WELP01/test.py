# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

getStarted = 'Get Started'

def clickItemVerifyHelpCombo(button, expectedHelpComboRegex, testDetails):
    global getStarted
    mouseClick(button)
    helpCombo = waitForObject(":Qt Creator_HelpSelector_QComboBox")
    if not test.verify(waitFor('re.match(expectedHelpComboRegex, str(helpCombo.currentText))',
                               5000), testDetails):
        test.log("Found %s" % str(helpCombo.currentText))
    # select "Welcome" page from left toolbar again
    switchViewTo(ViewConstants.WELCOME)
    wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton(getStarted)
    return test.verify(all((wsButtonFrame, wsButtonLabel)),
                       "Verifying: '%s' button is being displayed." % getStarted)
def buttonActive(button):
    # colors of the default theme for active button on Welcome page
    defaultActiveRGB = (69, 206, 85)
    # colors of the dark theme for active button on Welcome page
    darkActiveRGB = (54, 193, 72)
    # QPalette::Window (used background color of Welcome page buttons)
    enumQPaletteWindow = 10
    color = button.palette.color(enumQPaletteWindow)
    current = (color.red, color.green, color.blue)
    return current == defaultActiveRGB or current == darkActiveRGB

def waitForButtonsState(projectsActive, examplesActive, tutorialsActive, timeout=5000):
    projButton = getWelcomeScreenSideBarButton('Projects')[0]
    exmpButton = getWelcomeScreenSideBarButton('Examples')[0]
    tutoButton = getWelcomeScreenSideBarButton('Tutorials')[0]
    if not all((projButton, exmpButton, tutoButton)):
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
    global getStarted
    # open Qt Creator
    startQC()
    if not startedWithoutPluginError():
        return

    setFixedHelpViewer(HelpViewer.HELPMODE)
    addCurrentCreatorDocumentation()

    buttonsAndState = {'Projects':False, 'Examples':True, 'Tutorials':False}
    for button, state in buttonsAndState.items():
        wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton(button)
        if test.verify(all((wsButtonFrame, wsButtonLabel)),
                       "Verified whether '%s' button is shown." % button):
            test.compare(buttonActive(wsButtonFrame), state,
                         "Verifying whether '%s' button is active (%s)." % (button, state))

    # select Projects and roughly check this
    switchToSubMode('Projects')
    for button in ['Create Project...', 'Open Project...']:
        wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton(button)
        if test.verify(all((wsButtonFrame, wsButtonLabel)),
                       "Verified whether '%s' button is shown." % button):
            test.verify(not buttonActive(wsButtonFrame),
                        "Verifying whether '%s' button is inactive." % button)

    wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton(getStarted)
    if test.verify(all((wsButtonFrame, wsButtonLabel)),
                   "Verifying: Qt Creator displays Welcome Page with '%s' button." % getStarted):
        if clickItemVerifyHelpCombo(wsButtonLabel, "Getting Started | Qt Creator Manual",
                                    "Verifying: Help with Creator Documentation is being opened."):

            textUrls = {'Online Community':'https://forum.qt.io',
                        'Blogs':'https://planet.qt.io',
                        'Qt Account':'https://account.qt.io',
                        'User Guide':'qthelp://org.qt-project.qtcreator/doc/index.html'
                        }
            for text, url in textUrls.items():
                button, label = getWelcomeScreenBottomButton(text)
                if test.verify(all((button, label)),
                               "Verifying whether link button (%s) exists." % text):
                    test.compare(str(button.toolTip), url, "Verifying URL for %s" % text)
    wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton(getStarted)
    if wsButtonLabel is not None:
        mouseClick(wsButtonLabel)
        qcManualQModelIndexStr = getQModelIndexStr("text~='Qt Creator Manual [0-9.]+'",
                                                   ":Qt Creator_QHelpContentWidget")
        if str(waitForObject(":Qt Creator_HelpSelector_QComboBox").currentText) == "(Untitled)":
            mouseClick(qcManualQModelIndexStr)
            test.warning("Clicking '%s' the second time showed blank page (Untitled)" % getStarted)
    else:
        test.fatal("Something's wrong - failed to find/click '%s' the second time." % getStarted)

    # select "Welcome" page from left toolbar again
    switchViewTo(ViewConstants.WELCOME)
    wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton(getStarted)
    test.verify(wsButtonFrame is not None and wsButtonLabel is not None,
                "Verifying: Getting Started topic is being displayed.")
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
    checkTableViewForContent(search % (expect[0][0], expect[0][1]), "Creating .*", "Tutorials",
                             "Verifying that at least one tutorial is displayed.")
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
