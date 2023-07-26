# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

if platform.system() == 'Darwin':
    keysToType = '<Command+Alt+a>'
    expectedKeys = 'Cmd+Opt+A'
else:
    keysToType = '<Ctrl+Alt+a>'
    expectedKeys = 'Ctrl+Alt+A'


# test Qt Creator version information from file and dialog
def getQtCreatorVersionFromDialog():
    chk = re.search("(?<=Qt Creator)\s\d+.\d+.\d+[-\w]*",
                    str(waitForObject("{text~='.*Qt Creator.*' type='QLabel' unnamed='1' visible='1' "
                                      "window=':About Qt Creator_Core::Internal::VersionDialog'}").text))
    try:
        ver = chk.group(0).strip()
        return ver
    except:
        test.fail("Failed to get the exact version from Dialog")
        return ""

def getQtCreatorVersionFromFile():
    qtCreatorPriFileName = "../../../../cmake/QtCreatorIDEBranding.cmake"
    # open file <qtCreatorPriFileName> and read version
    fileText = readFile(qtCreatorPriFileName)
    chk = re.search('(?<=set\(IDE_VERSION_DISPLAY ")\d+.\d+.\d+\S*(?="\))', fileText)
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
        test.compare(next(iter(filter(rightStart, items))),
                     'Qt Creator Manual %s' % expectedVersion,
                     'Verifying whether manual uses expected version.')
    except:
        t, v = sys.exc_info()[:2]
        test.log("Exception caught", "%s: %s" % (t.__name__, str(v)))
        test.fail("Missing Qt Creator Manual.")


def _shortcutMatches_(shortcutEdit, expectedText):
    return str(findObject(shortcutEdit).text) == expectedText


def setKeyboardShortcutForAboutQtC():
    invokeMenuItem("Edit", "Preferences...")
    mouseClick(waitForObjectItem(":Options_QListView", "Environment"))
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Keyboard")
    filter = waitForObject("{container={name='Command Mappings' type='QGroupBox' visible='1'} "
                           "type='Utils::FancyLineEdit' unnamed='1' visible='1' "
                           "placeholderText='Filter'}")
    replaceEditorContent(filter, "about")
    treewidget = waitForObject("{type='QTreeWidget' unnamed='1' visible='1'}")
    modelIndex = waitForObject("{column='0' text='AboutQtCreator' type='QModelIndex' "
                               "container={column='0' text='QtCreator' type='QModelIndex' "
                               "container=%s}}" % objectMap.realName(treewidget))
    treewidget.scrollTo(modelIndex)
    mouseClick(modelIndex)
    shortcutGB = "{title='Shortcut' type='QGroupBox' unnamed='1' visible='1'}"
    record = waitForObject("{container=%s type='Core::Internal::ShortcutButton' unnamed='1' "
                           "visible='1' text~='(Stop Recording|Record)'}" % shortcutGB)
    shortcut = ("{container=%s type='Utils::FancyLineEdit' unnamed='1' visible='1' "
                "placeholderText='Enter key sequence as text'}" % shortcutGB)
    clickButton(record)
    nativeType(keysToType)
    waitFor("_shortcutMatches_(shortcut, expectedKeys)", 5000)
    clickButton(record)

    gotExpectedShortcut = _shortcutMatches_(shortcut, expectedKeys)
    if not gotExpectedShortcut and platform.system() == 'Darwin':
        test.warning("Squish Issue: shortcut was set to %s - entering it manually now"
                     % waitForObject(shortcut).text)
        replaceEditorContent(shortcut, expectedKeys)
    else:
        test.verify(gotExpectedShortcut, "Expected key sequence is displayed.")
    clickButton(waitForObject(":Options.OK_QPushButton"))

def main():
    expectedVersion = getQtCreatorVersionFromFile()
    if not expectedVersion:
        test.fatal("Can't find version from file.")
        return
    startQC()
    if not startedWithoutPluginError():
        return
    setKeyboardShortcutForAboutQtC()
    if platform.system() == 'Darwin':
        try:
            waitForObject(":Qt Creator.QtCreator.MenuBar_QMenuBar", 2000)
        except:
            nativeMouseClick(waitForObject(":Qt Creator_Core::Internal::MainWindow", 1000), 20, 20, 0, Qt.LeftButton)
    nativeType(keysToType)
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
    test.compare(actualVersion, expectedVersion,
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
