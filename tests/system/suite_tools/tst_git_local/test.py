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

# necessary to not use symbolic links for the parent path of the git project
srcPath = os.path.realpath(srcPath)
projectName = "gitProject"

# TODO: Make selecting changes possible
def commit(commitMessage, expectedLogMessage, uncheckUntracked=False):
    openVcsLog()
    clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))
    invokeMenuItem("Tools", "Git", "Local Repository", "Commit...")
    replaceEditorContent(waitForObject(":Description.description_Utils::CompletingTextEdit"), commitMessage)
    ensureChecked(waitForObject(":Files.Check all_QCheckBox"))
    if uncheckUntracked:
        treeView = waitForObject("{container=':splitter.Files_QGroupBox' name='fileView' type='QTreeView' visible='1'}")
        model = treeView.model()
        for indexStr in dumpItems(model):
            if 'untracked' in indexStr:
                clickItem(treeView, indexStr, 5, 5, 0, Qt.LeftButton)
    checkOrFixCommitterInformation('invalidAuthorLabel', 'authorLineEdit', 'Nobody')
    checkOrFixCommitterInformation('invalidEmailLabel', 'emailLineEdit', 'nobody@nowhere.com')
    clickButton(waitForObject(":splitter.Commit File(s)_VcsBase::QActionPushButton"))
    vcsLog = waitForObject("{type='QPlainTextEdit' unnamed='1' visible='1' "
                           "window=':Qt Creator_Core::Internal::MainWindow'}").plainText
    test.verify(expectedLogMessage in str(vcsLog), "Searching for '%s' in log:\n%s " % (expectedLogMessage, vcsLog))
    return commitMessage

def verifyItemsInGit(commitMessages):
    gitEditor = waitForObject(":Qt Creator_Git::Internal::GitEditor")
    waitFor("len(str(gitEditor.plainText)) > 0 and str(gitEditor.plainText) != 'Working...'", 20000)
    plainText = str(gitEditor.plainText)
    verifyItemOrder(commitMessages, plainText)
    return plainText

def createLocalGitConfig(path):
    config = os.path.join(path, "config")
    test.verify(os.path.exists(config), "Verifying if .git/config exists.")
    file = open(config, "a")
    file.write("\n[user]\n\temail = nobody@nowhere.com\n\tname = Nobody\n")
    file.close()

def checkOrFixCommitterInformation(labelName, lineEditName, expected):
    lineEd = waitForObject("{name='%s' type='QLineEdit' visible='1'}" % lineEditName)
    if not (test.compare(lineEd.text, expected, "Verifying commit information matches local config")
        and test.verify(checkIfObjectExists("{name='%s' type='QLabel' visible='1'}" % labelName,
                                            False, 1000),
                                            "Verifying invalid label is missing")):
        test.log("Commit information invalid or missing - entering dummy value (%s)" % expected)
        replaceEditorContent(lineEd, expected)

# Opens a commit's diff from a diff log
# param count is the number of the commit (1-based) in chronologic order
def __clickCommit__(count):
    gitEditor = waitForObject(":Qt Creator_Git::Internal::GitEditor")
    fileName = waitForObject(":Qt Creator_FilenameQComboBox")
    test.verify(waitFor('str(fileName.currentText).startswith("Git Log")', 1000),
                "Verifying Qt Creator still displays git log inside editor.")
    waitFor("'Initial Commit' in str(gitEditor.plainText)", 3000)
    content = str(gitEditor.plainText)
    commit = None
    # find commit
    try:
        # Commits are listed in reverse chronologic order, so we have to invert count
        line = filter(lambda line: line.startswith("commit"), content.splitlines())[-count]
        commit = line.split(" ", 1)[1]
    except:
        test.fail("Could not find the %d. commit - leaving test" % count)
        return False
    placeCursorToLine(gitEditor, line)
    for i in range(30):
        type(gitEditor, "<Left>")
    # get the current cursor rectangle which should be positioned on the commit ID
    rect = gitEditor.cursorRect()
    # click on the commit ID
    mouseClick(gitEditor, rect.x, rect.y + rect.height / 2, 0, Qt.LeftButton)
    expected = 'Git Show "%s"' % commit
    test.verify(waitFor('str(fileName.currentText) == expected', 5000),
                "Verifying editor switches to Git Show.")
    description = waitForObject(":Qt Creator_DiffEditor::Internal::DescriptionEditorWidget")
    waitFor('len(str(description.plainText)) != 0', 5000)
    show = str(description.plainText)
    id = "Nobody <nobody@nowhere\.com>"
    time = "\w{3} \w{3} \d{1,2} \d{2}:\d{2}:\d{2} \d{4}.* seconds ago\)"
    expected = [{"commit %s" % commit:False},
                {"Author: %s, %s" % (id, time): True},
                {"Committer: %s, %s" % (id, time): True}]
    for line, exp in zip(show.splitlines(), expected):
        expLine = exp.keys()[0]
        isRegex = exp.values()[0]
        if isRegex:
            test.verify(re.match(expLine, line), "Verifying commit header line '%s'" % line)
        else:
            test.compare(line, expLine, "Verifying commit header line.")
    test.verify(description.readOnly,
                "Verifying description editor widget is readonly.")
    return True

