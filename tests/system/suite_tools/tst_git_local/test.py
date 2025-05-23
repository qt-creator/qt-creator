# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
                mouseClick(waitForObjectItem(treeView, indexStr))
    checkOrFixCommitterInformation('invalidAuthorLabel', 'authorLineEdit', 'Nobody')
    checkOrFixCommitterInformation('invalidEmailLabel', 'emailLineEdit', 'nobody@nowhere.com')
    clickButton(waitForObject(":splitter.Commit File(s)_VcsBase::QActionPushButton"))
    vcsLog = waitForObject("{type='QPlainTextEdit' unnamed='1' visible='1' "
                           "window=':Qt Creator_Core::Internal::MainWindow'}")
    test.verify(waitFor('expectedLogMessage in str(vcsLog.plainText)', 2000),
                "Searching for '%s' in log:\n%s " % (expectedLogMessage, vcsLog.plainText))
    return commitMessage

def verifyItemsInGit(commitMessages):
    gitEditor = waitForObject(":Qt Creator_Git::Internal::GitEditor")
    if not waitFor("len(str(gitEditor.plainText))>0 and str(gitEditor.plainText)!='Working...'",
                   40000):
        test.warning("Waiting for GitEditor timed out.")
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
        line = list(filter(lambda line: line.startswith("commit"),
                           content.splitlines()))[-count].strip()
        commit = line.split(" ", 1)[1]
    except:
        test.fail("Could not find the %d. commit - leaving test" % count)
        return False
    placeCursorToLine(gitEditor, line)
    for _ in range(30):
        type(gitEditor, "<Left>")
    # get the current cursor rectangle which should be positioned on the commit ID
    rect = gitEditor.cursorRect()
    # click on the commit ID
    mouseClick(gitEditor, rect.x, rect.y + rect.height / 2, 0, Qt.LeftButton)
    expected = 'Git Show "%s"' % commit
    test.verify(waitFor('str(fileName.currentText) == expected', 5000),
                "Verifying editor switches to Git Show.")
    description = waitForObject(":Qt Creator_DiffEditor::Internal::DescriptionEditorWidget")
    waitFor('len(str(description.plainText)) != 0 '
            'and str(description.plainText) != "Waiting for data..."', 5000)
    show = str(description.plainText)
    id = "Nobody <nobody@nowhere\.com>"
    time = "\w{3} \w{3} \d{1,2} \d{2}:\d{2}:\d{2} \d{4}.* seconds ago\)"
    expected = [["commit %s " % commit, False],
                ["Author: %s, %s" % (id, time), True],
                ["Committer: %s, %s" % (id, time), True]]
    for line, exp in zip(show.splitlines(), expected):
        expLine = exp[0]
        isRegex = exp[1]
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
        changed = waitForObject(":Qt Creator_DiffEditor::SideDiffEditorWidgetChanged")
        original = waitForObject(":Qt Creator_DiffEditor::SideDiffEditorWidgetOriginal")
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
            test.verify("HEADERS += \\\n    mainwindow.h \\\n    pointless_header.h\n" in diffChanged,
                        "Verifying whether diff editor has pointless_header.h listed in pro file.")
            test.verify("HEADERS += \\\n    mainwindow.h\n\n" in diffOriginal
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
    startQC()
    if not startedWithoutPluginError():
        return
    createProject_Qt_GUI(srcPath, projectName, addToVersionControl = "Git", buildSystem = "qmake")
    openVcsLog()
    vcsLog = waitForObject("{type='Core::OutputWindow' unnamed='1' visible='1' "
                           "window=':Qt Creator_Core::Internal::MainWindow'}").plainText
    test.verify("Initialized empty Git repository in %s"
                % os.path.join(srcPath, projectName, ".git").replace("\\", "/") in str(vcsLog),
                "Has initialization of repo been logged:\n%s " % vcsLog)
    createLocalGitConfig(os.path.join(srcPath, projectName, ".git"))
    commitMessages = [commit("Initial Commit", "Committed 6 files.")]
    clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))
    headerName = "pointless_header.h"
    addCPlusPlusFile(headerName, "C/C++ Header File", projectName + ".pro",
                     addToVCS="Git", expectedHeaderName=headerName)
    commitMessages.insert(0, commit("Added pointless header file", "Committed 2 files."))
    readmeName = "README.txt"
    addEmptyFileOutsideProject(readmeName)
    replaceEditorContent(waitForObject(":Qt Creator_TextEditor::TextEditorWidget"),
                         "Some important advice in the README")
    invokeMenuItem("File", "Save All")
    commitsInProject = list(commitMessages) # deep copy
    commitOutsideProject = commit("Added README file", "Committed 1 files.", True)
    commitMessages.insert(0, commitOutsideProject)

    invokeMenuItem('Tools', 'Git', 'Current File', 'Log of "%s"' % readmeName)
    fileLog = verifyItemsInGit([commitOutsideProject])
    for commitMessage in commitsInProject:
        test.verify(not commitMessage in fileLog,
                    "Verify that no unrelated commits are displayed in file log")
    invokeMenuItem("File", "Close All")

    invokeMenuItem('Tools', 'Git', 'Current Project Directory',
                   'Log Directory of Project "%s"' % projectName)
    verifyItemsInGit(commitMessages)
    invokeMenuItem("File", "Close All")

    invokeMenuItem("Tools", "Git", "Local Repository", "Log")
    verifyItemsInGit(commitMessages)
    # verifyClickCommit() must be called after the local git has been created and the files
    # have been pushed to the repository
    verifyClickCommit()
    # test for QTCREATORBUG-15051
    addEmptyFileOutsideProject("anotherFile.txt")
    fakeIdCommitMessage = "deadbeefdeadbeefdeadbeef is not a commit id"
    commit(fakeIdCommitMessage, "Committed 1 files.")
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
    rect = gitEditor.cursorRect()
    mouseClick(gitEditor, rect.x+rect.width/2, rect.y+rect.height/2, 0, Qt.LeftButton)
    changed = waitForObject(":Qt Creator_DiffEditor::SideDiffEditorWidgetChanged")
    waitFor('str(changed.plainText) != "Waiting for data..."', 5000)
    test.compare(str(changed.plainText), "Retrieving data failed.",
                 "Showing an invalid commit can't succeed but Creator survived.")
    invokeMenuItem("File", "Exit")
    waitForCleanShutdown()

def deleteProject():
    path = os.path.join(srcPath, projectName)
    if os.path.exists(path):
        try:
            # Make files in .git writable to remove them
            for root, _, files in os.walk(path):
                for name in files:
                    os.chmod(os.path.join(root, name), stat.S_IWUSR)
            shutil.rmtree(path)
        except:
            test.warning("Error while removing '%s'" % path)

def init():
    deleteProject()

def cleanup():
    deleteProject()
