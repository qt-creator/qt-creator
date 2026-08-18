// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosdevice_test.h"

#include "harmonyosdevice.h"
#include "harmonyossdk.h"
#include "harmonyossettings.h"

#include <projectexplorer/devicesupport/devicemanager.h>

#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/qtcprocess.h>

#include <QTest>

using namespace ProjectExplorer;
using namespace Utils;

namespace HarmonyOs::Internal {

// Exercises the hdc-backed file access and process interface against whatever
// device is attached, and skips when there is none.
class HarmonyOsDeviceTest final : public QObject
{
    Q_OBJECT

private:
    IDevice::Ptr attachedDevice()
    {
        const FilePath hdc = Sdk::hdcCommand(settings().sdkLocation());
        if (hdc.isEmpty())
            return {};

        Process process;
        process.setCommand({hdc, {"list", "targets"}});
        process.runBlocking(std::chrono::seconds(10));
        if (process.result() != ProcessResult::FinishedWithSuccess)
            return {};

        const QStringList lines = process.cleanedStdOut().split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            const QString serial = line.trimmed();
            if (serial.isEmpty() || serial.contains(' '))
                continue;
            const IDevice::Ptr device = HarmonyOsDevice::create();
            const auto harmonyDevice = static_cast<HarmonyOsDevice *>(device.get());
            harmonyDevice->setupId(IDevice::ManuallyAdded, Id::fromString(QString("HarmonyOS.Test")));
            harmonyDevice->setDeviceState(IDevice::DeviceReadyToUse);
            harmonyDevice->setSerialNumber(serial);
            // Only a registered device resolves its own "device://" paths.
            DeviceManager::addDevice(device);
            return device;
        }
        return {};
    }

private slots:
    void cleanup()
    {
        if (const IDevice::Ptr device = DeviceManager::find(Id::fromString(QString("HarmonyOS.Test"))))
            DeviceManager::removeDevice(device->id());
    }

    void testFileAccess()
    {
        const IDevice::Ptr device = attachedDevice();
        if (!device)
            QSKIP("No HarmonyOS device is attached");
        QVERIFY(device->fileAccess());

        const FilePath shell = device->filePath("/bin/sh");
        QVERIFY(shell.exists());
        QVERIFY(shell.isExecutableFile());
        QVERIFY(!device->filePath("/bin/no-such-tool").exists());

        const FilePath binDir = device->filePath("/bin");
        QVERIFY(binDir.isDir());
        QVERIFY(binDir.dirEntries(FileFilter({}, DirFilterFlag::Files)).size() > 100);

        const FilePath file = device->filePath("/data/local/tmp/qtc-file-access-test");
        file.removeFile();
        const QByteArray contents = "written through hdc shell\n";
        QVERIFY(file.writeFileContents(contents));
        QCOMPARE(file.fileContents().value_or(QByteArray()), contents);
        QCOMPARE(file.fileSize(), contents.size());
        QVERIFY(file.removeFile());
        QVERIFY(!file.exists());
    }

    void testRunProcess()
    {
        const IDevice::Ptr device = attachedDevice();
        if (!device)
            QSKIP("No HarmonyOS device is attached");

        Process process;
        process.setCommand({device->filePath("/bin/echo"), {"ran-on-device"}});
        process.runBlocking(std::chrono::seconds(15));
        QCOMPARE(process.result(), ProcessResult::FinishedWithSuccess);
        QCOMPARE(process.cleanedStdOut().trimmed(), QString("ran-on-device"));
    }

    // Reports what a build on the device would have to work with.
    void testProbeBuildEnvironment()
    {
        const IDevice::Ptr device = attachedDevice();
        if (!device)
            QSKIP("No HarmonyOS device is attached");

        const Result<Environment> env = device->systemEnvironmentWithError();
        QVERIFY2(env.has_value(), qPrintable(env ? QString() : env.error()));
        qDebug().noquote() << "PATH on the device:" << env->value("PATH");

        for (const QString &tool : {QString("sh"), QString("tar"), QString("cmake"),
                                    QString("ninja"), QString("make"), QString("cc"),
                                    QString("clang"), QString("gcc"), QString("g++"),
                                    QString("python3"), QString("node"), QString("ld"),
                                    QString("ar"), QString("pkg-config")}) {
            const FilePath found = device->searchExecutableInPath(tool);
            qDebug().noquote() << QString("  %1: %2").arg(tool, 12)
                                      .arg(found.isEmpty() ? QString("-") : found.path());
        }

        // Whatever else is missing, the shell the file access relies on must be there.
        QVERIFY(!device->searchExecutableInPath("sh").isEmpty());
    }

    // Runs the same detection the Devices page offers.
    void testAutoDetect()
    {
        const IDevice::Ptr device = attachedDevice();
        if (!device)
            QSKIP("No HarmonyOS device is attached");

        QStringList log;
        const ToolDetectionLogger logger([&log](const QString &message) {
            log.append(message);
            qDebug().noquote() << "detection:" << message;
        });

        bool done = false;
        device->runAutoDetect(logger, [&done] { done = true; });
        QTRY_VERIFY_WITH_TIMEOUT(done, 240 * 1000);
        QVERIFY(!log.isEmpty());
    }

    void testFailingProcess()
    {
        const IDevice::Ptr device = attachedDevice();
        if (!device)
            QSKIP("No HarmonyOS device is attached");

        Process process;
        process.setCommand({device->filePath("/bin/sh"), {"-c", "exit 3"}});
        process.runBlocking(std::chrono::seconds(15));
        QCOMPARE(process.exitCode(), 3);
        QCOMPARE(process.result(), ProcessResult::FinishedWithError);
    }
};

QObject *createHarmonyOsDeviceTest()
{
    return new HarmonyOsDeviceTest;
}

} // namespace HarmonyOs::Internal

#include "harmonyosdevice_test.moc"
