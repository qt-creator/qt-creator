# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

SpeedCrunchPath = ""

def buildConfigFromFancyToolButton(fancyToolButton):
    beginOfBuildConfig = "<b>Build:</b> "
    endOfBuildConfig = "<br/><b>Deploy:</b>"
    toolTipText = str(fancyToolButton.toolTip)
    beginIndex = toolTipText.find(beginOfBuildConfig) + len(beginOfBuildConfig)
    endIndex = toolTipText.find(endOfBuildConfig)
    return toolTipText[beginIndex:endIndex]

def main():
    if not neededFilePresent(SpeedCrunchPath):
        return
    startQC()
    if not startedWithoutPluginError():
        return
    openQmakeProject(SpeedCrunchPath, [Targets.DESKTOP_5_14_1_DEFAULT])
    waitForProjectParsing()

    fancyToolButton = waitForObject(":*Qt Creator_Core::Internal::FancyToolButton")

    availableConfigs = iterateBuildConfigs("Release")
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version (need Release build) - leaving without building.")
    for kit, config in availableConfigs:
        selectBuildConfig(kit, config)
        buildConfig = buildConfigFromFancyToolButton(fancyToolButton)
        if buildConfig != config:
            test.fatal("Build configuration %s is selected instead of %s" % (buildConfig, config))
            continue
        test.log("Testing build configuration: " + config)
        invokeMenuItem("Build", "Run qmake")
        waitForCompile()
        selectFromLocator("t rebuild", "Rebuild All Projects")
        waitForCompile(300000)
        checkCompile()
        checkLastBuild()

    # Add a new run configuration

    invokeMenuItem("File", "Exit")

def init():
    global SpeedCrunchPath
    SpeedCrunchPath = os.path.join(srcPath, "creator-test-data", "speedcrunch", "src", "speedcrunch.pro")
    cleanup()

def cleanup():
    # Make sure the .user files are gone
    cleanUpUserFiles(SpeedCrunchPath)
    for dir in glob.glob(os.path.join(srcPath, "creator-test-data", "speedcrunch", "speedcrunch-build-*")):
        deleteDirIfExists(dir)
