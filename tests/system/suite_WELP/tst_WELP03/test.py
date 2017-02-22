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
source("../../shared/suites_qtta.py")

def handlePackagingMessageBoxes():
    if platform.system() == "Darwin":
        messageBox = "{type='QMessageBox' unnamed='1' visible='1'}"
    else:
        messageBox = ("{type='QMessageBox' unnamed='1' visible='1' "
                      "windowTitle='Add Packaging Files to Project'}")
    while (True):
        try:
            waitForObject(messageBox, 3000)
            clickButton(waitForObject("{text='Yes' type='QPushButton' unnamed='1' visible='1' "
                                      "window=%s}" % messageBox))
        except:
            break

def openExample(examplesLineEdit, input, exampleRegex, exampleName):
    replaceEditorContent(examplesLineEdit, input)
    tableView = waitForObject("{type='QTableView' unnamed='1' visible='1' "
                              "window=':Qt Creator_Core::Internal::MainWindow'}")
    waitFor('findExampleOrTutorial(tableView, exampleRegex) is not None', 3000)
    example = findExampleOrTutorial(tableView, exampleRegex, True)
    if test.verify(example is not None, "Verifying: Example (%s) is shown." % exampleName):
        mouseClick(example)
        handlePackagingMessageBoxes()
        helpWidget = waitForObject(":Help Widget_Help::Internal::HelpWidget")
        test.verify(waitFor('exampleName in str(helpWidget.windowTitle)', 5000),
                    "Verifying: The example application is opened inside Help.")
        sendEvent("QCloseEvent", helpWidget)
        # assume the correct kit is selected, hit Configure Project
        clickButton(waitForObject(":Qt Creator.Configure Project_QPushButton"))
    return example

def main():
    # open Qt Creator
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    qchs = []
    for p in Qt5Path.getPaths(Qt5Path.DOCS):
        qchs.extend([os.path.join(p, "qtopengl.qch"), os.path.join(p, "qtwidgets.qch")])
    addHelpDocumentation(qchs)
    setAlwaysStartFullHelp()
    wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton('Get Started Now')
    if not test.verify(all((wsButtonFrame, wsButtonLabel)),
                       "Verifying: Qt Creator displays Welcome Page with Getting Started."):
        test.fatal("Something's wrong - leaving test.")
        invokeMenuItem("File", "Exit")
        return
    # select "Examples" topic
    wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton('Examples')
    if all((wsButtonFrame, wsButtonLabel)):
        mouseClick(wsButtonLabel)
    expect = (("QTableView", "unnamed='1' visible='1' window=':Qt Creator_Core::Internal::MainWindow'",
               "examples list"),
              ("QLineEdit", "placeholderText='Search in Examples...'", "examples search line edit"),
              ("QComboBox", "text~='.*Qt.*' visible='1'", "Qt version combo box"))
    search = "{type='%s' %s}"
    test.verify(all(map(checkIfObjectExists, (search % (exp[0], exp[1]) for exp in expect))),
                "Verifying: 'Examples' topic is opened and the examples are shown.")

    examplesLineEdit = waitForObject(search % (expect[1][0], expect[1][1]))
    mouseClick(examplesLineEdit)
    combo = waitForObject(search % (expect[2][0], expect[2][1]))
    test.log("Using examples from Kit %s." % str(combo.currentText))
    replaceEditorContent(examplesLineEdit, "qwerty")
    tableView = waitForObject(search % (expect[0][0], expect[0][1]))
    waitFor('findExampleOrTutorial(tableView, ".*") is None', 3000)
    example = findExampleOrTutorial(tableView, ".*", True)
    test.verify(example is None, "Verifying: No example is shown.")

    proFiles = map(lambda p: os.path.join(p, "opengl", "2dpainting", "2dpainting.pro"),
                   Qt5Path.getPaths(Qt5Path.EXAMPLES))
    cleanUpUserFiles(proFiles)
    for p in proFiles:
        removePackagingDirectory(os.path.dirname(p))

    example = openExample(examplesLineEdit, "2d painting", "2D Painting.*", "2D Painting Example")
    if example is not None:
        test.verify(checkIfObjectExists("{column='0' container=':Qt Creator_Utils::NavigationTreeView'"
                                        " text='2dpainting' type='QModelIndex'}"),
                    "Verifying: The project is shown in 'Edit' mode.")
        invokeContextMenuOnProject('2dpainting', 'Close Project "2dpainting"')
        navTree = waitForObject(":Qt Creator_Utils::NavigationTreeView")
        res = waitFor("navTree.model().rowCount(navTree.rootIndex()) == 0", 2000)
        test.verify(not checkIfObjectItemExists(":Qt Creator_Utils::NavigationTreeView", "2dpainting"),
                    "Verifying: The first example is closed.")
    # clean up created packaging directories
    for p in proFiles:
        removePackagingDirectory(os.path.dirname(p))

    # go to "Welcome" page and choose another example
    switchViewTo(ViewConstants.WELCOME)
    proFiles = map(lambda p: os.path.join(p, "widgets", "itemviews", "addressbook", "addressbook.pro"),
                   Qt5Path.getPaths(Qt5Path.EXAMPLES))
    cleanUpUserFiles(proFiles)
    for p in proFiles:
        removePackagingDirectory(os.path.dirname(p))
    examplesLineEdit = waitForObject(search %(expect[1][0], expect[1][1]))
    example = openExample(examplesLineEdit, "address book", "Address Book.*",
                          "Address Book Example")
    if example is not None:
        # close second example application
        test.verify(checkIfObjectExists("{column='0' container=':Qt Creator_Utils::NavigationTreeView'"
                                        " text='propertyanimation' type='QModelIndex'}", False) and
                    checkIfObjectExists("{column='0' container=':Qt Creator_Utils::NavigationTreeView'"
                                        " text='addressbook' type='QModelIndex'}"),
                    "Verifying: The project is shown in 'Edit' mode while old project isn't.")
        invokeContextMenuOnProject('addressbook', 'Close Project "addressbook"')
        navTree = waitForObject(":Qt Creator_Utils::NavigationTreeView")
        res = waitFor("navTree.model().rowCount(navTree.rootIndex()) == 0", 2000)
        test.verify(not checkIfObjectItemExists(":Qt Creator_Utils::NavigationTreeView", "addressbook"),
                    "Verifying: The second example is closed.")
    # clean up created packaging directories
    for p in proFiles:
        removePackagingDirectory(os.path.dirname(p))
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
