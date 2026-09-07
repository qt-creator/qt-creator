// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "pylspprune_test.h"
#include "../pythonsettings.h"

#include <utils/filepath.h>
#include <utils/hostosinfo.h>

#include <QTemporaryDir>
#include <QTest>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python::Internal {

static void createInstallation(const FilePath &root, const QString &version)
{
    const FilePath binDir = root / version / "bin";
    QVERIFY_RESULT(binDir.ensureWritableDir());
    QVERIFY_RESULT(binDir.pathAppended("pylsp").writeFileContents("marker"));
}

static bool hasInstallation(const FilePath &root, const QString &version)
{
    return (root / version / "bin" / "pylsp").exists();
}

// An interpreter whose file is there, but which answers "--version" with a failure.
static void createFailingPython(const FilePath &dir, FilePath *python)
{
    if (HostOsInfo::isWindowsHost()) {
        *python = dir / "failing-python.bat";
        QVERIFY_RESULT(python->writeFileContents("@exit /b 1\r\n"));
        return;
    }
    *python = dir / "failing-python";
    QVERIFY_RESULT(python->writeFileContents("#!/bin/sh\nexit 1\n"));
    QVERIFY_RESULT(python->setPermissions(
        QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
}

void PylspPruneTest::testRemovesInstallationsWithoutInterpreter()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath root = FilePath::fromUserInput(tmp.path()) / "pylsp";

    createInstallation(root, "Python 3.14.4");
    createInstallation(root, "Python 2.7.18");

    prunePylspInstallations(root, {"Python 3.14.4"});

    QVERIFY(hasInstallation(root, "Python 3.14.4"));
    QVERIFY(!(root / "Python 2.7.18").exists());
}

void PylspPruneTest::testRemovesEverythingWithoutVersions()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath root = FilePath::fromUserInput(tmp.path()) / "pylsp";

    createInstallation(root, "Python 3.14.4");

    // No local interpreter left means no installation can still be used.
    prunePylspInstallations(root, {});

    QVERIFY(!(root / "Python 3.14.4").exists());
    QVERIFY(root.isDir());
}

void PylspPruneTest::testIgnoresAMissingRoot()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath root = FilePath::fromUserInput(tmp.path()) / "pylsp";

    prunePylspInstallations(root, {"Python 3.14.4"});

    QVERIFY(!root.exists());
}

void PylspPruneTest::testReportsNoVersionForAnUnreadableInterpreter()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    FilePath python;
    createFailingPython(FilePath::fromUserInput(tmp.path()), &python);
    QVERIFY(python.isExecutableFile());

    const Interpreter interpreter("failing", "Failing Python", python);

    // Nothing may be pruned while an interpreter that is there stays unreadable.
    QVERIFY(!activePythonVersions({interpreter}).has_value());
}

void PylspPruneTest::testReportsTheVersionOfAReadableInterpreter()
{
    const FilePath python = FilePath::fromUserInput("python3").searchInPath();
    if (!python.isExecutableFile())
        QSKIP("No python3 on PATH to read a version from");

    const Interpreter interpreter("readable", "Python from PATH", python);

    const std::optional<QStringList> versions = activePythonVersions({interpreter});
    QVERIFY(versions.has_value());
    QCOMPARE(versions->size(), 1);
    QVERIFY(versions->first().startsWith("Python "));
}

QObject *createPylspPruneTest()
{
    return new PylspPruneTest;
}

} // namespace Python::Internal

#endif // WITH_TESTS
