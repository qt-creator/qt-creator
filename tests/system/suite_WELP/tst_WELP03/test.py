source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

webPageContentLoadedValue = 0

# wait until help gets loaded
def webPageContentLoaded(obj, param):
    global webPageContentLoadedValue
    objectClass = str(obj.metaObject().className())
    if objectClass in ("QWebPage", "Help::Internal::HelpViewer"):
        webPageContentLoadedValue += 1

def isLoaded():
    if platform.system() == "Darwin":
        return webPageContentLoadedValue == 1
    else:
        return webPageContentLoadedValue == 3

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

def main():
    global webPageContentLoadedValue
    if platform.system() in ('Microsoft', 'Windows'):
        test.log("Test case is currently disabled on Windows because it constantly crashes the AUT")
        return
    # open Qt Creator
    startApplication("qtcreator" + SettingsPath)
    installLazySignalHandler(":QWebPage","loadFinished(bool)", "webPageContentLoaded")
    installLazySignalHandler(":*Qt Creator_Help::Internal::HelpViewer", "loadFinished(bool)",
                             "webPageContentLoaded")
    addHelpDocumentationFromSDK()
    setAlwaysStartFullHelp()
    if not test.verify(checkIfObjectExists(getQmlItem("Text", ":Qt Creator_QDeclarativeView", False,
                                                      "text='Getting Started'")),
                       "Verifying: Qt Creator displays Welcome Page with Getting Started."):
        mouseClick(waitForObject(getQmlItem("LinkedText", ":Qt Creator_QDeclarativeView", False,
                                            "text='Getting Started'")), 5, 5, 0, Qt.LeftButton)
    # select "Examples" topic
    mouseClick(waitForObject(getQmlItem("LinkedText", ":Qt Creator_QDeclarativeView", False,
                                        "text='Examples'")), 5, 5, 0, Qt.LeftButton)
    test.verify(checkIfObjectExists(getQmlItem("Text", ":Qt Creator_QDeclarativeView", False,
                                               "text='Examples'")),
                "Verifying: 'Examples' topic is opened and the examples are shown.")
    # select an example and run example
    webPageContentLoadedValue = 0
    cleanUpUserFiles(sdkPath + "/Examples/4.7/opengl/2dpainting/2dpainting.pro")
    removePackagingDirectory(sdkPath + "/Examples/4.7/opengl/2dpainting")
    mouseClick(waitForObject(getQmlItem("Text", ":Qt Creator_QDeclarativeView", False,
                                        "text='Search in Examples...'")), 5, 5, 0, Qt.LeftButton)
    searchTutsAndExmpl = getQmlItem("TextInput", ":Qt Creator_QDeclarativeView", False)
    replaceEditorContent(waitForObject(searchTutsAndExmpl), "qwerty")
    test.verify(checkIfObjectExists("{clip='true' container=':Qt Creator_QDeclarativeView' "
                                    "enabled='true' id='captionItem' type='Text' unnamed='1' "
                                    "visible='true'}", False),
                "Verifying: 'Tutorials' topic is opened and nothing is shown.")
    replaceEditorContent(waitForObject(searchTutsAndExmpl), "2d painting")
    twoDPainting = getQmlItem("Text", ":Qt Creator_QDeclarativeView", True, "text~='2D Painting.*'")
    test.verify(checkIfObjectExists(twoDPainting),
                "Verifying: Text and Video tutorials are shown.")
    mouseClick(waitForObject(twoDPainting), 5, 5, 0, Qt.LeftButton)
    handlePackagingMessageBoxes()
    waitFor("isLoaded()", 5000)
    test.verify("2D Painting Example" in str(waitForObject(":Qt Creator_HelpSelector_QComboBox").currentText),
                "Verifying: The example application is opened.")
    switchViewTo(ViewConstants.EDIT)
    test.verify(checkIfObjectExists("{column='0' container=':Qt Creator_Utils::NavigationTreeView'"
                                    " text='2dpainting' type='QModelIndex'}"),
                "Verifying: The project is shown in 'Edit' mode.")
    openItemContextMenu(waitForObject(":Qt Creator_Utils::NavigationTreeView"),
                        "2dpainting", 5, 5, 0)
    activateItem(waitForObjectItem(":Qt Creator.Project.Menu.Project_QMenu",
                                   'Close Project "2dpainting"'))
    # close example and go to "Welcome" page -> "Examples" again and choose another example
    webPageContentLoadedValue = 0
    switchViewTo(ViewConstants.WELCOME)
    cleanUpUserFiles(sdkPath + "/Examples/4.7/itemviews/addressbook/addressbook.pro")
    removePackagingDirectory(sdkPath + "/Examples/4.7/itemviews/addressbook")
    replaceEditorContent(waitForObject(searchTutsAndExmpl),
                         "address book")
    addressBook = getQmlItem("Text", ":Qt Creator_QDeclarativeView", True, "text~='Address Book.*'")
    test.verify(checkIfObjectExists(addressBook),
                "Verifying: Text and Video tutorials are shown.")
    mouseClick(waitForObject(addressBook), 5, 5, 0, Qt.LeftButton)
    handlePackagingMessageBoxes()
    waitFor("isLoaded()", 5000)
    test.verify("Address Book Example" in str(waitForObject(":Qt Creator_HelpSelector_QComboBox").currentText),
                "Verifying: First example is closed and another application is opened.")
    # close second example application
    switchViewTo(ViewConstants.EDIT)
    test.verify(checkIfObjectExists("{column='0' container=':Qt Creator_Utils::NavigationTreeView'"
                                    " text='propertyanimation' type='QModelIndex'}", False) and
                checkIfObjectExists("{column='0' container=':Qt Creator_Utils::NavigationTreeView'"
                                    " text='addressbook' type='QModelIndex'}"),
                "Verifying: The project is shown in 'Edit' mode while old project isn't.")
    openItemContextMenu(waitForObject(":Qt Creator_Utils::NavigationTreeView"),
                        "addressbook", 5, 5, 0)
    activateItem(waitForObjectItem(":Qt Creator.Project.Menu.Project_QMenu",
                                   'Close Project "addressbook"'))
    navTree = waitForObject(":Qt Creator_Utils::NavigationTreeView")
    res = waitFor("navTree.model().rowCount(navTree.rootIndex()) == 0", 2000)
    test.verify(not checkIfObjectItemExists(":Qt Creator_Utils::NavigationTreeView", "addressbook"),
                "Verifying: The second example is closed.")
    # clean up created packaging directories
    removePackagingDirectory(sdkPath + "/Examples/4.7/opengl/2dpainting")
    removePackagingDirectory(sdkPath + "/Examples/4.7/itemviews/addressbook")
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
