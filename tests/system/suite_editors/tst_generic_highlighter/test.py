# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

def __highlighterDefinitionsDirectory__():
    if platform.system() in ('Microsoft', 'Windows'):
        basePath = os.path.expandvars("%LOCALAPPDATA%")
    elif platform.system() == 'Linux':
        basePath = os.path.expanduser("~/.local/share")
    else:  # macOS
        basePath = os.path.expanduser("~/Library/Application Support")
    return os.path.join(basePath, "org.kde.syntax-highlighting", "syntax")

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
    mouseClick(waitForObjectItem(table, row))
    return waitForObject("{name='patternsLineEdit' type='QLineEdit' visible='1'}")

def getOrModifyFilePatternsFor(mimeType, filter='', toBePresent=None):
    toSuffixArray = lambda x : [pat.replace("*", "") for pat in x.split(";")]

    result = []
    invokeMenuItem("Edit", "Preferences...")
    mouseClick(waitForObjectItem(":Options_QListView", "Environment"))
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "MIME Types")
    replaceEditorContent(waitForObject("{name='filterLineEdit' type='Utils::FancyLineEdit' "
                                       "visible='1'}"), filter)
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
        test.log("Checking %s" % dumpItems(model)[0])
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
    elif model.rowCount() > 0:
        test.warning("MIME type '%s' has unexpected results." % mimeType)
    else:
        test.log("MIME type '%s' seems to be unknown to the system." % mimeType)
    clickButton(":Options.Cancel_QPushButton")
    return result

def addHighlighterDefinition(*languages):
    syntaxDirectory = __highlighterDefinitionsDirectory__()
    toBeChecked = (os.path.join(syntaxDirectory, x + ".xml") for x in languages)
    test.log("Updating highlighter definitions...")
    invokeMenuItem("Edit", "Preferences...")
    mouseClick(waitForObjectItem(":Options_QListView", "Text Editor"))
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Generic Highlighter")

    test.log("Trying to download definitions...")
    clickButton("{text='Download Definitions' type='QPushButton' unnamed='1' visible='1'}")
    updateStatus = "{name='updateStatus' type='QLabel' visible='1'}"
    waitFor("object.exists(updateStatus)", 5000)
    if waitFor('str(findObject(updateStatus).text) == "Download finished"', 20000):
        test.log("Received definitions")
        test.verify(os.path.exists(syntaxDirectory),
                    "Directory for syntax highlighter files exists.")
        xmlFiles = glob.glob(os.path.join(syntaxDirectory, "*.xml"))
        test.verify(len(xmlFiles) > 0, "Verified presence of syntax highlighter files. "
                    "(Found %d)" % len(xmlFiles))
        # should we check output (General Messages) as well?
        test.passes("Updated definitions")
        clickButton(":Options.OK_QPushButton")
        return map(os.path.exists, toBeChecked)
    else:
        test.fail("Could not update highlighter definitions")
        clickButton(":Options.Cancel_QPushButton")
        return map(os.path.exists, toBeChecked)

def hasSuffix(fileName, suffixPatterns):
    for suffix in suffixPatterns:
        if fileName.endswith(suffix):
            return True
    return False

def displayHintForHighlighterDefinition(fileName, patterns, added):
    if hasSuffix(fileName, patterns):
        return not added
    test.warning("Got an unexpected suffix.", "Filename: %s, Patterns: %s"
                 % (fileName, str(patterns)))
    return False

def main():
    miss = ("A highlight definition was not found for this file. Would you like to download "
            "additional highlight definition files?")
    startQC()
    if not startedWithoutPluginError():
        return

    patterns = getOrModifyFilePatternsFor("text/x-haskell", "x-haskell")

    folder = tempDir()
    filesToTest = ["Main.lhs", "Main.hs"]
    code = ['module Main where', '', 'main :: IO ()', '', 'main = putStrLn "Hello World!"']

    for current in filesToTest:
        createFile(folder, current)
        editor = getEditorForFileSuffix(current)
        if editor == None:
            earlyExit("Something's really wrong! (did the UI change?)")
            return
        expectHint = hasSuffix(current, patterns)
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
    addedHaskell = addHighlighterDefinition("haskell")
    patterns = getOrModifyFilePatternsFor('text/x-haskell', 'x-haskell', ['.hs', '.lhs'])

    home = os.path.expanduser("~")
    for current in filesToTest:
        recentFile = os.path.join(folder, current)
        if recentFile.startswith(home) and platform.system() in ('Linux', 'Darwin'):
            recentFile = recentFile.replace(home, "~", 1)
        invokeMenuItem("File", "Recent Files", "%d | " + recentFile)
        editor = getEditorForFileSuffix(current)
        display = displayHintForHighlighterDefinition(current, patterns, addedHaskell)
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
    saveAndExit()


def init():
    syntaxDirectory = __highlighterDefinitionsDirectory__()
    if not os.path.exists(syntaxDirectory):
        return
    test.log("Removing existing highlighter definitions folder")
    deleteDirIfExists(syntaxDirectory)
