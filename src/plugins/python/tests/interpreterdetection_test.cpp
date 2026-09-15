// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "interpreterdetection_test.h"

#include "../pythonkitaspect.h"
#include "../pythonsettings.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/globaltasktree.h>

#include <QFont>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Python::Internal {

// A file the detection accepts as a Python: it only has to be executable and answer with
// something that parses as a version.
static FilePath writeStubInterpreter(const QTemporaryDir &dir, const QString &name)
{
    const FilePath python = (FilePath::fromUserInput(dir.path()) / name).withExecutableSuffix();
    if (!python.writeFileContents("#!/bin/sh\necho \"Python 3.13.0\"\n"))
        return {};
    if (!python.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner))
        return {};
    return python;
}

void InterpreterDetectionTest::testReportsAnEmptySearch()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    // The logger is copied into the detection and outlives this function when the wait
    // below times out, so the lines it writes to must not live on this stack.
    const auto lines = std::make_shared<QStringList>();
    const ToolDetectionLogger logger([lines](const QString &message) { *lines << message; });

    // The signal reaches every tool detection handler, not just this plugin's, so the
    // temporary directory is what keeps the others from doing anything: it is empty, and
    // nothing outside it is searched.
    // A token of 0 leaves the device's task bookkeeping alone, see
    // IDevice::registerToolDetectionTask().
    emit DeviceManager::instance()->toolDetectionRequested(
        ProjectExplorer::Constants::DESKTOP_DEVICE_ID,
        {FilePath::fromUserInput(tmp.path())},
        0,
        logger);

    const auto reported = [lines](const QString &message) {
        return Utils::contains(*lines, [&message](const QString &line) {
            return line.contains(message);
        });
    };

    QTRY_VERIFY(reported("No new Python interpreters found."));
    QVERIFY(reported("Searching for Python interpreters..."));
}

void InterpreterDetectionTest::testRegistersDetectedInterpreters()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath python = writeStubInterpreter(tmp, "python3");
    QVERIFY(!python.isEmpty());

    const QList<PythonSettings::Interpreter> before = PythonSettings::interpreters();
    const QString defaultId = PythonSettings::defaultInterpreterId();

    // Whatever the detection registered, including the kit it generated for it, is gone
    // again when this test leaves, also on a failing assertion.
    const QScopeGuard restoreInterpreters(
        [before, defaultId] { PythonSettings::setInterpreter(before, defaultId); });

    emit DeviceManager::instance()->toolDetectionRequested(
        ProjectExplorer::Constants::DESKTOP_DEVICE_ID, {python.parentDir()}, 0, {});

    QTRY_COMPARE(PythonSettings::interpreters().size(), before.size() + 1);
    QCOMPARE(PythonSettings::interpreters().last().command, python);
}

void InterpreterDetectionTest::testAssignsAKnownInterpreterToAKit()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath python = writeStubInterpreter(tmp, "python3");
    QVERIFY(!python.isEmpty());

    const QList<PythonSettings::Interpreter> before = PythonSettings::interpreters();
    const QString defaultId = PythonSettings::defaultInterpreterId();
    const QScopeGuard restoreInterpreters(
        [before, defaultId] { PythonSettings::setInterpreter(before, defaultId); });

    // Register it the way a tool detection on the device does, so that the kit detection
    // below searches the same paths and finds nothing new.
    emit DeviceManager::instance()->toolDetectionRequested(
        ProjectExplorer::Constants::DESKTOP_DEVICE_ID, {python.parentDir()}, 0, {});
    QTRY_COMPARE(PythonSettings::interpreters().size(), before.size() + 1);

    Kit *kit = KitManager::registerKit({});
    QVERIFY(kit);
    const QScopeGuard deregisterKit([kit] { KitManager::deregisterKit(kit); });
    QVERIFY(!PythonKitAspect::python(kit));

    // The detection outlives this function when the wait below times out.
    const auto done = std::make_shared<bool>(false);
    GlobalTaskTree::start(
        Group{PythonSettings::autoDetect(kit, {python.parentDir()}, {}, [](const QString &) {})},
        {},
        [done] { *done = true; });
    QTRY_VERIFY(*done);

    const std::optional<Interpreter> assigned = PythonKitAspect::python(kit);
    QVERIFY(assigned);
    QCOMPARE(assigned->command, python);
}

