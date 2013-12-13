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

projectName = "gitProject"

# TODO: Make selecting changes possible
def commit(commitMessage, expectedLogMessage):
    ensureChecked(waitForObject(":Qt Creator_VersionControl_Core::Internal::OutputPaneToggleButton"))
    clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))
    invokeMenuItem("Tools", "Git", "Local Repository", "Commit...")
    replaceEditorContent(waitForObject(":Description.description_Utils::CompletingTextEdit"), commitMessage)
    ensureChecked(waitForObject(":Files.Check all_QCheckBox"))
    clickButton(waitForObject(":splitter.Commit File(s)_VcsBase::QActionPushButton"))
    vcsLog = waitForObject("{type='QPlainTextEdit' unnamed='1' visible='1' "
                           "window=':Qt Creator_Core::Internal::MainWindow'}").plainText
    test.verify(expectedLogMessage in str(vcsLog), "Searching for '%s' in log:\n%s " % (expectedLogMessage, vcsLog))
    return commitMessage

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    createProject_Qt_GUI(srcPath, projectName, addToVersionControl = "Git")
    if isQt4Build and not object.exists(":Qt Creator_VersionControl_Core::Internal::OutputPaneToggleButton"):
        clickButton(waitForObject(":Qt Creator_Core::Internal::OutputPaneManageButton"))
        activateItem(waitForObjectItem("{type='QMenu' unnamed='1' visible='1'}", "Version Control"))
    ensureChecked(waitForObject(":Qt Creator_VersionControl_Core::Internal::OutputPaneToggleButton"))
    vcsLog = waitForObject("{type='QPlainTextEdit' unnamed='1' visible='1' "
                           "window=':Qt Creator_Core::Internal::MainWindow'}").plainText
    test.verify("Initialized empty Git repository in %s"
                % os.path.join(srcPath, projectName, ".git").replace("\\", "/") in str(vcsLog),
                "Has initialization of repo been logged:\n%s " % vcsLog)
    commitMessages = [commit("Initial Commit", "Committed 5 file(s).")]
    clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))
    addCPlusPlusFileToCurrentProject("pointless_header.h", "C++ Header File", addToVCS = "Git")
    commitMessages.insert(0, commit("Added pointless header file", "Committed 2 file(s)."))
    __createProjectOrFileSelectType__("  General", "Text File", isProject=False)
    replaceEditorContent(waitForObject(":New Text File.nameLineEdit_Utils::FileNameValidatingLineEdit"), "README")
    clickButton(waitForObject(":Next_QPushButton"))
    __createProjectHandleLastPage__(["README.txt"], "Git", "<None>")
    replaceEditorContent(waitForObject(":Qt Creator_TextEditor::PlainTextEditorWidget"),
                         "Some important advice in the README")
    invokeMenuItem("File", "Save All")
    commitMessages.insert(0, commit("Added README file", "Committed 2 file(s).")) # QTCREATORBUG-11074
    invokeMenuItem("File", "Close All")
    invokeMenuItem("Tools", "Git", "Local Repository", "Log")
    gitEditor = waitForObject(":Qt Creator_Git::Internal::GitEditor")
    waitFor("str(gitEditor.plainText) != 'Waiting for data...'", 20000)
    verifyItemOrder(commitMessages, gitEditor.plainText)
    invokeMenuItem("File", "Close All Projects and Editors")
    invokeMenuItem("File", "Exit")

def deleteProject():
    path = os.path.join(srcPath, projectName)
    if os.path.exists(path):
        try:
            # Make files in .git writable to remove them
            for root, dirs, files in os.walk(path):
                for name in files:
                    os.chmod(os.path.join(root, name), stat.S_IWUSR)
            shutil.rmtree(path)
        except:
            test.warning("Error while removing '%s'" % path)

def init():
    deleteProject()

def cleanup():
    deleteProject()
