# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

SpeedCrunchPath = ""
BuildPath = tempDir()

def cmakeSupported():
    versionLines = filter(lambda line: "cmake version " in line,
                          getOutputFromCmdline(["cmake", "--version"]).splitlines())
    try:
        versionLine = list(versionLines)[0]
        test.log("Using " + versionLine)
        matcher = re.match("cmake version (\d+)\.(\d+)\.\d+", versionLine)
        major = __builtin__.int(matcher.group(1))
        minor = __builtin__.int(matcher.group(2))
    except:
        return False

    return (major, minor) >= (3, 14)

def main():
    if (which("cmake") == None):
        test.fatal("cmake not found in PATH - needed to run this test")
        return
    if not cmakeSupported():
        test.warning("CMake version is no more supported for QC")
        return
    if not neededFilePresent(SpeedCrunchPath):
        return

    startQC()
    if not startedWithoutPluginError():
        return
    result = openCmakeProject(SpeedCrunchPath, BuildPath)
    if not result:
        test.fatal("Could not open/create cmake project - leaving test")
        invokeMenuItem("File", "Exit")
        return
    waitForProjectParsing()
    naviTreeView = "{column='0' container=':Qt Creator_Utils::NavigationTreeView' text~='%s' type='QModelIndex'}"
    treeFile = "projecttree_speedcrunch.tsv"
    compareProjectTree(naviTreeView % "speedcrunch( \[\S+\])?", treeFile)

    # Invoke a rebuild of the application
    selectFromLocator("t rebuild", "Rebuild All Projects")

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