void InterpreterDetectionTest::testAssignsAnInterpreterRegisteredUnderASymlinkTarget()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath target = writeStubInterpreter(tmp, "python3.12");
    QVERIFY(!target.isEmpty());
    const FilePath link = (FilePath::fromUserInput(tmp.path()) / "python3").withExecutableSuffix();
    if (!target.createSymLink(link))
        QSKIP("Cannot create a symbolic link here.");

    const QList<PythonSettings::Interpreter> before = PythonSettings::interpreters();
    const QString defaultId = PythonSettings::defaultInterpreterId();
    const QScopeGuard restoreInterpreters(
        [before, defaultId] { PythonSettings::setInterpreter(before, defaultId); });

    // Registered under the name the symbolic link points to, not the one the search builds.
    PythonSettings::addInterpreter(PythonSettings::createInterpreter(target, "Target"));

    Kit *kit = KitManager::registerKit({});
    QVERIFY(kit);
    const QScopeGuard deregisterKit([kit] { KitManager::deregisterKit(kit); });

    const auto done = std::make_shared<bool>(false);
    GlobalTaskTree::start(
        Group{PythonSettings::autoDetect(kit, {target.parentDir()}, {}, [](const QString &) {})},
        {},
        [done] { *done = true; });
    QTRY_VERIFY(*done);

    const std::optional<Interpreter> assigned = PythonKitAspect::python(kit);
    QVERIFY(assigned);
    QCOMPARE(assigned->command, target);
}

void InterpreterDetectionTest::testRegistersOnceWhenSearchPathsResolveToTheSameBinary()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath python = writeStubInterpreter(tmp, "python3");
    QVERIFY(!python.isEmpty());
    const FilePath otherDir = FilePath::fromUserInput(tmp.path()) / "other";
    QVERIFY(otherDir.ensureWritableDir());
    if (!python.createSymLink((otherDir / "python3").withExecutableSuffix()))
        QSKIP("Cannot create a symbolic link here.");

    const QList<PythonSettings::Interpreter> before = PythonSettings::interpreters();
    const QString defaultId = PythonSettings::defaultInterpreterId();
    const QScopeGuard restoreInterpreters(
        [before, defaultId] { PythonSettings::setInterpreter(before, defaultId); });

    const auto done = std::make_shared<bool>(false);
    GlobalTaskTree::start(
        Group{PythonSettings::autoDetect(
            nullptr, {python.parentDir(), otherDir}, {}, [](const QString &) {})},
        {},
        [done] { *done = true; });
    QTRY_VERIFY(*done);

    QCOMPARE(PythonSettings::interpreters().size(), before.size() + 1);
}

void InterpreterDetectionTest::testDetectionIgnoresUnappliedRowsOfThePage()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath python = writeStubInterpreter(tmp, "python3");
    QVERIFY(!python.isEmpty());

    const QList<PythonSettings::Interpreter> before = PythonSettings::interpreters();
    const QString defaultId = PythonSettings::defaultInterpreterId();
    const QScopeGuard restoreInterpreters(
        [before, defaultId] { PythonSettings::setInterpreter(before, defaultId); });

    InterpreterModel &model = interpreterModel();
    PythonSettings::addInterpreter(PythonSettings::createInterpreter(python, "Registered"));
    QCOMPARE(PythonSettings::interpreters().size(), before.size() + 1);

    // The user deleted the row on the page without pressing Apply yet.
    model.destroyItem(model.itemAt(model.rowCount() - 1));

    const auto done = std::make_shared<bool>(false);
    GlobalTaskTree::start(
        Group{PythonSettings::autoDetect(nullptr, {python.parentDir()}, {}, [](const QString &) {})},
        {},
        [done] { *done = true; });
    QTRY_VERIFY(*done);

    QCOMPARE(PythonSettings::interpreters().size(), before.size() + 1);
}

