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

# test Qt Creator version information from file and dialog
def getQtCreatorVersionFromDialog():
    chk = re.search("(?<=Qt Creator)\s\d+.\d+.\d+",
                    str(waitForObject("{text?='*Qt Creator*' type='QLabel' unnamed='1' visible='1' "
                                      "window=':About Qt Creator_Core::Internal::VersionDialog'}").text))
    try:
        ver = chk.group(0).strip()
        return ver
    except:
        test.fail("Failed to get the exact version from Dialog")
        return ""

def getQtCreatorVersionFromFile():
    qtCreatorPriFileName = "../../../../qtcreator.pri"
    # open file <qtCreatorPriFileName> and read version
    fileText = readFile(qtCreatorPriFileName)
    chk = re.search("(?<=QTCREATOR_VERSION =)\s\d+.\d+.\d+", fileText)
    try:
        ver = chk.group(0).strip()
        return ver
    except:
        test.fail("Failed to get the exact version from File")
        return ""

def checkQtCreatorHelpVersion(expectedVersion):
    switchViewTo(ViewConstants.HELP)
    try:
        helpContentWidget = waitForObject(':Qt Creator_QHelpContentWidget', 5000)
        items = dumpItems(helpContentWidget.model())
        test.compare(filter(lambda x: x.startswith('Qt Creator Manual'), items)[0],
                     'Qt Creator Manual %s' % expectedVersion,
                     'Verifying whether manual uses expected version.')
    except:
        test.xverify(False, "Missing Qt Creator Manual (QTCREATORBUG-13233).")

def setKeyboardShortcutForAboutQtC():
    invokeMenuItem("Tools", "Options...")
    waitForObjectItem(":Options_QListView", "Environment")
    clickItem(":Options_QListView", "Environment", 14, 15, 0, Qt.LeftButton)
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Keyboard")
    filter = waitForObject("{container={title='Keyboard Shortcuts' type='QGroupBox' unnamed='1' "
                           "visible='1'} type='Utils::FancyLineEdit' unnamed='1' visible='1' "
                           "placeHolderText='Filter'}")
    replaceEditorContent(filter, "about")
    treewidget = waitForObject("{type='QTreeWidget' unnamed='1' visible='1'}")
    modelIndex = waitForObject("{column='0' text='AboutQtCreator' type='QModelIndex' "
                               "container={column='0' text='QtCreator' type='QModelIndex' "
                               "container=%s}}" % objectMap.realName(treewidget))
    mouseClick(modelIndex, 5, 5, 0, Qt.LeftButton)
    shortcut = waitForObject("{container={title='Shortcut' type='QGroupBox' unnamed='1' "
                             "visible='1'} type='Utils::FancyLineEdit' unnamed='1' visible='1' "
                             "placeHolderText='Type to set shortcut'}")
    mouseClick(shortcut, 5, 5, 0, Qt.LeftButton)
    nativeType("<Ctrl+Alt+a>")
    clickButton(waitForObject(":Options.OK_QPushButton"))

def main():
    expectedVersion = getQtCreatorVersionFromFile()
    if not expectedVersion:
        test.fatal("Can't find version from file.")
        return
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    setKeyboardShortcutForAboutQtC()
    if platform.system() == 'Darwin':
        try:
            waitForObject(":Qt Creator.QtCreator.MenuBar_QMenuBar", 2000)
        except:
            nativeMouseClick(waitForObject(":Qt Creator_Core::Internal::MainWindow", 1000), 20, 20, 0, Qt.LeftButton)
    nativeType("<Ctrl+Alt+a>")
    # verify qt creator version
    try:
        waitForObject(":About Qt Creator_Core::Internal::VersionDialog", 5000)
    except:
        test.warning("Using workaround of invoking menu entry "
                     "(known issue when running on Win inside Jenkins)")
        if platform.system() == "Darwin":
            invokeMenuItem("Help", "About Qt Creator")
        else:
            invokeMenuItem("Help", "About Qt Creator...")
        waitForObject(":About Qt Creator_Core::Internal::VersionDialog", 5000)
    actualVersion = getQtCreatorVersionFromDialog()
    test.verify(actualVersion == expectedVersion,
                "Verifying version. Current version is '%s', expected version is '%s'"
                % (actualVersion, expectedVersion))
    # close and verify about dialog closed
    clickButton(waitForObject("{text='Close' type='QPushButton' unnamed='1' visible='1' "
                              "window=':About Qt Creator_Core::Internal::VersionDialog'}"))
    test.verify(checkIfObjectExists(":About Qt Creator_Core::Internal::VersionDialog", False),
                "Verifying if About dialog closed.")
    checkQtCreatorHelpVersion(expectedVersion)
    # exit qt creator
    invokeMenuItem("File", "Exit")
    # verify if qt creator closed properly
    test.verify(checkIfObjectExists(":Qt Creator_Core::Internal::MainWindow", False),
                "Verifying if Qt Creator closed.")
