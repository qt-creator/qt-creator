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

cloneUrl = "https://codereview.qt-project.org/p/qt-labs/jom"
cloneDir = "myCloneOfJom"

def verifyCloneLog(targetDir, canceled):
    if canceled:
        summary = "Failed."
    else:
        finish = findObject(":Git Repository Clone.Finish_QPushButton")
        waitFor("finish.enabled", 30000)
        cloneLog = str(waitForObject(":Git Repository Clone.logPlainTextEdit_QPlainTextEdit").plainText)
        if "fatal: " in cloneLog:
            test.warning("Cloning failed outside Creator.")
            return False
        # test for QTCREATORBUG-10112
        test.compare(cloneLog.count("remote: Counting objects:"), 1)
        test.compare(cloneLog.count("remote: Finding sources:"), 1)
        test.compare(cloneLog.count("Receiving objects:"), 1)
        test.compare(cloneLog.count("Resolving deltas:"), 1)
        test.verify(not "Stopping..." in cloneLog,
                    "Searching for 'Stopping...' in clone log")
        test.verify(("'" + cloneDir + "'..." in cloneLog),
                    "Searching for clone directory in clone log")
        summary = "Succeeded."
    try:
        resultLabel = findObject(":Git Repository Clone.Result._QLabel")
        test.verify(waitFor('str(resultLabel.text) == summary', 3000),
                    "Verifying expected result (%s)" % summary)
    except:
        if canceled:
            test.warning("Could not find resultLabel",
                         "Cloning might have failed before clicking 'Cancel'")
            return object.exists(":New Text File_ProjectExplorer::JsonWizard")
        else:
            test.fail("Could not find resultLabel")
    return True

def verifyVersionControlView(targetDir, canceled):
    openVcsLog()
    vcsLog = str(waitForObject("{type='QPlainTextEdit' unnamed='1' visible='1' "
                               "window=':Qt Creator_Core::Internal::MainWindow'}").plainText)
    test.log("Clone log is: %s" % vcsLog)
    test.verify("Executing in " + targetDir + ":" in vcsLog,
                "Searching for target directory in clone log")
    test.verify(" ".join(["clone", "--progress", cloneUrl, cloneDir]) in vcsLog,
                "Searching for git parameters in clone log")
    test.verify(canceled == (" terminated abnormally" in vcsLog),
                "Searching for result in clone log")
    clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))

def verifyFiles(targetDir):
    for file in [".gitignore", "CMakeLists.txt", "jom.pro",
                 os.path.join("bin", "ibjom.cmd"),
                 os.path.join("src", "app", "main.cpp")]:
        test.verify(os.path.exists(os.path.join(targetDir, cloneDir, file)),
                    "Verify the existence of %s" % file)

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    for button in ["Cancel immediately",
                   ":Git Repository Clone.Cancel_QPushButton",
                   ":Git Repository Clone.Finish_QPushButton"]:
        __createProjectOrFileSelectType__("  Import Project", "Git Clone")
        replaceEditorContent(waitForObject("{name='Repo' type='QLineEdit' visible='1'}"),
                             cloneUrl)
        targetDir = tempDir()
        replaceEditorContent(waitForObject(":Working Copy_Utils::BaseValidatingLineEdit"),
                             targetDir)
        cloneDirEdit = waitForObject("{name='Dir' type='QLineEdit' visible='1'}")
        test.compare(cloneDirEdit.text, "jom")
        replaceEditorContent(cloneDirEdit, cloneDir)
        clickButton(waitForObject(":Next_QPushButton"))
        cloneLog = waitForObject(":Git Repository Clone.logPlainTextEdit_QPlainTextEdit", 1000)
        test.compare(waitForObject(":Git Repository Clone.Result._QLabel").text,
                     "Running Git clone...")
        if button == "Cancel immediately":
            # wait for cloning to have started
            waitFor('len(str(cloneLog.plainText)) > 20 + len(cloneDir)')
            clickButton(":Git Repository Clone.Cancel_QPushButton")
            if not verifyCloneLog(targetDir, True):
                continue
            clickButton(":Git Repository Clone.Cancel_QPushButton")
        else:
            if not verifyCloneLog(targetDir, False):
                clickButton(":Git Repository Clone.Cancel_QPushButton")
                continue
            verifyFiles(targetDir)
            try:
                clickButton(waitForObject(button))
            except:
                cloneLog = waitForObject(":Git Repository Clone.logPlainTextEdit_QPlainTextEdit")
                test.fatal("Cloning failed",
                           str(cloneLog.plainText))
                clickButton(":Git Repository Clone.Cancel_QPushButton")
                continue
            if button == ":Git Repository Clone.Finish_QPushButton":
                try:
                    # CMake wizard shown
                    clickButton(waitForObject(":CMake Wizard.Cancel_QPushButton", 5000))
                    clickButton(waitForObject(":Cannot Open Project.OK_QPushButton"))
                    test.passes("The checked out project was being opened with CMake.")
                except:
                    try:
                        # QMake Project mode shown
                        clickButton(waitForObject(":*Qt Creator.Cancel_QPushButton", 5000))
                        test.passes("The checked out project was being opened with QMake.")
                    except:
                        clickButton(waitForObject(":Cannot Open Project.Show Details..._QPushButton"))
                        test.fail("The checked out project was not being opened.",
                                  waitForObject(":Cannot Open Project_QTextEdit").plainText)
                        clickButton(waitForObject(":Cannot Open Project.OK_QPushButton"))
        verifyVersionControlView(targetDir, button == "Cancel immediately")
    invokeMenuItem("File", "Exit")
