#############################################################################
##
## Copyright (C) 2015 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company.  For licensing terms and
## conditions see http://www.qt.io/terms-conditions.  For further information
## use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file.  Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, The Qt Company gives you certain additional
## rights.  These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

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
    openQmakeProject(pathSpeedcrunch, Targets.DESKTOP_480_DEFAULT)
    # Wait for parsing to complete
    waitFor("runButton.enabled", 30000)
    # Starting before opening, because this is where Creator froze (QTCREATORBUG-10733)
    startopening = datetime.utcnow()
    openQmakeProject(pathCreator, Targets.DESKTOP_531_DEFAULT)
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
    generalMessages = str(waitForObject(":Qt Creator_Core::OutputWindow").plainText)
    test.verify("Project MESSAGE: Cannot build Qt Creator with Qt version 5.3.1." in generalMessages,
                "Warning about outdated Qt shown?")
    test.verify("Project ERROR: Use at least Qt 5.4.0." in generalMessages,
                "Minimum Qt version shown?")

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

