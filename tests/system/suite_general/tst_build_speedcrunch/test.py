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
import re

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
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    checkedTargets = openQmakeProject(SpeedCrunchPath, Targets.DESKTOP_480_DEFAULT)
    progressBarWait(30000)

    fancyToolButton = waitForObject(":*Qt Creator_Core::Internal::FancyToolButton")

    availableConfigs = iterateBuildConfigs(len(checkedTargets), "Release")
    if not availableConfigs:
        test.fatal("Haven't found a suitable Qt version (need Release build) - leaving without building.")
    for kit, config in availableConfigs:
        selectBuildConfig(len(checkedTargets), kit, config)
        buildConfig = buildConfigFromFancyToolButton(fancyToolButton)
        if buildConfig != config:
            test.fatal("Build configuration %s is selected instead of %s" % (buildConfig, config))
            continue
        test.log("Testing build configuration: " + config)
        invokeMenuItem("Build", "Run qmake")
        waitForCompile()
        invokeMenuItem("Build", "Rebuild All")
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
