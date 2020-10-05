############################################################################
#
# Copyright (C) 2020 The Qt Company Ltd.
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

# This tests for QTCREATORBUG-24565

TabBlankTab = '\t \t'
TripleTab = '\t\t\t'


def main():
    files = map(lambda record: os.path.join(srcPath, testData.field(record, "filename")),
                testData.dataset("files.tsv"))
    files = filter(lambda x: not x.endswith(".bin"), files)
    for currentFile in files:
        if not neededFilePresent(currentFile):
            return

    startQC()
    if not startedWithoutPluginError():
        return

    ignoredFiles = ignoredFilesFromSettings()
    directory = tempDir()

    for currentFile in files:
        baseName = os.path.basename(currentFile)
        fileName = os.path.join(directory, baseName)
        test.log("Testing file %s" % fileName)
        content = readFile(currentFile)
        preparedContent, emptyLine, trailingWS = prepareFileExternal(fileName, content)
        isIgnored = isIgnoredFile(baseName, ignoredFiles)
        test.verify(preparedContent.find('\t') != -1, "Added at least one tab.")
        test.log("Expecting file to be cleaned (trailing whitespace): %s" % str(not isIgnored))

        invokeMenuItem("File", "Open File or Project...")
        selectFromFileDialog(fileName, True)
        editor = getEditorForFileSuffix(fileName)
        if editor == None:
            test.fatal("Could not get the editor for '%s'" % fileName,
                       "Skipping this file for now.")
            continue

        fileCombo = waitForObject(":Qt Creator_FilenameQComboBox")
        invokeMenuItem("Edit", "Advanced", "Clean Whitespace")
        if emptyLine or (trailingWS and not isIgnored):
            waitFor("str(fileCombo.currentText).endswith('*')", 2500)

        currentContent = str(editor.toPlainText())
        if trailingWS and not isIgnored:
            test.verify(currentContent.find(TabBlankTab) == -1, "Trailing whitespace cleaned.")
            expectedContent = preparedContent.replace(TabBlankTab, '')
            if emptyLine:
                expectedContent = expectedContent.replace(TripleTab, '')
            test.compare(currentContent, expectedContent,
                         "'%s' vs '%s'" % (currentContent, expectedContent))
        else:
            test.verify(currentContent.find(TabBlankTab) != -1, "Trailing whitespace still present.")
        if emptyLine:
            test.compare(currentContent.find(TripleTab), -1, "Indentation cleaned.")

        hasBeenCleaned = str(fileCombo.currentText).endswith('*')
        test.compare(hasBeenCleaned, emptyLine or (trailingWS and not isIgnored),
                     "Cleaned as expected.")
        if hasBeenCleaned:
            invokeMenuItem('File', 'Save "%s"' % os.path.basename(fileName))

    invokeMenuItem("File", "Exit")


def prepareFileExternal(fileName, content):
    if content.find('\t') != -1:
        test.fatal("Tabs present before edit (%s)." % fileName)
    modifiedContent = ''
    emptyLine = trailingWS = False
    lines = content.splitlines(True)

    for currentLine in lines:
        if not emptyLine:
            if len(currentLine) == 1:  # just the line break
                currentLine = TripleTab + '\n'
                emptyLine = True
                test.log("Replaced empty line by 3 tabs.")
        elif not trailingWS:
            if re.match(r"^.*\S+.*$", currentLine) is not None:
                currentLine = currentLine[:-1] + TabBlankTab + '\n'
                trailingWS = True
                test.log("Added trailing whitespace.")
        modifiedContent += currentLine

    with open(fileName, "w") as f:
        f.write(modifiedContent)

    if not emptyLine or not trailingWS:
        test.warning("Could not completely prepare test file %s." % fileName)
    return modifiedContent, emptyLine, trailingWS


def isIgnoredFile(fileName, ignoredFiles):
    for filePattern in ignoredFiles:
        pattern = filePattern.replace('.', r'\.')
        pattern = pattern.replace('*', '.*')
        pattern = pattern.replace('?', '.')
        pattern = '^' + pattern + '$'
        if re.match(pattern, fileName) is not None:
            return True
    return False


def ignoredFilesFromSettings():
    invokeMenuItem("Tools", "Options...")
    mouseClick(waitForObjectItem(":Options_QListView", "Text Editor"))
    waitForObject("{container=':Options.qt_tabwidget_tabbar_QTabBar' type='TabItem' "
                  "text='Behavior'}")
    clickOnTab(":Options.qt_tabwidget_tabbar_QTabBar", "Behavior")
    ensureChecked("{type='QCheckBox' name='skipTrailingWhitespace'}")
    ignoredLineEdit = waitForObject("{type='QLineEdit' name='ignoreFileTypes'}")
    ignoredFiles = str(ignoredLineEdit.text).split(',')
    test.log("Ignored files: %s" % str(ignoredFiles))
    clickButton(":Options.Cancel_QPushButton")
    return ignoredFiles
