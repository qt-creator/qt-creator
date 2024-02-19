# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

def main():
    with GitClone("git://code.qt.io/qt-creator/qt-creator.git",
                  "v6.0.1") as CreatorSrcPath:
        if not CreatorSrcPath:
            test.fatal("Could not clone Qt Creator")
            return
        pathCreator = os.path.join(CreatorSrcPath, "qtcreator.qbs")

        startQC(["-noload", "ClangCodeModel"])
        if not startedWithoutPluginError():
            return
        openQbsProject(pathCreator)
        if not addAndActivateKit(Targets.DESKTOP_5_14_1_DEFAULT):
            test.fatal("Failed to activate '%s'"
                       % Targets.getStringForTarget(Targets.DESKTOP_5_10_1_DEFAULT))
            invokeMenuItem("File", "Exit")
            return
        test.log("Start parsing project")
        rootNodeTemplate = ("{column='0' container=':Qt Creator_Utils::NavigationTreeView' "
                            "text~='%s( \[\S+\])?' type='QModelIndex'}")
        ntwObject = waitForObject(rootNodeTemplate % "Qt Creator", 200000)
        if waitFor("ntwObject.model().rowCount(ntwObject) > 2", 20000): # No need to wait for C++,
            test.log("Parsing project done")                            # we only need the project
        else:
            test.warning("Parsing project timed out")
        compareProjectTree(rootNodeTemplate % "Qt Creator", "projecttree_creator.tsv")
        buildIssuesTexts = map(lambda i: str(i[0]), getBuildIssues(False))
        deprecationWarnings = "\n".join(set(filter(lambda s: "deprecated" in s, buildIssuesTexts)))
        if deprecationWarnings:
            test.warning("Creator claims that the .qbs file uses deprecated features.",
                         deprecationWarnings)
        invokeMenuItem("File", "Exit")
