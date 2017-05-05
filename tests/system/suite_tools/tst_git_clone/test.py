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

cloneUrl = "https://code.qt.io/installer-framework/installer-framework.git"
cloneDir = "myCloneOfIfw"

def verifyCloneLog(targetDir, canceled):
    if canceled:
        summary = "Failed."
    else:
        finish = findObject(":Git Repository Clone.Finish_QPushButton")
        waitFor("finish.enabled", 90000)
        cloneLog = str(waitForObject(":Git Repository Clone.logPlainTextEdit_QPlainTextEdit").plainText)
        if "fatal: " in cloneLog:
            test.warning("Cloning failed outside Creator.")
            return False
        # test for QTCREATORBUG-10112
        test.compare(cloneLog.count("remote: Total"), 1)
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
    vcsLog = str(waitForObject("{type='Core::OutputWindow' unnamed='1' visible='1' "
                               "window=':Qt Creator_Core::Internal::MainWindow'}").plainText)
    test.log("Clone log is: %s" % vcsLog)
    test.verify("Running in " + targetDir + ":" in vcsLog,
                "Searching for target directory in clone log")
    test.verify(" ".join(["clone", "--progress", cloneUrl, cloneDir]) in vcsLog,
                "Searching for git parameters in clone log")
    test.verify(canceled == (" terminated abnormally" in vcsLog),
                "Searching for result in clone log")
    clickButton(waitForObject(":*Qt Creator.Clear_QToolButton"))

def verifyFiles(targetDir):
    for file in [".gitignore", "LICENSE.GPL3-EXCEPT", "installerfw.pro",
                 os.path.join("tests", "test-installer", "create-test-installer.bat"),
                 os.path.join("src", "sdk", "main.cpp")]:
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
        test.compare(cloneDirEdit.text, "installer-framework")
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
                    # Projects mode shown
                    clickButton(waitForObject(":Qt Creator.Configure Project_QPushButton", 5000))
                    test.passes("The checked out project was being opened.")
                except:
                    clickButton(waitForObject(":Cannot Open Project.Show Details..._QPushButton"))
                    test.fail("The checked out project was not being opened.",
                              str(waitForObject(":Cannot Open Project_QTextEdit").plainText))
                    clickButton(waitForObject(":Cannot Open Project.OK_QPushButton"))
        verifyVersionControlView(targetDir, button == "Cancel immediately")
    invokeMenuItem("File", "Exit")
