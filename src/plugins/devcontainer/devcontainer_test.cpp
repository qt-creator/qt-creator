// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>

#include <devcontainer/devcontainer.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/infobar.h>
#include <utils/mimeutils.h>
#include <utils/qtcprocess.h>

#include <coreplugin/icore.h>

using namespace Utils;

namespace DevContainer::Internal {

static bool testDocker(const FilePath &executable)
{
    Process p;
    p.setCommand({executable, {"info", "--format", "{{.OSType}}"}});
    p.runBlocking();
    const QString platform = p.cleanedStdOut().trimmed();
    return p.result() == ProcessResult::FinishedWithSuccess && platform == "linux";
}

static bool testDockerMount(const FilePath &executable, const FilePath &testDir)
{
    Process p;
    p.setCommand(
        {executable,
         {"run",
          "--rm",
          "--mount",
          "type=bind,source=" + testDir.path() + ",target=/mnt/test",
          "alpine:latest",
          "ls",
          "/mnt/test"}});
    p.runBlocking();
    if (p.result() != ProcessResult::FinishedWithSuccess) {
        qWarning() << "Docker mount test failed:" << p.verboseExitMessage();
        return false;
    }
    return p.result() == ProcessResult::FinishedWithSuccess;
}

class Tests : public QObject
{
    Q_OBJECT

    const FilePath testData{TESTDATA};

private slots:
    void initTestCase()
    {
        if (!testDocker("docker"))
            QSKIP("Docker is not set up correctly, skipping tests.");

        if (!testDockerMount("docker", testData))
            QSKIP("Docker mount test failed, skipping tests.");
    }

    void testSimpleProject()
    {
        const auto cmakelists = testData / "simpleproject" / "CMakeLists.txt";
        ProjectExplorer::OpenProjectResult opr
            = ProjectExplorer::ProjectExplorerPlugin::openProject(cmakelists);

        QVERIFY(opr);

        InstanceConfig instanceConfig;
        instanceConfig.configFilePath = testData / "simpleproject" / ".devcontainer"
                                        / "devcontainer.json";
        instanceConfig.workspaceFolder = opr.project()->projectDirectory();

        const auto infoBarEntryId = Utils::Id::fromString(
            QString("DevContainer.Instantiate.InfoBar." + instanceConfig.devContainerId()));

        Utils::InfoBar *infoBar = Core::ICore::infoBar();
        QVERIFY(infoBar->containsInfo(infoBarEntryId));
        Utils::InfoBarEntry entry
            = Utils::findOrDefault(infoBar->entries(), [infoBarEntryId](const Utils::InfoBarEntry &e) {
                  return e.id() == infoBarEntryId;
              });

        QCOMPARE(entry.id(), infoBarEntryId);
        QCOMPARE(entry.buttons().size(), 1);
        auto yesButton = entry.buttons().first();

        // Trigger loading the DevContainer instance
        yesButton.callback();
        FilePath expectedRootPath
            = FilePath::fromParts(u"devcontainer", instanceConfig.devContainerId(), u"/");
        QVERIFY(expectedRootPath.exists());
        QVERIFY(!infoBar->containsInfo(infoBarEntryId));

        FilePath expectedLibExecMountPoint = FilePath::fromParts(
            u"devcontainer", instanceConfig.devContainerId(), u"/custom/libexec/mnt/point");
        QVERIFY(expectedLibExecMountPoint.exists());
        QVERIFY(expectedLibExecMountPoint.isReadableDir());

        FilePath expectedWorkspaceFolder = FilePath::fromParts(
            u"devcontainer", instanceConfig.devContainerId(), u"/custom/workspace");
        QVERIFY(expectedWorkspaceFolder.exists());
        QVERIFY(expectedWorkspaceFolder.isReadableDir());

        FilePath expectedMainCpp = expectedWorkspaceFolder / "main.cpp";
        QVERIFY(expectedMainCpp.exists());
        QVERIFY(expectedMainCpp.isReadableFile());

        ProjectExplorer::ProjectManager::removeProject(opr.project());
        QVERIFY(!expectedRootPath.exists());
    }
};

QObject *createDevcontainerTest()
{
    return new Tests;
}

} // namespace DevContainer::Internal

#include "devcontainer_test.moc"
