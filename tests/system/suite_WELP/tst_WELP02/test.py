source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

def main():
    # prepare example project
    sourceExample = os.path.join(sdkPath, "Examples", "4.7", "declarative", "animation", "basics",
                                 "property-animation")
    if not neededFilePresent(sourceExample):
        return
    # open Qt Creator
    startApplication("qtcreator" + SettingsPath)
    if not test.verify(checkIfObjectExists(getQmlItem("Text", ":Qt Creator_QDeclarativeView", False,
                                                      "text='Getting Started'")),
                       "Verifying: Qt Creator displays Welcome Page with Getting Started."):
        mouseClick(waitForObject(getQmlItem("LinkedText", ":Qt Creator_QDeclarativeView", False,
                                            "text='Getting Started'")), 5, 5, 0, Qt.LeftButton)
    # select "Develop" topic
    mouseClick(waitForObject(getQmlItem("LinkedText", ":Qt Creator_QDeclarativeView", False,
                                        "text='Develop'")), 5, 5, 0, Qt.LeftButton)
    sessionsText = getQmlItem("Text", ":Qt Creator_QDeclarativeView", False, "text='Sessions'")
    recentProjText = getQmlItem("Text", ":Qt Creator_QDeclarativeView", False,
                                "text='Recent Projects'")
    test.verify(checkIfObjectExists(sessionsText) and checkIfObjectExists(recentProjText),
                "Verifying: 'Develop' with 'Recently used sessions' and "
                "'Recently used Projects' is being opened.")
    # select "Create Project" and try to create a new project.
    # create Qt Quick application from "Welcome" page -> "Develop" tab
    createNewQtQuickApplication(tempDir(), "SampleApp", fromWelcome = True)
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}",
                  "sourceFilesRefreshed(QStringList)")
    test.verify(checkIfObjectExists("{column='0' container=':Qt Creator_Utils::NavigationTreeView'"
                                    " text~='SampleApp( \(.*\))?' type='QModelIndex'}"),
                "Verifying: The project is opened in 'Edit' mode after configuring.")
    # go to "Welcome page" -> "Develop" topic again.
    switchViewTo(ViewConstants.WELCOME)
    test.verify(checkIfObjectExists(sessionsText) and checkIfObjectExists(recentProjText),
                "Verifying: 'Develop' with 'Sessions' and 'Recent Projects' is opened.")
    # select "Open project" and select any project.
    # copy example project to temp directory
    examplePath = os.path.join(prepareTemplate(sourceExample), "propertyanimation.pro")
    # open example project from "Welcome" page -> "Develop" tab
    openQmakeProject(examplePath, fromWelcome = True)
    waitForSignal("{type='CppTools::Internal::CppModelManager' unnamed='1'}",
                  "sourceFilesRefreshed(QStringList)")
    test.verify(checkIfObjectExists("{column='0' container=':Qt Creator_Utils::NavigationTreeView'"
                                    " text~='propertyanimation( \(.*\))?' type='QModelIndex'}"),
                "Verifying: The project is opened in 'Edit' mode after configuring.")
    # go to "Welcome page" -> "Develop" again and check if there is an information about
    # recent projects in "Recent Projects".
    switchViewTo(ViewConstants.WELCOME)
    test.verify(checkIfObjectExists(getQmlItem("LinkedText", ":Qt Creator_QDeclarativeView", False,
                                               "text='propertyanimation'")) and
                checkIfObjectExists(getQmlItem("LinkedText", ":Qt Creator_QDeclarativeView", False,
                                               "text='SampleApp'")),
                "Verifying: 'Develop' is opened and the recently created and "
                "opened project can be seen in 'Recent Projects'.")
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
