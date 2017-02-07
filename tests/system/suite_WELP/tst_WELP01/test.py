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

getStarted = 'Get Started Now'

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
    (activeRed, activeGreen, activeBlue) = (64, 66, 68)
    # QPalette::Window (used background color of Welcome page buttons)
    enumQPaletteWindow = 10
    color = button.palette.color(enumQPaletteWindow)
    return color.red == activeRed and color.green == activeGreen and color.blue == activeBlue

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
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return

    setAlwaysStartFullHelp()
    addCurrentCreatorDocumentation()

    buttonsAndState = {'Projects':True, 'Examples':False, 'Tutorials':False}
    for button, state in buttonsAndState.items():
        wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton(button)
        if test.verify(all((wsButtonFrame, wsButtonLabel)),
                       "Verified whether '%s' button is shown." % button):
            test.compare(buttonActive(wsButtonFrame), state,
                         "Verifying whether '%s' button is active (%s)." % (button, state))

    for button in ['New Project', 'Open Project']:
        wsButtonFrame, wsButtonLabel = getWelcomeScreenMainButton(button)
        if test.verify(all((wsButtonFrame, wsButtonLabel)),
                       "Verified whether '%s' button is shown." % button):
            test.verify(not buttonActive(wsButtonFrame),
                        "Verifying whether '%s' button is inactive." % button)

    wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton(getStarted)
    if test.verify(all((wsButtonFrame, wsButtonLabel)),
                   "Verifying: Qt Creator displays Welcome Page with '%s' button." % getStarted):
        if clickItemVerifyHelpCombo(wsButtonLabel, "Qt Creator Manual",
                                    "Verifying: Help with Creator Documentation is being opened."):

            textUrls = {'Online Community':'http://forum.qt.io',
                        'Blogs':'http://planet.qt.io',
                        'Qt Account':'https://account.qt.io',
                        'User Guide':'qthelp://org.qt-project.qtcreator/doc/index.html'
                        }
            for text, url in textUrls.items():
                test.verify(checkIfObjectExists("{type='QLabel' text='%s' unnamed='1' visible='1' "
                                                "window=':Qt Creator_Core::Internal::MainWindow'}"
                                                % text),
                            "Verifying whether link button (%s) exists." % text)
                # TODO find way to verify URLs (or tweak source code of Welcome page to become able)
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
    wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton('Examples')
    if all((wsButtonFrame, wsButtonLabel)):
        mouseClick(wsButtonLabel)
    test.verify(waitForButtonsState(False, True, False), "Buttons' states have changed.")

    expect = (("QTableView", "unnamed='1' visible='1' window=':Qt Creator_Core::Internal::MainWindow'",
               "examples list"),
              ("QLineEdit", "placeholderText='Search in Examples...'", "examples search line edit"),
              ("QComboBox", "text~='.*Qt.*'", "Qt version combo box"))
    search = "{type='%s' %s}"
    for (qType, prop, info) in expect:
        test.verify(checkIfObjectExists(search % (qType, prop)),
                    "Verifying whether %s is shown" % info)
    checkTableViewForContent(search % (expect[0][0], expect[0][1]), ".*Example", "Examples",
                             "Verifying that at least one example is displayed.")

    # select Tutorials and roughly check them
    wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton('Tutorials')
    if all((wsButtonFrame, wsButtonLabel)):
        mouseClick(wsButtonLabel)
    test.verify(waitForButtonsState(False, False, True), "Buttons' states have changed.")
    expect = (("QTableView", "unnamed='1' visible='1' window=':Qt Creator_Core::Internal::MainWindow'",
               "tutorials list"),
              ("QLineEdit", "placeholderText='Search in Tutorials...'",
               "tutorials search line edit"))
    for (qType, prop, info) in expect:
        test.verify(checkIfObjectExists(search % (qType, prop)),
                    "Verifying whether %s is shown" % info)
    checkTableViewForContent(search % (expect[0][0], expect[0][1]), "Creating.*", "Tutorials",
                             "Verifying that at least one tutorial is displayed.")
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