void InterpreterDetectionTest::testDetectsForAKitWithoutAKitOfItsOwn()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath python = writeStubInterpreter(tmp, "python3");
    QVERIFY(!python.isEmpty());

    const QList<PythonSettings::Interpreter> before = PythonSettings::interpreters();
    const QString defaultId = PythonSettings::defaultInterpreterId();
    const QScopeGuard restoreInterpreters(
        [before, defaultId] { PythonSettings::setInterpreter(before, defaultId); });

    Kit *kit = KitManager::registerKit({});
    QVERIFY(kit);
    const QScopeGuard deregisterKit([kit] { KitManager::deregisterKit(kit); });
    const int kitCount = KitManager::kits().size();

    const auto done = std::make_shared<bool>(false);
    GlobalTaskTree::start(
        Group{PythonSettings::autoDetect(kit, {python.parentDir()}, {}, [](const QString &) {})},
        {},
        [done] { *done = true; });
    QTRY_VERIFY(*done);

    QCOMPARE(PythonSettings::interpreters().size(), before.size() + 1);
    const std::optional<Interpreter> assigned = PythonKitAspect::python(kit);
    QVERIFY(assigned);
    QCOMPARE(assigned->command, python);
    QCOMPARE(KitManager::kits().size(), kitCount);
}

void InterpreterDetectionTest::testKeepsUnappliedEditsOnASettingsWrite()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath shownPython = writeStubInterpreter(tmp, "python3");
    QVERIFY(!shownPython.isEmpty());
    const FilePath addedPython = writeStubInterpreter(tmp, "python3.14");
    QVERIFY(!addedPython.isEmpty());

    const QList<PythonSettings::Interpreter> before = PythonSettings::interpreters();
    const QString defaultId = PythonSettings::defaultInterpreterId();
    const QScopeGuard restoreInterpreters(
        [before, defaultId] { PythonSettings::setInterpreter(before, defaultId); });

    InterpreterModel &model = interpreterModel();
    QCOMPARE(model.interpreters().size(), before.size());

    PythonSettings::addInterpreter(
        PythonSettings::createInterpreter(shownPython, "Shown"));
    QCOMPARE(model.interpreters().size(), before.size() + 1);

    // What the user typed into the Interpreters page without pressing Apply yet.
    const int edited = model.interpreters().size() - 1;
    model.itemAt(edited)->itemData.name = "Edited";

    // A write from elsewhere, the way a venv creation or a device tool detection ends.
    PythonSettings::addInterpreter(
        PythonSettings::createInterpreter(addedPython, "Added"));

    const QList<PythonSettings::Interpreter> shown = model.interpreters();
    QCOMPARE(shown.size(), before.size() + 2);
    QCOMPARE(shown.at(edited).name, QString("Edited"));
    QCOMPARE(shown.last().command, addedPython);

    // A write that changes a row the user did not touch, and keeps its id.
    QList<PythonSettings::Interpreter> renamed = PythonSettings::interpreters();
    renamed.last().name = "Renamed";
    PythonSettings::setInterpreter(renamed, PythonSettings::defaultInterpreterId());

    const QList<PythonSettings::Interpreter> followed = model.interpreters();
    QCOMPARE(followed.size(), before.size() + 2);
    QCOMPARE(followed.at(edited).name, QString("Edited"));
    QCOMPARE(followed.last().name, QString("Renamed"));
}

