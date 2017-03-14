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
    if platform.system() == 'Darwin':
        test.warning("This needs a Qt 5.4 kit. Skipping it.")
        return
    pathCreator = os.path.join(srcPath, "creator", "qtcreator.qbs")
    if not neededFilePresent(pathCreator):
        return

    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    openQbsProject(pathCreator)
    if not addAndActivateKit(Targets.DESKTOP_541_GCC):
        test.fatal("Failed to activate '%s'" % Targets.getStringForTarget(Targets.DESKTOP_541_GCC))
        invokeMenuItem("File", "Exit")
        return
    test.log("Start parsing project")
    rootNodeTemplate = "{column='0' container=':Qt Creator_Utils::NavigationTreeView' text~='%s( \[\S+\])?' type='QModelIndex'}"
    ntwObject = waitForObject(rootNodeTemplate % "Qt Creator", 200000)
    if waitFor("ntwObject.model().rowCount(ntwObject) > 2", 20000):     # No need to wait for C++-parsing
        test.log("Parsing project done")                                # we only need the project
    else:
        test.warning("Parsing project timed out")
    compareProjectTree(rootNodeTemplate % "Qt Creator", "projecttree_creator.tsv")
    invokeMenuItem("File", "Exit")
