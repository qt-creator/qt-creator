# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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


def openExample(examplesLineEdit, input, exampleRegex, exampleName, waitForChildCount=0):
    replaceEditorContent(examplesLineEdit, input)
    listView = waitForObject("{type='QListView' name='AllItemsView' visible='1' "
                              "window=':Qt Creator_Core::Internal::MainWindow'}")
    filterModel = __childrenOfType__(listView, 'QSortFilterProxyModel')
    if len(filterModel) != 1:
        test.fatal("Failed to find filter proxy model.")
        return None

    filterModel = filterModel[0]
    if waitForChildCount > 0:
        waitFor("filterModel.rowCount() == waitForChildCount", 3000)

    waitFor('findExampleOrTutorial(listView, exampleRegex) is not None', 3000)
    example = findExampleOrTutorial(listView, exampleRegex, True)
    if test.verify(example is not None, "Verifying: Example (%s) is shown." % exampleName):
        mouseClick(waitForObjectItem(listView, str(example.text)))
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
    startQC()
    if not startedWithoutPluginError():
        return
    qchs = []
    for p in QtPath.getPaths(QtPath.DOCS):
        qchs.extend([os.path.join(p, "qtopengl.qch"), os.path.join(p, "qtwidgets.qch")])
    addHelpDocumentation(qchs)
    setFixedHelpViewer(HelpViewer.HELPMODE)
    wsButtonFrame, wsButtonLabel = getWelcomeScreenSideBarButton('Get Started')
    if not test.verify(all((wsButtonFrame, wsButtonLabel)),
                       "Verifying: Qt Creator displays Welcome Page with Getting Started."):
        test.fatal("Something's wrong - leaving test.")
        invokeMenuItem("File", "Exit")
        return
    # select "Examples" topic
    switchToSubMode('Examples')
    expect = (("QListView", "unnamed='1' visible='1' window=':Qt Creator_Core::Internal::MainWindow'",
               "examples list"),
              ("QLineEdit", "placeholderText='Search in Examples...'", "examples search line edit"),
              ("QComboBox", "currentText~='.*Qt.*' visible='1'", "Qt version combo box"))
    search = "{type='%s' %s}"
    test.verify(all(map(checkIfObjectExists, (search % (exp[0], exp[1]) for exp in expect))),
                "Verifying: 'Examples' topic is opened and the examples are shown.")

    examplesLineEdit = waitForObject(search % (expect[1][0], expect[1][1]))
    mouseClick(examplesLineEdit)
    combo = waitForObject(search % (expect[2][0], expect[2][1]))
    test.log("Using examples from Kit %s." % str(combo.currentText))
    replaceEditorContent(examplesLineEdit, "qwerty")
    listView = waitForObject("{type='QListView' name='AllItemsView'}")
    waitFor('findExampleOrTutorial(listView, ".*") is None', 3000)
    example = findExampleOrTutorial(listView, ".*", True)
    test.verify(example is None, "Verifying: No example is shown.")

    proFiles = [os.path.join(p, "opengl", "2dpainting", "2dpainting.pro")
                for p in QtPath.getPaths(QtPath.EXAMPLES)]
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
        waitFor("navTree.model().rowCount(navTree.rootIndex()) == 0", 2000)
        test.verify(not checkIfObjectItemExists(":Qt Creator_Utils::NavigationTreeView", "2dpainting"),
                    "Verifying: The first example is closed.")
    # clean up created packaging directories
    for p in proFiles:
        removePackagingDirectory(os.path.dirname(p))

    # go to "Welcome" page and choose another example
    switchViewTo(ViewConstants.WELCOME)
    proFiles = [os.path.join(p, "widgets", "itemviews", "addressbook", "addressbook.pro")
                for p in QtPath.getPaths(QtPath.EXAMPLES)]
    cleanUpUserFiles(proFiles)
    for p in proFiles:
        removePackagingDirectory(os.path.dirname(p))
    examplesLineEdit = waitForObject(search %(expect[1][0], expect[1][1]))
    example = openExample(examplesLineEdit, "address book", "(0000 )?Address Book.*",
                          "Address Book Example", 3)
    if example is not None:
        # close second example application
        test.verify(checkIfObjectExists("{column='0' container=':Qt Creator_Utils::NavigationTreeView'"
                                        " text='propertyanimation' type='QModelIndex'}", False) and
                    checkIfObjectExists("{column='0' container=':Qt Creator_Utils::NavigationTreeView'"
                                        " text='addressbook' type='QModelIndex'}"),
                    "Verifying: The project is shown in 'Edit' mode while old project isn't.")
        invokeContextMenuOnProject('addressbook', 'Close Project "addressbook"')
        navTree = waitForObject(":Qt Creator_Utils::NavigationTreeView")
        waitFor("navTree.model().rowCount(navTree.rootIndex()) == 0", 2000)
        test.verify(not checkIfObjectItemExists(":Qt Creator_Utils::NavigationTreeView", "addressbook"),
                    "Verifying: The second example is closed.")
    # clean up created packaging directories
    for p in proFiles:
        removePackagingDirectory(os.path.dirname(p))
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