void InterpreterDetectionTest::testFollowsADefaultChangedElsewhere()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath firstPython = writeStubInterpreter(tmp, "python3");
    QVERIFY(!firstPython.isEmpty());
    const FilePath secondPython = writeStubInterpreter(tmp, "python3.14");
    QVERIFY(!secondPython.isEmpty());

    const QList<PythonSettings::Interpreter> before = PythonSettings::interpreters();
    const QString defaultId = PythonSettings::defaultInterpreterId();
    const QScopeGuard restoreInterpreters(
        [before, defaultId] { PythonSettings::setInterpreter(before, defaultId); });

    InterpreterModel &model = interpreterModel();
    const auto isShownAsDefault = [&model](const QString &id) {
        const QModelIndex index = model.findIndex(Utils::equal(&Interpreter::id, id));
        return model.data(index, Qt::FontRole).value<QFont>().bold();
    };

    const PythonSettings::Interpreter first
        = PythonSettings::createInterpreter(firstPython, "First");
    PythonSettings::addInterpreter(first, true);
    QVERIFY(isShownAsDefault(first.id));

    // The old default is still there, but the page has no "Make Default" of its own to keep.
    const PythonSettings::Interpreter second
        = PythonSettings::createInterpreter(secondPython, "Second");
    int fontChanges = 0;
    const QMetaObject::Connection counter = QObject::connect(
        &model, &QAbstractItemModel::dataChanged, &model,
        [&fontChanges](const QModelIndex &, const QModelIndex &, const QList<int> &roles) {
            fontChanges += roles.contains(Qt::FontRole);
        });
    const QScopeGuard disconnectCounter([counter] { QObject::disconnect(counter); });
    PythonSettings::addInterpreter(second, true);
    QVERIFY(isShownAsDefault(second.id));
    QVERIFY(!isShownAsDefault(first.id));
    // A view only repaints the rows it is told about.
    QCOMPARE(fontChanges, 2);
}

void InterpreterDetectionTest::testRegistersOnceWhenDetectionsOverlap()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath python = writeStubInterpreter(tmp, "python3");
    QVERIFY(!python.isEmpty());

    const QList<PythonSettings::Interpreter> before = PythonSettings::interpreters();
    const QString defaultId = PythonSettings::defaultInterpreterId();
    const QScopeGuard restoreInterpreters(
        [before, defaultId] { PythonSettings::setInterpreter(before, defaultId); });

    const auto finished = std::make_shared<int>(0);
    for (int i = 0; i < 2; ++i) {
        GlobalTaskTree::start(
            Group{PythonSettings::autoDetect(
                nullptr, {python.parentDir()}, {}, [](const QString &) {})},
            {},
            [finished] { ++*finished; });
    }
    QTRY_COMPARE(*finished, 2);

    QCOMPARE(PythonSettings::interpreters().size(), before.size() + 1);
}

void InterpreterDetectionTest::testKeepsTheDefaultWhenRemovingDetectedInterpreters()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath python = writeStubInterpreter(tmp, "python3");
    QVERIFY(!python.isEmpty());

    const QList<PythonSettings::Interpreter> before = PythonSettings::interpreters();
    const QString defaultId = PythonSettings::defaultInterpreterId();
    const QScopeGuard restoreInterpreters(
        [before, defaultId] { PythonSettings::setInterpreter(before, defaultId); });

    const DetectionSource source(DetectionSource::FromSystem, "interpreterdetection-test");
    const PythonSettings::Interpreter interpreter
        = PythonSettings::createInterpreter(python, "Stub", {}, source);
    PythonSettings::addInterpreter(interpreter, true);
    QCOMPARE(PythonSettings::defaultInterpreterId(), interpreter.id);

    PythonSettings::removeDetectedPython(source.id, [](const QString &) {});

    const QList<PythonSettings::Interpreter> remaining = PythonSettings::interpreters();
    QVERIFY(!Utils::contains(remaining, Utils::equal(&Interpreter::id, interpreter.id)));
    // The default is persisted now, so it must not name an interpreter that is gone.
    const QString newDefaultId = PythonSettings::defaultInterpreterId();
    QVERIFY(newDefaultId.isEmpty()
            || Utils::contains(remaining, Utils::equal(&Interpreter::id, newDefaultId)));
}

QObject *createInterpreterDetectionTest()
{
    return new InterpreterDetectionTest;
}

} // namespace Python::Internal

#endif // WITH_TESTS
