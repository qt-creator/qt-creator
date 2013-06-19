#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

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

def qt5SDKPath():
    if platform.system() in ('Microsoft', 'Windows'):
        return os.path.abspath("C:/Qt/Qt5.0.1/5.0.1/msvc2010")
    elif platform.system() == 'Linux':
        if __is64BitOS__():
            return os.path.expanduser("~/Qt5.0.1/5.0.1/gcc_64")
        return os.path.expanduser("~/Qt5.0.1/5.0.1/gcc")
    else:
        return os.path.expanduser("~/Qt5.0.1/5.0.1/clang_64")

def main():
    global sdkPath, webPageContentLoadedValue
    # open Qt Creator
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    installLazySignalHandler(":QWebPage","loadFinished(bool)", "webPageContentLoaded")
    installLazySignalHandler(":*Qt Creator_Help::Internal::HelpViewer", "loadFinished(bool)",
                             "webPageContentLoaded")
    qt5sdkPath = qt5SDKPath()
    qchs = [os.path.join(sdkPath, "Documentation", "qt.qch"),
            os.path.join(qt5sdkPath, "doc", "qtopengl.qch"),
            os.path.join(qt5sdkPath, "doc", "qtwidgets.qch")]
    addHelpDocumentation(qchs)
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
    basePath = "opengl/2dpainting/2dpainting.pro"
    qt4Exmpl = os.path.join(sdkPath, "Examples", "4.7", basePath)
    qt5Exmpl = os.path.join(qt5sdkPath, "examples", basePath)
    cleanUpUserFiles([qt4Exmpl, qt5Exmpl])
    removePackagingDirectory(os.path.dirname(qt4Exmpl))
    removePackagingDirectory(os.path.dirname(qt5Exmpl))
    mouseClick(waitForObject(getQmlItem("Text", ":Qt Creator_QDeclarativeView", False,
                                        "text='Search in Examples...'")), 5, 5, 0, Qt.LeftButton)
    searchTutsAndExmpl = getQmlItem("TextInput", ":Qt Creator_QDeclarativeView", False)
    kitCombo = waitForObject("{clip='false' container=':Qt Creator_QDeclarativeView' enabled='true'"
                             " type='ChoiceList' unnamed='1' visible='true'}")
    test.log("Using examples from Kit %s." % (kitCombo.currentText))
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
    navTree = waitForObject(":Qt Creator_Utils::NavigationTreeView")
    res = waitFor("navTree.model().rowCount(navTree.rootIndex()) == 0", 2000)
    test.verify(not checkIfObjectItemExists(":Qt Creator_Utils::NavigationTreeView", "2dpainting"),
                "Verifying: The first example is closed.")
    # clean up created packaging directories
    removePackagingDirectory(os.path.dirname(qt4Exmpl))
    removePackagingDirectory(os.path.dirname(qt5Exmpl))

    # close example and go to "Welcome" page -> "Examples" again and choose another example
    webPageContentLoadedValue = 0
    switchViewTo(ViewConstants.WELCOME)
    basePath = "itemviews/addressbook/addressbook.pro"
    qt4Exmpl = os.path.join(sdkPath, "Examples", "4.7", basePath)
    qt5Exmpl = os.path.join(qt5sdkPath, "examples", "widgets", basePath)
    cleanUpUserFiles([qt4Exmpl, qt5Exmpl])
    removePackagingDirectory(os.path.dirname(qt4Exmpl))
    removePackagingDirectory(os.path.dirname(qt5Exmpl))
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
    removePackagingDirectory(os.path.dirname(qt4Exmpl))
    removePackagingDirectory(os.path.dirname(qt5Exmpl))
    # exit Qt Creator
    invokeMenuItem("File", "Exit")
