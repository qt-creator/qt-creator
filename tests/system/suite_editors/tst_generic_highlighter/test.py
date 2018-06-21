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

def createFile(folder, filename):
    __createProjectOrFileSelectType__("  General", "Empty File", isProject = False)
    replaceEditorContent(waitForObject("{name='nameLineEdit' visible='1' "
                                       "type='Utils::FileNameValidatingLineEdit'}"), filename)
    replaceEditorContent(waitForObject("{type='Utils::FancyLineEdit' unnamed='1' visible='1' "
                                       "window={type='ProjectExplorer::JsonWizard' unnamed='1' "
                                       "visible='1'}}"), folder)
    clickButton(waitForObject(":Next_QPushButton"))
    __createProjectHandleLastPage__()

def clickTableGetPatternLineEdit(table, row):
    clickItem(table, row, 5, 5, 0, Qt.LeftButton)
    return waitForObject("{name='patternsLineEdit' type='QLineEdit' visible='1'}")

def getOrModifyFilePatternsFor(mimeType, filter='', toBePresent=None):
    toSuffixArray = lambda x : [pat.replace("*", "") for pat in x.split(";")]

    result = []
    invokeMenuItem("Tools", "Options...")
    waitForObjectItem(":Options_QListView", "Environment")
    clickItem(":Options_QListView", "Environment", 14, 15, 0, Qt.LeftButton)
    waitForObject("{container=':Options.qt_tabwidget_tabbar_QTabBar' type='TabItem' "
                  "text='MIME Types'}")
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "MIME Types")
    replaceEditorContent(waitForObject("{name='filterLineEdit' type='QLineEdit' visible='1'}"),
                         filter)
    mimeTypeTable = waitForObject("{name='mimeTypesTreeView' type='QTreeView' visible='1'}")
    model = mimeTypeTable.model()
    if filter == '':
        for row in dumpItems(model):
            if row == mimeType:
                result = toSuffixArray(str(clickTableGetPatternLineEdit(mimeTypeTable, row).text))
                break
        clickButton(":Options.Cancel_QPushButton")
        if result == ['']:
            test.warning("MIME type '%s' seems to have no file patterns." % mimeType)
        return result
    waitFor('model.rowCount() == 1', 2000)
    if model.rowCount() == 1:
        patternsLineEd = clickTableGetPatternLineEdit(mimeTypeTable, dumpItems(model)[0])
        patterns = str(patternsLineEd.text)
        if toBePresent:
            actualSuffixes = toSuffixArray(patterns)
            toBeAddedSet = set(toBePresent).difference(set(actualSuffixes))
            if toBeAddedSet:
                patterns += ";*" + ";*".join(toBeAddedSet)
                replaceEditorContent(patternsLineEd, patterns)
                clickButton(":Options.OK_QPushButton")
                try:
                    mBox = waitForObject("{type='QMessageBox' unnamed='1' visible='1' "
                                         "text?='Conflicting pattern*'}", 2000)
                    conflictingSet = set(str(mBox.detailedText).replace("*", "").splitlines())
                    clickButton(waitForObject("{text='OK' type='QPushButton' unnamed='1' visible='1' "
                                              "window={type='QMessageBox' unnamed='1' visible='1'}}"))
                    if toBeAddedSet.intersection(conflictingSet):
                        test.fatal("At least one of the patterns to be added is already in use "
                                   "for another MIME type.",
                                   "Conflicting patterns: %s" % str(conflictingSet))
                    if conflictingSet.difference(toBeAddedSet):
                        test.fail("MIME type handling failed. (QTCREATORBUG-12149?)",
                                  "Conflicting patterns: %s" % str(conflictingSet))
                        # re-check the patterns
                        result = getOrModifyFilePatternsFor(mimeType)
                except:
                    result = toSuffixArray(patterns)
                    test.passes("Added suffixes")
                return result
            else:
                result = toSuffixArray(patterns)
        else:
            result = toSuffixArray(patterns)
    elif model.rowCount() > 1:
        test.warning("MIME type '%s' has ambiguous results." % mimeType)
    else:
        test.log("MIME type '%s' seems to be unknown to the system." % mimeType)
    clickButton(":Options.Cancel_QPushButton")
    return result

def uncheckGenericHighlighterFallback():
    invokeMenuItem("Tools", "Options...")
    waitForObjectItem(":Options_QListView", "Text Editor")
    clickItem(":Options_QListView", "Text Editor", 14, 15, 0, Qt.LeftButton)
    waitForObject("{container=':Options.qt_tabwidget_tabbar_QTabBar' type='TabItem' "
                  "text='Generic Highlighter'}")
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Generic Highlighter")
    ensureChecked("{name='useFallbackLocation' text='Use fallback location' type='QCheckBox' "
                  "visible='1'}", False)
    clickButton(":Options.OK_QPushButton")

