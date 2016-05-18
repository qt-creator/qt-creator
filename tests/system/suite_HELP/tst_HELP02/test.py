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
    def rightStart(x):
        return x.startswith('Qt Creator Manual')

    switchViewTo(ViewConstants.HELP)
    try:
        helpContentWidget = waitForObject(':Qt Creator_QHelpContentWidget', 5000)
        waitFor("any(map(rightStart, dumpItems(helpContentWidget.model())))", 10000)
        items = dumpItems(helpContentWidget.model())
        test.compare(filter(rightStart, items)[0],
                     'Qt Creator Manual %s' % expectedVersion,
                     'Verifying whether manual uses expected version.')
    except:
        t, v = sys.exc_info()[:2]
        test.log("Exception caught", "%s(%s)" % (str(t), str(v)))
        test.fail("Missing Qt Creator Manual.")

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
    shortcutGB = "{title='Shortcut' type='QGroupBox' unnamed='1' visible='1'}"
    record = waitForObject("{container=%s type='Core::Internal::ShortcutButton' unnamed='1' "
                           "visible='1' text~='(Stop Recording|Record)'}" % shortcutGB)
    shortcut = ("{container=%s type='Utils::FancyLineEdit' unnamed='1' visible='1' "
                "placeHolderText='Enter key sequence as text'}" % shortcutGB)
    clickButton(record)
    nativeType("<Ctrl+Alt+a>")
    clickButton(record)
    expected = 'Ctrl+Alt+A'
    if platform.system() == 'Darwin':
        expected = 'Ctrl+Opt+A'
    test.verify(waitFor("str(findObject(shortcut).text) == expected", 5000),
                "Expected key sequence is displayed.")
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
