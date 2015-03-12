#############################################################################
##
## Copyright (C) 2015 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company.  For licensing terms and
## conditions see http://www.qt.io/terms-conditions.  For further information
## use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file.  Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, The Qt Company gives you certain additional
## rights.  These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")

class Qt5Path:
    DOCS = 0
    EXAMPLES = 1

    @staticmethod
    def getPaths(pathSpec):
        if pathSpec == Qt5Path.DOCS:
            path52 = "/doc"
            path53 = "/Docs/Qt-5.3"
            path54 = "/Docs/Qt-5.4"
        elif pathSpec == Qt5Path.EXAMPLES:
            path52 = "/examples"
            path53 = "/Examples/Qt-5.3"
            path54 = "/Examples/Qt-5.4"
        else:
            test.fatal("Unknown pathSpec given: %s" % str(pathSpec))
            return []
        if platform.system() in ('Microsoft', 'Windows'):
            return ["C:/Qt/Qt5.2.1/5.2.1/msvc2010" + path52,
                    "C:/Qt/Qt5.3.1" + path53, "C:/Qt/Qt5.4.1" + path54]
        elif platform.system() == 'Linux':
            if __is64BitOS__():
                return map(os.path.expanduser, ["~/Qt5.2.1/5.2.1/gcc_64" + path52,
                                                "~/Qt5.3.1" + path53, "~/Qt5.4.1" + path54])
            return map(os.path.expanduser, ["~/Qt5.2.1/5.2.1/gcc" + path52,
                                            "~/Qt5.3.1" + path53, "~/Qt5.4.1" + path54])
        else:
            return map(os.path.expanduser, ["~/Qt5.2.1/5.2.1/clang_64" + path52,
                                            "~/Qt5.3.1" + path53])

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
    if not canTestEmbeddedQtQuick():
        test.log("Welcome mode is not scriptable with this Squish version")
        return
    global sdkPath
    # open Qt Creator
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    qchs = [os.path.join(sdkPath, "Documentation", "qt.qch")]
    for p in Qt5Path.getPaths(Qt5Path.DOCS):
        qchs.extend([os.path.join(p, "qtopengl.qch"), os.path.join(p, "qtwidgets.qch")])
    addHelpDocumentation(qchs)
    setAlwaysStartFullHelp()
    getStartedNow = getQmlItem("Button", ":WelcomePageStyledBar.WelcomePage_QQuickView", False,
                               "text='Get Started Now' id='gettingStartedButton'")
    if not test.verify(checkIfObjectExists(getStartedNow),
                       "Verifying: Qt Creator displays Welcome Page with Getting Started."):
        test.fatal("Something's wrong - leaving test.")
        invokeMenuItem("File", "Exit")
        return
    # select "Examples" topic
    mouseClick(waitForObject(getQmlItem("Button", ":WelcomePageStyledBar.WelcomePage_QQuickView",
                                        False, "text='Examples'")), 5, 5, 0, Qt.LeftButton)
    test.verify(checkIfObjectExists(getQmlItem("Text",
                                               ":WelcomePageStyledBar.WelcomePage_QQuickView",
                                               False, "text='Examples'")),
                "Verifying: 'Examples' topic is opened and the examples are shown.")
    basePath = "opengl/2dpainting/2dpainting.pro"
    qt4Exmpl = os.path.join(sdkPath, "Examples", "4.7", basePath)
    qt5Exmpls = []
    for p in Qt5Path.getPaths(Qt5Path.EXAMPLES):
        qt5Exmpls.append(os.path.join(p, basePath))
    proFiles = [qt4Exmpl]
    proFiles.extend(qt5Exmpls)
    cleanUpUserFiles(proFiles)
    for p in proFiles:
        removePackagingDirectory(os.path.dirname(p))
    examplesLineEdit = getQmlItem("TextField", ":WelcomePageStyledBar.WelcomePage_QQuickView",
                                  False, "id='lineEdit' placeholderText='Search in Examples...'")
    mouseClick(waitForObject(examplesLineEdit), 5, 5, 0, Qt.LeftButton)
    test.log("Using examples from Kit %s."
             % (waitForObject(getQmlItem("ComboBox", ":WelcomePageStyledBar.WelcomePage_QQuickView",
                                         False, "id='comboBox'")).currentText))
    replaceEditorContent(waitForObject(examplesLineEdit), "qwerty")
    test.verify(checkIfObjectExists(getQmlItem("Delegate",
                                               ":WelcomePageStyledBar.WelcomePage_QQuickView",
                                               False, "id='delegate' radius='0' caption~='.*'"),
                                    False), "Verifying: No example is shown.")
    replaceEditorContent(waitForObject(examplesLineEdit), "2d painting")
    twoDPainting = getQmlItem("Delegate", ":WelcomePageStyledBar.WelcomePage_QQuickView",
                              False, "id='delegate' radius='0' caption~='2D Painting.*'")
    test.verify(checkIfObjectExists(twoDPainting),
                "Verifying: Example (2D Painting) is shown.")
    mouseClick(waitForObject(twoDPainting), 5, 5, 0, Qt.LeftButton)
    handlePackagingMessageBoxes()
    helpWidget = waitForObject(":Help Widget_Help::Internal::HelpWidget")
    test.verify(waitFor('"2D Painting Example" in str(helpWidget.windowTitle)', 5000),
                "Verifying: The example application is opened inside Help.")
    sendEvent("QCloseEvent", helpWidget)
    # assume the correct kit is selected, hit Configure Project
    clickButton(waitForObject("{text='Configure Project' type='QPushButton' unnamed='1' visible='1'"
                              "window=':Qt Creator_Core::Internal::MainWindow'}"))
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
    basePath = "itemviews/addressbook/addressbook.pro"
    qt4Exmpl = os.path.join(sdkPath, "Examples", "4.7", basePath)
    qt5Exmpls = []
    for p in Qt5Path.getPaths(Qt5Path.EXAMPLES):
        qt5Exmpls.append(os.path.join(p, "widgets", basePath))
    proFiles = [qt4Exmpl]
    proFiles.extend(qt5Exmpls)
    cleanUpUserFiles(proFiles)
    for p in proFiles:
        removePackagingDirectory(os.path.dirname(p))
    replaceEditorContent(waitForObject(examplesLineEdit), "address book")
    addressBook = getQmlItem("Delegate", ":WelcomePageStyledBar.WelcomePage_QQuickView",
                              False, "id='delegate' radius='0' caption~='Address Book.*'")
    test.verify(checkIfObjectExists(addressBook), "Verifying: Example (address book) is shown.")
    mouseClick(waitForObject(addressBook), 5, 5, 0, Qt.LeftButton)
    handlePackagingMessageBoxes()
    helpWidget = waitForObject(":Help Widget_Help::Internal::HelpWidget")
    test.verify(waitFor('"Address Book Example" in str(helpWidget.windowTitle)', 5000),
                "Verifying: The example application is opened inside Help.")
    sendEvent("QCloseEvent", helpWidget)
    # assume the correct kit is selected, hit Configure Project
    clickButton(waitForObject("{text='Configure Project' type='QPushButton' unnamed='1' visible='1'"
                              "window=':Qt Creator_Core::Internal::MainWindow'}"))
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