def addHighlighterDefinition(language):
    global tmpSettingsDir
    test.log("Adding highlighter definitions for '%s'." % language)
    invokeMenuItem("Tools", "Options...")
    waitForObjectItem(":Options_QListView", "Text Editor")
    clickItem(":Options_QListView", "Text Editor", 14, 15, 0, Qt.LeftButton)
    waitForObject("{container=':Options.qt_tabwidget_tabbar_QTabBar' type='TabItem' "
                  "text='Generic Highlighter'}")
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Generic Highlighter")
    clickButton("{text='Download Definitions...' type='QPushButton' unnamed='1' visible='1'}")
    table = waitForObject("{name='definitionsTable' type='QTableWidget' visible='1'}")
    model = table.model()
    for row in range(model.rowCount()):
        if str(model.data(model.index(row, 0)).toString()) == language:
            clickItem(table, "%d/0" % row, 5, 5, 0, Qt.LeftButton)
            clickButton("{name='downloadButton' text='Download Selected Definitions' "
                        "type='QPushButton' visible='1'}")
            # downloading happens asynchronously but may take a little time
            progressBarWait(10000)
            languageFile = os.path.join(tmpSettingsDir, "QtProject", "qtcreator",
                                        "generic-highlighter", "%s.xml"
                                        % language.lower().replace(" ", "-"))
            test.verify(waitFor("os.path.exists(languageFile)", 10000),
                        "Verifying whether highlight definition file for '%s' has been downloaded "
                        "and placed to settings." % language)
            clickButton("{text='Download Definitions...' type='QPushButton' unnamed='1' "
                        "visible='1'}")
            table = waitForObject("{name='definitionsTable' type='QTableWidget' visible='1'}")
            model = table.model()
            test.verify(str(model.data(model.index(row, 1))) != "",
                        "Verifying a definition has been downloaded.")
            clickButton("{text='Close' type='QPushButton' unnamed='1' visible='1'}")
            clickButton(":Options.OK_QPushButton")
            return True
    test.fail("Could not find the specified language (%s) to download a highlighter definition"
              % language)
    clickButton("{text='Close' type='QPushButton' unnamed='1' visible='1'}")
    clickButton(":Options.OK_QPushButton")
    return False

def hasSuffix(fileName, suffixPatterns):
    for suffix in suffixPatterns:
        if fileName.endswith(suffix):
            return True
    return False

def displayHintForHighlighterDefinition(fileName, patterns, lPatterns, added, addedLiterate):
    if hasSuffix(fileName, patterns):
        return not added
    if hasSuffix(fileName, lPatterns):
        return not addedLiterate
    test.warning("Got an unexpected suffix.", "Filename: %s, Patterns: %s"
                 % (fileName, str(patterns + lPatterns)))
    return False

def main():
    miss = "A highlight definition was not found for this file. Would you like to try to find one?"
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    uncheckGenericHighlighterFallback()
    patterns = getOrModifyFilePatternsFor("text/x-haskell", "x-haskell")
    lPatterns = getOrModifyFilePatternsFor("text/x-literate-haskell", "literate-haskell")

    folder = tempDir()
    filesToTest = ["Main.lhs", "Main.hs"]
    code = ['module Main where', '', 'main :: IO ()', '', 'main = putStrLn "Hello World!"']

    for current in filesToTest:
        createFile(folder, current)
        editor = getEditorForFileSuffix(current)
        if editor == None:
            earlyExit("Something's really wrong! (did the UI change?)")
            return
        expectHint = hasSuffix(current, patterns) or hasSuffix(current, lPatterns)
        mssg = "Verifying whether hint for missing highlight definition is present. (expected: %s)"
        try:
            waitForObject("{text='%s' type='QLabel' unnamed='1' visible='1' "
                          "window=':Qt Creator_Core::Internal::MainWindow'}" % miss, 2000)
            test.verify(expectHint, mssg % str(expectHint))
        except:
            test.verify(not expectHint, mssg % str(expectHint))
        # literate haskell: first character must be '>' otherwise it's a comment
        if current.endswith(".lhs"):
            typeLines(editor, [">" + line for line in code])
        else:
            typeLines(editor, code)

    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Close All")
    addedHighlighterDefinition = addHighlighterDefinition("Haskell")
    addedLiterateHighlighterDefinition = addHighlighterDefinition("Literate Haskell")
    patterns = getOrModifyFilePatternsFor('text/x-haskell', 'x-haskell', ['.hs'])
    lPatterns = getOrModifyFilePatternsFor('text/x-literate-haskell', 'literate-haskell', ['.lhs'])

    home = os.path.expanduser("~")
    for current in filesToTest:
        recentFile = os.path.join(folder, current)
        if recentFile.startswith(home) and platform.system() in ('Linux', 'Darwin'):
            recentFile = recentFile.replace(home, "~", 1)
        invokeMenuItem("File", "Recent Files", "(&\\d \| )?%s" % recentFile)
        editor = getEditorForFileSuffix(current)
        display = displayHintForHighlighterDefinition(current, patterns, lPatterns,
                                                      addedHighlighterDefinition,
                                                      addedLiterateHighlighterDefinition)
        try:
            waitForObject("{text='%s' type='QLabel' unnamed='1' visible='1' "
                          "window=':Qt Creator_Core::Internal::MainWindow'}" % miss, 2000)
            test.verify(display, "Hint for missing highlight definition was present "
                        "- current file: %s" % current)
        except:
            test.verify(not display, "Hint for missing highlight definition is not shown "
                        "- current file: %s" % current)
        placeCursorToLine(editor, '.*%s' % code[-1], True)
        for _ in range(23):
            type(editor, "<Left>")
        type(editor, "<Return>")
        if current.endswith(".lhs"):
            type(editor, ">")
        type(editor, "<Tab>")

    invokeMenuItem("File", "Save All")
    invokeMenuItem("File", "Exit")
