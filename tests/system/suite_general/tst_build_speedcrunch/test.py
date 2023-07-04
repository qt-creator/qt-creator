# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")


def buildConfigFromFancyToolButton(fancyToolButton):
    beginOfBuildConfig = "<b>Build:</b> "
    endOfBuildConfig = "<br/><b>Deploy:</b>"
    toolTipText = str(fancyToolButton.toolTip)
    beginIndex = toolTipText.find(beginOfBuildConfig) + len(beginOfBuildConfig)
    endIndex = toolTipText.find(endOfBuildConfig)
    return toolTipText[beginIndex:endIndex]

def main():
    with GitClone("https://bitbucket.org/heldercorreia/speedcrunch.git",
                  "release-0.12.0") as SpeedCrunchPath:
        if not SpeedCrunchPath:
            test.fatal("Could not clone SpeedCrunch")
            return
        startQC()
        if not startedWithoutPluginError():
            return
        openQmakeProject(os.path.join(SpeedCrunchPath, "src", "speedcrunch.pro"),
                         [Targets.DESKTOP_5_14_1_DEFAULT])
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
