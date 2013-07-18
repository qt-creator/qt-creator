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
    suitableKits = Targets.DESKTOP_474_GCC | Targets.DESKTOP_480_GCC
    if platform.system() in ('Windows', 'Microsoft'):
        suitableKits |= Targets.DESKTOP_480_MSVC2010
    checkedTargets = openQmakeProject(SpeedCrunchPath, suitableKits)
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
        waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)")
        invokeMenuItem("Build", "Rebuild All")
        waitForSignal("{type='ProjectExplorer::BuildManager' unnamed='1'}", "buildQueueFinished(bool)", 300000)
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
