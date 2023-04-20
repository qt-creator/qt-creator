# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

# entry of test
def main():
    # prepare example project
    sourceExample = os.path.join(QtPath.examplesPath(Targets.DESKTOP_5_14_1_DEFAULT),
                                 "quick", "animation")
    proFile = "animation.pro"
    if not neededFilePresent(os.path.join(sourceExample, proFile)):
        return
    # copy example project to temp directory
    templateDir = prepareTemplate(sourceExample, "/../shared")
    examplePath = os.path.join(templateDir, proFile)
    startQC()
    if not startedWithoutPluginError():
        return
    # open example project, supports only Qt 5
    targets = Targets.desktopTargetClasses()
    targets.discard(Targets.DESKTOP_5_4_1_GCC)
    openQmakeProject(examplePath, targets)
    # build and wait until finished - on all build configurations
    availableConfigs = iterateBuildConfigs()
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version - leaving without building.")
    for kit, config in availableConfigs:
        selectBuildConfig(kit, config)
        # try to build project
        test.log("Testing build configuration: " + config)
        invokeMenuItem("Build", "Build All Projects")
        waitForCompile()
        # verify build successful
        ensureChecked(waitForObject(":Qt Creator_CompileOutput_Core::Internal::OutputPaneToggleButton"))
        compileOutput = waitForObject(":Qt Creator.Compile Output_Core::OutputWindow")
        if not test.verify(compileSucceeded(compileOutput.plainText),
                           "Verifying building of existing complex qt application."):
            test.log(str(compileOutput.plainText))
    # exit
    invokeMenuItem("File", "Exit")
# no cleanup needed, as whole testing directory gets properly removed after test finished

