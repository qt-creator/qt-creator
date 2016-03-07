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

def main():
    pathReadme = srcPath + "/creator/README.md"
    if not neededFilePresent(pathReadme):
        return

    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return

    invokeMenuItem("File", "Open File or Project...")
    selectFromFileDialog(pathReadme)
    invokeMenuItem("Tools", "Git", "Actions on Commits...")
    pathEdit = waitForObject(":Select a Git Commit.workingDirectoryEdit_QLineEdit")
    revEdit = waitForObject(":Select a Git Commit.changeNumberEdit_Utils::CompletingLineEdit")
    test.compare(str(pathEdit.displayText), os.path.join(srcPath, "creator"))
    test.compare(str(revEdit.displayText), "HEAD")
    replaceEditorContent(revEdit, "05c35356abc31549c5db6eba31fb608c0365c2a0") # Initial import
    detailsEdit = waitForObject(":Select a Git Commit.detailsText_QPlainTextEdit")
    test.verify(detailsEdit.readOnly, "Details view is read only?")
    waitFor("str(detailsEdit.plainText) != 'Fetching commit data...'")
    commitDetails = str(detailsEdit.plainText)
    test.verify("commit 05c35356abc31549c5db6eba31fb608c0365c2a0\n" \
                "Author: con <qtc-commiter@nokia.com>" in commitDetails,
                "Information header in details view?")
    test.verify("Initial import" in commitDetails, "Commit message in details view?")
    test.verify("src/plugins/debugger/gdbengine.cpp                 | 4035 ++++++++++++++++++++"
                in commitDetails, "Text file in details view?")
    test.verify("src/plugins/find/images/expand.png                 |  Bin 0 -> 931 bytes"
                in commitDetails, "Binary file in details view?")
    test.verify(" files changed, 229938 insertions(+)" in commitDetails,
                "Summary in details view?")
    clickButton(waitForObject(":Select a Git Commit.Show_QPushButton"))
    changedEdit = waitForObject(":Qt Creator_DiffEditor::SideDiffEditorWidget")
    waitFor("len(str(changedEdit.plainText)) > 0 and "
            "str(changedEdit.plainText) != 'Waiting for data...'", 40000)
    diffPlainText = str(changedEdit.plainText)
    test.verify("# This file is used to ignore files which are generated" in diffPlainText,
                "Comment from .gitignore in diff?")
    test.verify("SharedTools::QtSingleApplication app((QLatin1String(appNameC)), argc, argv);"
                in diffPlainText, "main function in diff?")
    invokeMenuItem("File", "Exit")
