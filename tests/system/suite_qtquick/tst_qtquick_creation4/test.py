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

def main():
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    for target in [Targets.DESKTOP_561_DEFAULT, Targets.DESKTOP_531_DEFAULT]:
        # using a temporary directory won't mess up a potentially existing
        createNewQmlExtension(tempDir(), [target])
        # wait for parsing to complete
        progressBarWait(30000)
        test.log("Building project Qt Quick 2 Extension Plugin (%s)"
                 % Targets.getStringForTarget(target))
        invokeMenuItem("Build","Build All")
        waitForCompile()
        checkCompile()
        checkLastBuild()
        invokeMenuItem("File", "Close All Projects and Editors")
    invokeMenuItem("File", "Exit")