def verifyClickCommit():
    for i in range(1, 3):
        if not __clickCommit__(i):
            continue
        changed = waitForObject(":Qt Creator_DiffEditor::SideDiffEditorWidget")
        original = waitForObject(":Qt Creator_DiffEditor::SideDiffEditorWidget2")
        waitFor('str(changed.plainText) != "Waiting for data..." '
                'and str(original.plainText) != "Waiting for data..."', 5000)
        # content of diff editors is merge of modified files
        diffOriginal = str(original.plainText)
        diffChanged = str(changed.plainText)
        if i == 1:
            # diffChanged must completely contain main.cpp
            mainCPP = readFile(os.path.join(srcPath, projectName, "main.cpp"))
            test.verify(mainCPP in diffChanged,
                        "Verifying whether diff editor contains main.cpp file.")
            test.verify(mainCPP not in diffOriginal,
                        "Verifying whether original does not contain main.cpp file.")
        elif i == 2:
            # diffChanged must completely contain the pointless_header.h
            pointlessHeader = readFile(os.path.join(srcPath, projectName, "pointless_header.h"))
            test.verify(pointlessHeader in diffChanged,
                        "Verifying whether diff editor contains pointless_header.h file.")
            test.verify(pointlessHeader not in diffOriginal,
                        "Verifying whether original does not contain pointless_header.h file.")
            test.verify("HEADERS  += mainwindow.h \\\n    pointless_header.h\n" in diffChanged,
                        "Verifying whether diff editor has pointless_header.h listed in pro file.")
            test.verify("HEADERS  += mainwindow.h\n\n" in diffOriginal
                        and "pointless_header.h" not in diffOriginal,
                        "Verifying whether original has no additional header in pro file.")
        test.verify(original.readOnly and changed.readOnly,
                    "Verifying that the actual diff editor widgets are readonly.")
        invokeMenuItem("Tools", "Git", "Local Repository", "Log")

def addEmptyFileOutsideProject(filename):
    __createProjectOrFileSelectType__("  General", "Empty File", isProject=False)
    replaceEditorContent(waitForObject(":New Text File.nameLineEdit_Utils::FileNameValidatingLineEdit"), filename)
    clickButton(waitForObject(":Next_QPushButton"))
    __createProjectHandleLastPage__([filename], "Git", "<None>")

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    createProject_Qt_GUI(srcPath, projectName, addToVersionControl = "Git")
    openVcsLog()
    vcsLog = waitForObject("{type='Core::OutputWindow' unnamed='1' visible='1' "
                           "window=':Qt Creator_Core::Internal::MainWindow'}").plainText
    test.verify("Initialized empty Git repository in %s"
                % os.path.join(srcPath, projectName, ".git").replace("\\", "/") in str(vcsLog),
                "Has initialization of repo been logged:\n%s " % vcsLog)
    createLocalGitConfig(os.path.join(srcPath, projectName, ".git"))
    commitMessages = [commit("Initial Commit", "Committed 5 file(s).")]
    clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))
    headerName = "pointless_header.h"
    addCPlusPlusFileToCurrentProject(headerName, "C++ Header File", addToVCS="Git",
                                     expectedHeaderName=headerName)
    commitMessages.insert(0, commit("Added pointless header file", "Committed 2 file(s)."))
    readmeName = "README.txt"
    addEmptyFileOutsideProject(readmeName)
    replaceEditorContent(waitForObject(":Qt Creator_TextEditor::TextEditorWidget"),
                         "Some important advice in the README")
    invokeMenuItem("File", "Save All")
    commitsInProject = list(commitMessages) # deep copy
    commitOutsideProject = commit("Added README file", "Committed 2 file(s).", True) # QTCREATORBUG-11074
    commitMessages.insert(0, commitOutsideProject)

    invokeMenuItem('Tools', 'Git', 'Current File', 'Log of "%s"' % readmeName)
    fileLog = verifyItemsInGit([commitOutsideProject])
    for commitMessage in commitsInProject:
        test.verify(not commitMessage in fileLog,
                    "Verify that no unrelated commits are displayed in file log")
    invokeMenuItem("File", "Close All")

    invokeMenuItem('Tools', 'Git', 'Current Project', 'Log Project "%s"' % projectName)
    projectLog = verifyItemsInGit(commitsInProject)
    test.xverify(not commitOutsideProject in projectLog,    # QTCREATORBUG-10170
                 "Verify that no unrelated commits are displayed in project log")
    invokeMenuItem("File", "Close All")

    invokeMenuItem("Tools", "Git", "Local Repository", "Log")
    verifyItemsInGit(commitMessages)
    # verifyClickCommit() must be called after the local git has been created and the files
    # have been pushed to the repository
    verifyClickCommit()
    # test for QTCREATORBUG-15051
    addEmptyFileOutsideProject("anotherFile.txt")
    fakeIdCommitMessage = "deadbeefdeadbeefdeadbeef is not a commit id"
    commit(fakeIdCommitMessage, "Committed 1 file(s).")
    invokeMenuItem("Tools", "Git", "Local Repository", "Log")
    gitEditor = waitForObject(":Qt Creator_Git::Internal::GitEditor")
    waitFor("len(str(gitEditor.plainText)) > 0 and str(gitEditor.plainText) != 'Working...'", 20000)
    placeCursorToLine(gitEditor, fakeIdCommitMessage)
    if platform.system() == 'Darwin':
        type(gitEditor, "<Ctrl+Left>")
    else:
        type(gitEditor, "<Home>")
    for _ in range(5):
        type(gitEditor, "<Right>")
    rect = gitEditor.cursorRect(gitEditor.textCursor())
    mouseClick(gitEditor, rect.x+rect.width/2, rect.y+rect.height/2, 0, Qt.LeftButton)
    changed = waitForObject(":Qt Creator_DiffEditor::SideDiffEditorWidget")
    waitFor('str(changed.plainText) != "Waiting for data..."', 5000)
    test.compare(str(changed.plainText), "Failed",
                 "Showing an invalid commit can't succeed but Creator survived.")

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
