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
    pathCreator = srcPath + "/creator/qtcreator.pro"
    pathSpeedcrunch = srcPath + "/creator-test-data/speedcrunch/src/speedcrunch.pro"
    if not neededFilePresent(pathCreator) or not neededFilePresent(pathSpeedcrunch):
        return

    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return

    runButton = findObject(':*Qt Creator.Run_Core::Internal::FancyToolButton')
    openQmakeProject(pathSpeedcrunch, [Targets.DESKTOP_480_DEFAULT])
    # Wait for parsing to complete
    waitFor("runButton.enabled", 30000)
    # Starting before opening, because this is where Creator froze (QTCREATORBUG-10733)
    startopening = datetime.utcnow()
    openQmakeProject(pathCreator, [Targets.DESKTOP_561_DEFAULT])
    # Wait for parsing to complete
    startreading = datetime.utcnow()
    waitFor("runButton.enabled", 300000)
    secondsOpening = (datetime.utcnow() - startopening).seconds
    secondsReading = (datetime.utcnow() - startreading).seconds
    timeoutOpen = 45
    timeoutRead = 22
    test.verify(secondsOpening <= timeoutOpen, "Opening and reading qtcreator.pro took %d seconds. "
                "It should not take longer than %d seconds" % (secondsOpening, timeoutOpen))
    test.verify(secondsReading <= timeoutRead, "Just reading qtcreator.pro took %d seconds. "
                "It should not take longer than %d seconds" % (secondsReading, timeoutRead))

    naviTreeView = "{column='0' container=':Qt Creator_Utils::NavigationTreeView' text~='%s' type='QModelIndex'}"
    compareProjectTree(naviTreeView % "speedcrunch( \[\S+\])?", "projecttree_speedcrunch.tsv")
    compareProjectTree(naviTreeView % "qtcreator( \[\S+\])?", "projecttree_creator.tsv")

    # Verify warnings about old Qt version
    if not test.verify(object.exists(":Qt Creator_Core::OutputWindow"),
                       "Did the General Messages view show up?"):
        openGeneralMessages()
    # Verify messages appear once, from using default kit before configuring
    generalMessages = str(waitForObject(":Qt Creator_Core::OutputWindow").plainText)
    test.compare(generalMessages.count("Project MESSAGE: Cannot build Qt Creator with Qt version 5.3.1."), 1,
                 "Warning about outdated Qt shown?")
    test.compare(generalMessages.count("Project ERROR: Use at least Qt 5.5.0."), 1,
                 "Minimum Qt version shown?")

    # Verify that qmljs.g is in the project even when we don't know where (QTCREATORBUG-17609)
    selectFromLocator("p qmljs.g", "qmljs.g")
    # Now check some basic lookups in the search box
    selectFromLocator(": Qlist::QList", "QList::QList")
    test.compare(wordUnderCursor(waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "QList")

    invokeMenuItem("File", "Exit")

def init():
    cleanup()

def cleanup():
    # Make sure the .user files are gone
    cleanUpUserFiles([srcPath + "/creator-test-data/speedcrunch/src/speedcrunch.pro",
                      srcPath + "/creator/qtcreator.pro"])

