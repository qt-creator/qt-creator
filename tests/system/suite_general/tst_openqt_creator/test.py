# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

def main():
    pathCreator = srcPath + "/creator/qtcreator.pro"
    if not neededFilePresent(pathCreator):
        return

    with GitClone("https://bitbucket.org/heldercorreia/speedcrunch.git",
                  "0.11") as pathSpeedcrunch:
        if not pathSpeedcrunch:
            test.fatal("Could not clone SpeedCrunch")
            return
        startQC(["-noload", "ClangCodeModel"])
        if not startedWithoutPluginError():
            return

        runButton = findObject(':*Qt Creator.Run_Core::Internal::FancyToolButton')
        openQmakeProject(os.path.join(pathSpeedcrunch, "src", "speedcrunch.pro"),
                         [Targets.DESKTOP_5_14_1_DEFAULT])
        # Wait for parsing to complete
        waitFor("runButton.enabled", 30000)
        # Starting before opening, because this is where Creator froze (QTCREATORBUG-10733)
        startopening = datetime.utcnow()
        openQmakeProject(pathCreator, [Targets.DESKTOP_5_14_1_DEFAULT])
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

        openGeneralMessages()
        # Verify that qmljs.g is in the project even when we don't know where (QTCREATORBUG-17609)
        selectFromLocator("p qmljs.g", "qmljs.g")
        # Now check some basic lookups in the search box
        selectFromLocator(": qlist", "QList")
        test.compare(wordUnderCursor(waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")), "QList")

        invokeMenuItem("File", "Exit")

def init():
    cleanup()

def cleanup():
    # Make sure the .user files are gone
    cleanUpUserFiles([srcPath + "/creator/qtcreator.pro"])

