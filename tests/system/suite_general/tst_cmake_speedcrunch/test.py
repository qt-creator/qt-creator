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

SpeedCrunchPath = ""
BuildPath = tempDir()

def cmakeSupportsServerMode():
    versionLines = filter(lambda line: "cmake version " in line,
                          getOutputFromCmdline(["cmake", "--version"]).splitlines())
    try:
        test.log("Using " + versionLines[0])
        matcher = re.match("cmake version (\d+)\.(\d+)\.\d+", versionLines[0])
        major = __builtin__.int(matcher.group(1))
        minor = __builtin__.int(matcher.group(2))
    except:
        return False
    if major < 3:
        return False
    elif major > 3:
        return True
    else:
        return minor >= 7

def main():
    if (which("cmake") == None):
        test.fatal("cmake not found in PATH - needed to run this test")
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
    naviTreeView = "{column='0' container=':Qt Creator_Utils::NavigationTreeView' text~='%s' type='QModelIndex'}"
    if cmakeSupportsServerMode():
        treeFile = "projecttree_speedcrunch_server.tsv"
    else:
        treeFile = "projecttree_speedcrunch.tsv"
    compareProjectTree(naviTreeView % "speedcrunch( \[\S+\])?", treeFile)

    if not cmakeSupportsServerMode() and JIRA.isBugStillOpen(18290):
        test.xfail("Building with cmake in Tealeafreader mode may fail", "QTCREATORBUG-18290")
    else:
        # Invoke a rebuild of the application
        invokeMenuItem("Build", "Rebuild All")

        # Wait for, and test if the build succeeded
        waitForCompile(300000)
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
