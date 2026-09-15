// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "pythonsettings_test.h"

#include "../pythonkitaspect.h"
#include "../pythonsettings.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>

#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#include <QtTaskTree/QTaskTree>

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Python::Internal {

class PythonTestDevice final : public IDevice
{
public:
    explicit PythonTestDevice(Id id)
    {
        setupId(AutoDetected, id);
        setType("Python.TestDevice");
    }

private:
    IDeviceWidget *createWidget() final { return nullptr; }
};

void PythonSettingsTest::testDropsADefaultThatIsGone()
{
    const QList<Interpreter> before = PythonSettings::interpreters();
    const QString defaultIdBefore = PythonSettings::defaultInterpreterId();

    // Not the interpreter from PATH, so it cannot be picked as the replacement default.
    const Interpreter extra("python-settings-test-id",
                            "Python Settings Test",
                            FilePath::fromUserInput("/does/not/exist/python3"));
    PythonSettings::setInterpreter(before + QList<Interpreter>{extra}, extra.id);
    QCOMPARE(PythonSettings::defaultInterpreterId(), extra.id);

    PythonSettings::setInterpreter(before, extra.id);
    const QString defaultId = PythonSettings::defaultInterpreterId();
    QVERIFY(defaultId.isEmpty()
            || Utils::contains(PythonSettings::interpreters(),
                               Utils::equal(&Interpreter::id, defaultId)));

    PythonSettings::setInterpreter(before, defaultIdBefore);
}

void PythonSettingsTest::testDetectionReachesKitsAndSettings()
{
    if (HostOsInfo::isWindowsHost())
        QSKIP("The stand-in interpreters are shell scripts.");

    const QList<Interpreter> before = PythonSettings::interpreters();
    const QString defaultIdBefore = PythonSettings::defaultInterpreterId();
    const QList<Interpreter> modelBefore = interpreterModel().interpreters();
    const QScopeGuard restore([&] {
        PythonSettings::setInterpreter(before, defaultIdBefore);
        interpreterModel().setInterpreters(modelBefore);
    });

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath root = FilePath::fromString(tmp.path());
    const FilePaths searchPaths{root / "first", root / "second"};
    for (const FilePath &dir : searchPaths) {
        QVERIFY(dir.createDir());
        const FilePath python = dir / "python3";
        QVERIFY(python.writeFileContents("#!/bin/sh\necho Python 3.99\n"));
        QVERIFY(python.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    }

    const DetectionSource source(DetectionSource::FromSystem, "python-settings-test-device");
    // A search covers a device's paths, the phases searching elsewhere are another device.
    const DetectionSource other(DetectionSource::FromSystem, "python-settings-test-other");
    const DetectionSource manual(DetectionSource::Manual, "python-settings-test-manual");
    const LogCallback ignore = [](const QString &) {};

    // Resolves to the first interpreter, but must not become the kit's.
    const FilePath venvPython = root / "venv/bin/python";
    QVERIFY(venvPython.parentDir().createDir());
    QVERIFY((root / "venv/pyvenv.cfg").ensureExistingFile());
    QVERIFY(searchPaths.first().pathAppended("python3").createSymLink(venvPython));
    PythonSettings::addInterpreter(
        Interpreter("python-settings-test-venv", "Test venv", venvPython, manual));

    Kit kit;
    QCOMPARE(QTaskTree::runBlocking(
                 Group{PythonSettings::autoDetect(&kit, searchPaths, source, ignore)}),
             DoneWith::Success);

    const QList<Interpreter> detected
        = Utils::filtered(PythonSettings::interpreters(), [&source](const Interpreter &i) {
              return i.detectionSource == source;
          });
    QCOMPARE(detected.size(), 2);

    // The first search path wins, as it would in PATH.
    const std::optional<Interpreter> kitPython = PythonKitAspect::python(&kit);
    QVERIFY(kitPython);
    QCOMPARE(kitPython->command, searchPaths.first() / "python3");

    // The kit detection would take a kit with the device's detection source for its own.
    for (const Interpreter &interpreter : detected)
        QVERIFY(!KitManager::kit(Id::fromString(interpreter.id)));

    // A second run finds both known, and still has to give a new kit its interpreter.
    Kit secondKit;
    QCOMPARE(QTaskTree::runBlocking(
                 Group{PythonSettings::autoDetect(&secondKit, searchPaths, source, ignore)}),
             DoneWith::Success);
    QCOMPARE(PythonKitAspect::python(&secondKit), kitPython);
    QCOMPARE(Utils::count(PythonSettings::interpreters(), [&source](const Interpreter &i) {
                 return i.detectionSource == source;
             }),
             2);

    // The device re-detection removes them first. They keep their ids.
    PythonSettings::removeDetectedPython(source.id, ignore);
    QCOMPARE(QTaskTree::runBlocking(
                 Group{PythonSettings::autoDetect(&kit, searchPaths, source, ignore)}),
             DoneWith::Success);
    QCOMPARE(PythonKitAspect::python(&kit), kitPython);
    QCOMPARE(Utils::filtered(PythonSettings::interpreters(), [&source](const Interpreter &i) {
                 return i.detectionSource == source;
             }),
             detected);

    // An interpreter that is gone from the device goes from the settings.
    const FilePath secondPython = searchPaths.last() / "python3";
    const Interpreter second = Utils::findOrDefault(detected, [&](const Interpreter &i) {
        return i.command == secondPython;
    });
    QCOMPARE(second.command, secondPython);
    QVERIFY(secondPython.removeFile());
    QCOMPARE(QTaskTree::runBlocking(
                 Group{PythonSettings::autoDetect(&kit, searchPaths, source, ignore)}),
             DoneWith::Success);
    QCOMPARE(PythonKitAspect::python(&kit), kitPython);
    QCOMPARE(Utils::filtered(PythonSettings::interpreters(), [&source](const Interpreter &i) {
                 return i.detectionSource == source;
             }),
             QList<Interpreter>{*kitPython});

    // Not finding an interpreter does not make it gone: the device may not answer.
    for (const FilePaths &paths : {FilePaths{}, FilePaths{root / "nothing"}}) {
        PythonSettings::removeDetectedPython(source.id, ignore);
        QCOMPARE(QTaskTree::runBlocking(
                     Group{PythonSettings::autoDetect(nullptr, paths, source, ignore)}),
                 DoneWith::Success);
        QCOMPARE(interpreterModel().interpreterFrom(source.id), QList<Interpreter>{*kitPython});
        QCOMPARE(Utils::filtered(PythonSettings::interpreters(), [&source](const Interpreter &i) {
                     return i.detectionSource == source;
                 }),
                 QList<Interpreter>{*kitPython});
    }
    const DetectionSource down(DetectionSource::FromSystem, "python-settings-test-down");
    const FilePath downRoot = FilePath::fromParts(u"device", down.id, u"/");
    const Interpreter onDown("python-settings-test-on-down", "Test on down",
                             downRoot / "usr/bin/python3", down);
    PythonSettings::addInterpreter(onDown);
    for (const FilePaths &paths : {FilePaths{}, FilePaths{downRoot / "usr/bin"}}) {
        QCOMPARE(QTaskTree::runBlocking(
                     Group{PythonSettings::autoDetect(nullptr, paths, down, ignore)}),
                 DoneWith::Success);
        QVERIFY(Utils::contains(PythonSettings::interpreters(), onDown));
    }

    // The python3 next to a known venv python is that interpreter.
    QVERIFY((searchPaths.first() / "python3").createSymLink(venvPython.parentDir() / "python3"));
    Kit venvPythonKit;
    QCOMPARE(QTaskTree::runBlocking(Group{PythonSettings::autoDetect(
                 &venvPythonKit, {venvPython.parentDir()}, other, ignore)}),
             DoneWith::Success);
    QCOMPARE(PythonKitAspect::python(&venvPythonKit).value_or(Interpreter()).id,
             QString("python-settings-test-venv"));
    QCOMPARE(Utils::count(PythonSettings::interpreters(), [&venvPython](const Interpreter &i) {
                 return i.command.parentDir() == venvPython.parentDir();
             }),
             1);

    // Known to the settings only, as after the startup scan.
    const FilePath thirdPath = root / "third";
    QVERIFY(thirdPath.createDir());
    QVERIFY((searchPaths.first() / "python3").copyFile(thirdPath / "python3"));
    const Interpreter applied("python-settings-test-applied", "Test applied",
                              thirdPath / "python3", manual);
    PythonSettings::addInterpreter(applied);
    Kit thirdKit;
    QCOMPARE(QTaskTree::runBlocking(
                 Group{PythonSettings::autoDetect(&thirdKit, {thirdPath}, other, ignore)}),
             DoneWith::Success);
    QCOMPARE(PythonKitAspect::python(&thirdKit), applied);
    QCOMPARE(Utils::count(PythonSettings::interpreters(), [&applied](const Interpreter &i) {
                 return i.command == applied.command;
             }),
             1);

    // A venv on a search path is its own interpreter, found once, whatever it links to.
    const FilePath unknownBase = root / "base/python3";
    QVERIFY(unknownBase.parentDir().createDir());
    QVERIFY((searchPaths.first() / "python3").copyFile(unknownBase));
    for (const FilePath &base : {unknownBase, searchPaths.first() / "python3"}) {
        const FilePath venvRoot = root / ("venv-" + base.parentDir().fileName());
        const FilePath venvBin = venvRoot / "bin";
        QVERIFY(venvBin.createDir());
        QVERIFY((venvRoot / "pyvenv.cfg").ensureExistingFile());
        QVERIFY(base.createSymLink(venvBin / "python3"));
        for (int run = 0; run < 2; ++run) {
            Kit venvKit;
            QCOMPARE(QTaskTree::runBlocking(
                         Group{PythonSettings::autoDetect(&venvKit, {venvBin}, other, ignore)}),
                     DoneWith::Success);
            QCOMPARE(Utils::count(PythonSettings::interpreters(), [&venvBin](const Interpreter &i) {
                         return i.command == venvBin / "python3";
                     }),
                     1);
            const std::optional<Interpreter> venvKitPython = PythonKitAspect::python(&venvKit);
            QVERIFY(venvKitPython);
            QCOMPARE(venvKitPython->command, venvBin / "python3");
        }
    }

    // A venv and its base on one search path are two interpreters, in either order.
    for (const bool venvFirst : {true, false}) {
        const QString name = venvFirst ? QString("pair-venv-first") : QString("pair-base-first");
        const FilePath pairBase = root / name / "base/python3";
        const FilePath pairVenvBin = root / name / "venv/bin";
        QVERIFY(pairBase.parentDir().createDir());
        QVERIFY((searchPaths.first() / "python3").copyFile(pairBase));
        QVERIFY(pairVenvBin.createDir());
        QVERIFY((root / name / "venv/pyvenv.cfg").ensureExistingFile());
        QVERIFY(pairBase.createSymLink(pairVenvBin / "python3"));
        const FilePaths paths = venvFirst ? FilePaths{pairVenvBin, pairBase.parentDir()}
                                          : FilePaths{pairBase.parentDir(), pairVenvBin};
        Kit pairKit;
        QCOMPARE(QTaskTree::runBlocking(
                     Group{PythonSettings::autoDetect(&pairKit, paths, other, ignore)}),
                 DoneWith::Success);
        for (const FilePath &dir : paths) {
            QCOMPARE(Utils::count(PythonSettings::interpreters(), [&dir](const Interpreter &i) {
                         return i.command == dir / "python3";
                     }),
                     1);
        }
    }

    // Two searches that start from the same settings find the same command once.
    const DetectionSource twice(DetectionSource::FromSystem, "python-settings-test-twice");
    const FilePath twiceBin = root / "twice";
    QVERIFY(twiceBin.createDir());
    QVERIFY((twiceBin / "python3").writeFileContents("#!/bin/sh\necho Python 3.99\n"));
    QVERIFY((twiceBin / "python3")
                .setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    Kit twiceKit;
    QCOMPARE(QTaskTree::runBlocking(
                 Group{parallel,
                       PythonSettings::autoDetect(nullptr, {twiceBin}, twice, ignore),
                       PythonSettings::autoDetect(&twiceKit, {twiceBin}, twice, ignore)}),
             DoneWith::Success);
    QCOMPARE(Utils::count(PythonSettings::interpreters(), [&twice](const Interpreter &i) {
                 return i.detectionSource == twice;
             }),
             1);
    QCOMPARE(PythonKitAspect::python(&twiceKit)->detectionSource, twice);

    // The desktop has no detection source id: its interpreters get kits of their own.
    const DetectionSource desktop(DetectionSource::FromSystem, {});
    const FilePath desktopBin = root / "desktop";
    QVERIFY(desktopBin.createDir());
    QVERIFY((desktopBin / "python3").writeFileContents("#!/bin/sh\necho Python 3.99\n"));
    QVERIFY((desktopBin / "python3")
                .setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    for (int run = 0; run < 2; ++run) {
        QCOMPARE(QTaskTree::runBlocking(
                     Group{PythonSettings::autoDetect(nullptr, {desktopBin}, desktop, ignore)}),
                 DoneWith::Success);
        const QList<Interpreter> found
            = Utils::filtered(PythonSettings::interpreters(), [&](const Interpreter &i) {
                  return i.command == desktopBin / "python3";
              });
        QCOMPARE(found.size(), 1);
        QVERIFY(KitManager::kit(Id::fromString(found.first().id)));
        QCOMPARE(Utils::count(KitManager::kits(), [&](const Kit *k) {
                     const std::optional<Interpreter> python = PythonKitAspect::python(k);
                     return python && python->command == desktopBin / "python3";
                 }),
                 1);
    }

    // A kit generated for the interpreter, as by an earlier version, can be the one the kit
    // detection fills. The interpreter being gone must not take that kit away.
    Kit *generatedKit = KitManager::registerKit({}, Id::fromString(kitPython->id));
    QVERIFY(generatedKit);
    QVERIFY(kitPython->command.removeFile());
    QCOMPARE(QTaskTree::runBlocking(
                 Group{PythonSettings::autoDetect(generatedKit, searchPaths, source, ignore)}),
             DoneWith::Success);
    QCOMPARE(KitManager::kit(Id::fromString(kitPython->id)), generatedKit);

    // The same holds when the detection runs without a kit, as the device page's own does.
    generatedKit->setDetectionSource(source);
    QCOMPARE(QTaskTree::runBlocking(
                 Group{PythonSettings::autoDetect(nullptr, searchPaths, source, ignore)}),
             DoneWith::Success);
    QCOMPARE(KitManager::kit(Id::fromString(kitPython->id)), generatedKit);

    PythonSettings::setInterpreter(
        Utils::filtered(PythonSettings::interpreters(),
                        [&](const Interpreter &i) {
                            return i.detectionSource != manual && i.detectionSource != other
                                   && i.detectionSource != down;
                        }),
        PythonSettings::defaultInterpreterId());

    // Removing the device takes its interpreters along.
    const Id deviceId = Id::fromString(source.id);
    DeviceManager::addDevice(std::make_shared<PythonTestDevice>(deviceId));
    DeviceManager::removeDevice(deviceId);
    QVERIFY(!Utils::anyOf(PythonSettings::interpreters(), [&source](const Interpreter &i) {
        return i.detectionSource == source;
    }));
    QVERIFY(interpreterModel().interpreterFrom(source.id).isEmpty());
    QVERIFY(!KitManager::kit(Id::fromString(kitPython->id)));
}

QObject *createPythonSettingsTest()
{
    return new PythonSettingsTest;
}

} // namespace Python::Internal

#endif // WITH_TESTS
