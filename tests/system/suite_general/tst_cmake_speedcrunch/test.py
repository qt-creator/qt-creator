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

SpeedCrunchPath = ""
BuildPath = tempDir()

def main():
    if (which("cmake") == None):
        test.fatal("cmake not found in PATH - needed to run this test")
        return
    if (which("qmake") == None):
        test.fatal("qmake not found in PATH - needed to run this test")
        return
    if not neededFilePresent(SpeedCrunchPath):
        return

    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    result = openCmakeProject(SpeedCrunchPath, BuildPath)
    if not result:
        test.fatal("Could not open/create cmake project - leaving test")
        invokeMenuItem("File", "Exit")
        return
    progressBarWait(30000)

    # Invoke a rebuild of the application
    invokeMenuItem("Build", "Rebuild All")

    # Wait for, and test if the build succeeded
    waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)", 300000)
    checkCompile()
    checkLastBuild()

    invokeMenuItem("File", "Exit")

def init():
    global SpeedCrunchPath
    SpeedCrunchPath = srcPath + "/creator-test-data/speedcrunch/src/CMakeLists.txt"
    cleanup()

def cleanup():
    global BuildPath
    # Make sure the .user files are gone
    cleanUpUserFiles(SpeedCrunchPath)
    deleteDirIfExists(BuildPath)
