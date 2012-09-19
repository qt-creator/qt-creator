source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

def main():
    # open Qt Creator
    startApplication("qtcreator" + SettingsPath)
    if not test.verify(checkIfObjectExists(getQmlItem("Text", ":Qt Creator_QDeclarativeView", False,
                                                      "text='Getting Started'")),
                       "Verifying: Qt Creator displays Welcome Page with Getting Started."):
        mouseClick(waitForObject(getQmlItem("LinkedText", ":Qt Creator_QDeclarativeView", False,
                                            "text='Getting Started'")), 5, 5, 0, Qt.LeftButton)
    # select "Tutorials" topic
    mouseClick(waitForObject(getQmlItem("LinkedText", ":Qt Creator_QDeclarativeView", False,
                                        "text='Tutorials'")), 5, 5, 0, Qt.LeftButton)
    mouseClick(waitForObject(getQmlItem("Text", ":Qt Creator_QDeclarativeView", False,
                                        "text='Search in Tutorials...'")), 5, 5, 0, Qt.LeftButton)
    searchTutsAndExmpl = getQmlItem("TextInput", ":Qt Creator_QDeclarativeView", False)
    replaceEditorContent(waitForObject(searchTutsAndExmpl), "qwerty")
    test.verify(checkIfObjectExists(getQmlItem("Text", ":Qt Creator_QDeclarativeView", False,
                                               "text='Tutorials'")) and
                checkIfObjectExists("{clip='true' container=':Qt Creator_QDeclarativeView' "
                                    "enabled='true' id='captionItem' type='Text' unnamed='1' "
                                    "visible='true'}", False),
                "Verifying: 'Tutorials' topic is opened and nothing is shown.")
    replaceEditorContent(waitForObject(searchTutsAndExmpl),
                         "building and running an example application")
    bldRunExmpl = getQmlItem("Text", ":Qt Creator_QDeclarativeView", True,
                             "text='Building and Running an Example Application'")
    test.verify(checkIfObjectExists(bldRunExmpl), "Verifying: Text and Video tutorials are shown.")
    # select a text tutorial
    mouseClick(waitForObject(bldRunExmpl), 5, 5, 0, Qt.LeftButton)
    test.verify(checkIfObjectExists(":Qt Creator.Go to Help Mode_QToolButton") and
                checkIfObjectExists(":DebugModeWidget.Debugger Toolbar_QDockWidget", False) and
                checkIfObjectExists(":Qt Creator.Analyzer Toolbar_QDockWidget", False),
                "Verifying: The tutorial is opened and located to the right part. "
                "The view is in 'Edit' mode.")
    # go to "Welcome" page -> "Tutorials" topic again and check the video tutorial link
    switchViewTo(ViewConstants.WELCOME)
    replaceEditorContent(waitForObject(searchTutsAndExmpl), "meet qt quick")
    test.verify(checkIfObjectExists(getQmlItem("Text", ":Qt Creator_QDeclarativeView", True,
                                               "text='Meet Qt Quick'")),
                "Verifying: Link to the video tutorial exists.")
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
