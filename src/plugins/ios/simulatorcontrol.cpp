/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "simulatorcontrol.h"
#include "iossimulator.h"
#include "iosconfigurations.h"

#include <utils/runextensions.h>

#ifdef Q_OS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <chrono>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMap>
#include <QProcess>
#include <QReadLocker>
#include <QReadWriteLock>
#include <QTime>
#include <QUrl>
#include <QWriteLocker>

namespace {
Q_LOGGING_CATEGORY(simulatorLog, "qtc.ios.simulator")
}

namespace Ios {
namespace Internal {

static int COMMAND_TIMEOUT = 10000;
static int SIMULATOR_TIMEOUT = 60000;

static bool checkForTimeout(const std::chrono::time_point< std::chrono::high_resolution_clock, std::chrono::nanoseconds> &start, int msecs = COMMAND_TIMEOUT)
{
    bool timedOut = false;
    auto end = std::chrono::high_resolution_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() > msecs)
        timedOut = true;
    return timedOut;
}

static QByteArray runSimCtlCommand(QStringList args)
{
    QProcess simCtlProcess;
    args.prepend(QStringLiteral("simctl"));
    simCtlProcess.start(QStringLiteral("xcrun"), args, QProcess::ReadOnly);
    if (!simCtlProcess.waitForFinished())
        qCDebug(simulatorLog) << "simctl command failed." << simCtlProcess.errorString();
    return simCtlProcess.readAll();
}

class SimulatorControlPrivate :QObject {
    Q_OBJECT
private:
    struct SimDeviceInfo {
        bool isBooted() const { return state.compare(QStringLiteral("Booted")) == 0; }
        bool isAvailable() const { return !availability.contains(QStringLiteral("unavailable")); }
        QString name;
        QString udid;
        QString availability;
        QString state;
        QString sdk;
    };

    SimulatorControlPrivate(QObject *parent = nullptr);
    ~SimulatorControlPrivate();
    SimDeviceInfo deviceInfo(const QString &simUdid) const;
    bool runCommand(QString command, const QStringList &args, QByteArray *output = nullptr);

    QHash<QString, QProcess*> simulatorProcesses;
    QReadWriteLock processDataLock;
    QList<IosDeviceType> availableDevices;
    friend class SimulatorControl;
};

SimulatorControlPrivate *SimulatorControl::d = new SimulatorControlPrivate;

SimulatorControl::SimulatorControl()
{

}

QList<Ios::Internal::IosDeviceType> SimulatorControl::availableSimulators()
{
    return d->availableDevices;
}

static QList<IosDeviceType> getAvailableSimulators()
{
    QList<IosDeviceType> availableDevices;
    const QByteArray output = runSimCtlCommand({QLatin1String("list"), QLatin1String("-j"), QLatin1String("devices")});
    QJsonDocument doc = QJsonDocument::fromJson(output);
    if (!doc.isNull()) {
        const QJsonObject buildInfo = doc.object().value("devices").toObject();
        foreach (const QString &buildVersion, buildInfo.keys()) {
            QJsonArray devices = buildInfo.value(buildVersion).toArray();
            foreach (const QJsonValue device, devices) {
                QJsonObject deviceInfo = device.toObject();
                QString deviceName = QString("%1, %2")
                        .arg(deviceInfo.value("name").toString("Unknown"))
                        .arg(buildVersion);
                QString deviceUdid = deviceInfo.value("udid").toString("Unknown");
                if (!deviceInfo.value("availability").toString().contains("unavailable")) {
                    IosDeviceType iOSDevice(IosDeviceType::SimulatedDevice, deviceUdid, deviceName);
                    availableDevices.append(iOSDevice);
                }
            }
        }
        std::stable_sort(availableDevices.begin(), availableDevices.end());
    } else {
        qCDebug(simulatorLog) << "Error parsing json output from simctl. Output:" << output;
    }
    return availableDevices;
}

void SimulatorControl::updateAvailableSimulators()
{
    QFuture<QList<IosDeviceType>> future = Utils::runAsync(getAvailableSimulators);
    Utils::onResultReady(future, d, [](const QList<IosDeviceType> &devices) {
        SimulatorControl::d->availableDevices = devices;
    });
}

// Blocks until simulators reaches "Booted" state.
bool SimulatorControl::startSimulator(const QString &simUdid)
{
    QWriteLocker locker(&d->processDataLock);
    bool simulatorRunning = isSimulatorRunning(simUdid);
    if (!simulatorRunning && d->deviceInfo(simUdid).isAvailable()) {
        // Simulator is not running but it's available. Start the simulator.
        QProcess *p = new QProcess;
        QObject::connect(p, static_cast<void(QProcess::*)(int)>(&QProcess::finished), [simUdid]() {
            QWriteLocker locker(&d->processDataLock);
            d->simulatorProcesses[simUdid]->deleteLater();
            d->simulatorProcesses.remove(simUdid);
        });

        const QString cmd = IosConfigurations::developerPath().appendPath(QStringLiteral("/Applications/Simulator.app")).toString();
        const QStringList args({QStringLiteral("--args"), QStringLiteral("-CurrentDeviceUDID"), simUdid});
        p->start(cmd, args);

        if (p->waitForStarted()) {
            d->simulatorProcesses[simUdid] = p;
            // At this point the sim device exists, available and was not running.
            // So the simulator is started and we'll wait for it to reach to a state
            // where we can interact with it.
            auto start = std::chrono::high_resolution_clock::now();
            SimulatorControlPrivate::SimDeviceInfo info;
            do {
                info = d->deviceInfo(simUdid);
            } while (!info.isBooted()
                     && p->state() == QProcess::Running
                     && !checkForTimeout(start, SIMULATOR_TIMEOUT));
            simulatorRunning = info.isBooted();
        } else {
            qCDebug(simulatorLog) << "Error starting simulator." << p->errorString();
            delete p;
        }
    }
    return simulatorRunning;
}

bool SimulatorControl::isSimulatorRunning(const QString &simUdid)
{
    if (simUdid.isEmpty())
        return false;
    return d->deviceInfo(simUdid).isBooted();
}

bool SimulatorControl::installApp(const QString &simUdid, const Utils::FileName &bundlePath, QByteArray &commandOutput)
{
    bool installed = false;
    if (isSimulatorRunning(simUdid)) {
        commandOutput = runSimCtlCommand(QStringList() << QStringLiteral("install") << simUdid << bundlePath.toString());
        installed = commandOutput.isEmpty();
    } else {
        commandOutput = "Simulator device not running.";
    }
    return installed;
}

qint64 SimulatorControl::launchApp(const QString &simUdid, const QString &bundleIdentifier, QByteArray* commandOutput)
{
    qint64 pId = -1;
    pId = -1;
    if (!bundleIdentifier.isEmpty() && isSimulatorRunning(simUdid)) {
        const QStringList args({QStringLiteral("launch"), simUdid , bundleIdentifier});
        const QByteArray output = runSimCtlCommand(args);
        const QByteArray pIdStr = output.trimmed().split(' ').last().trimmed();
        bool validInt = false;
        pId = pIdStr.toLongLong(&validInt);
        if (!validInt) {
            // Launch Failed.
            qCDebug(simulatorLog) << "Launch app failed. Process id returned is not valid. PID =" << pIdStr;
            pId = -1;
            if (commandOutput)
                *commandOutput = output;
        }
    }
    return pId;
}

QString SimulatorControl::bundleIdentifier(const Utils::FileName &bundlePath)
{
    QString bundleID;
#ifdef Q_OS_MAC
    if (bundlePath.exists()) {
        CFStringRef cFBundlePath = bundlePath.toString().toCFString();
        CFURLRef bundle_url = CFURLCreateWithFileSystemPath (kCFAllocatorDefault, cFBundlePath, kCFURLPOSIXPathStyle, true);
        CFRelease(cFBundlePath);
        CFBundleRef bundle = CFBundleCreate (kCFAllocatorDefault, bundle_url);
        CFRelease(bundle_url);
        CFStringRef cFBundleID = CFBundleGetIdentifier(bundle);
        bundleID = QString::fromCFString(cFBundleID).trimmed();
        CFRelease(bundle);
    }
#else
    Q_UNUSED(bundlePath)
#endif
    return bundleID;
}

QString SimulatorControl::bundleExecutable(const Utils::FileName &bundlePath)
{
    QString executable;
#ifdef Q_OS_MAC
    if (bundlePath.exists()) {
        CFStringRef cFBundlePath = bundlePath.toString().toCFString();
        CFURLRef bundle_url = CFURLCreateWithFileSystemPath (kCFAllocatorDefault, cFBundlePath, kCFURLPOSIXPathStyle, true);
        CFRelease(cFBundlePath);
        CFBundleRef bundle = CFBundleCreate (kCFAllocatorDefault, bundle_url);
        CFStringRef cFStrExecutableName = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(bundle, kCFBundleExecutableKey);
        executable = QString::fromCFString(cFStrExecutableName).trimmed();
        CFRelease(bundle);
    }
#else
    Q_UNUSED(bundlePath)
#endif
    return executable;
}

SimulatorControlPrivate::SimulatorControlPrivate(QObject *parent):
    QObject(parent),
    processDataLock(QReadWriteLock::Recursive)
{
}

SimulatorControlPrivate::~SimulatorControlPrivate()
{

}

// The simctl spawns the process and returns the pId but the application process might not have started, at least in a state where you can interrupt it.
// Use SimulatorControl::waitForProcessSpawn to be sure.
QProcess *SimulatorControl::spawnAppProcess(const QString &simUdid, const Utils::FileName &bundlePath, qint64 &pId, bool waitForDebugger, const QStringList &extraArgs)
{
    QProcess *simCtlProcess = nullptr;
    if (isSimulatorRunning(simUdid)) {
        QString bundleId = bundleIdentifier(bundlePath);
        QString executableName = bundleExecutable(bundlePath);
        QByteArray appPath = runSimCtlCommand(QStringList() << QStringLiteral("get_app_container") << simUdid << bundleId).trimmed();
        if (!appPath.isEmpty() && !executableName.isEmpty()) {
            // Spawn the app. The spawned app is started in suspended mode.
            appPath.append('/' + executableName.toLocal8Bit());
            simCtlProcess = new QProcess;
            QStringList args;
            args << QStringLiteral("simctl");
            args << QStringLiteral("spawn");
            if (waitForDebugger)
                args << QStringLiteral("-w");
            args << simUdid;
            args << QString::fromLocal8Bit(appPath);
            args << extraArgs;
            simCtlProcess->start(QStringLiteral("xcrun"), args);
            if (!simCtlProcess->waitForStarted()){
                // Spawn command failed.
                qCDebug(simulatorLog) << "Spawning the app failed." << simCtlProcess->errorString();
                delete simCtlProcess;
                simCtlProcess = nullptr;
            }

            // Find the process id of the the app process.
            if (simCtlProcess) {
                qint64 simctlPId = simCtlProcess->processId();
                pId = -1;
                QByteArray commandOutput;
                QStringList pGrepArgs;
                pGrepArgs << QStringLiteral("-f") << QString::fromLocal8Bit(appPath);
                auto begin = std::chrono::high_resolution_clock::now();
                // Find the pid of the spawned app.
                while (pId == -1 && d->runCommand(QStringLiteral("pgrep"), pGrepArgs, &commandOutput)) {
                    foreach (auto pidStr, commandOutput.trimmed().split('\n')) {
                        qint64 parsedPId = pidStr.toLongLong();
                        if (parsedPId != simctlPId)
                            pId = parsedPId;
                    }
                    if (checkForTimeout(begin)) {
                        qCDebug(simulatorLog) << "Spawning the app failed. Process timed out";
                        break;
                    }
                }
            }

            if (pId == -1) {
                // App process id can't be found.
                qCDebug(simulatorLog) << "Spawning the app failed. PID not found.";
                delete simCtlProcess;
                simCtlProcess = nullptr;
            }
        } else {
            qCDebug(simulatorLog) << "Spawning the app failed. Check installed app." << appPath;
        }
    } else {
        qCDebug(simulatorLog) << "Spawning the app failed. Simulator not running." << simUdid;
    }
    return simCtlProcess;
}

bool SimulatorControl::waitForProcessSpawn(qint64 processPId)
{
    bool success = true;
    if (processPId != -1) {
        // Wait for app to reach intruptible sleep state.
        QByteArray wqStr;
        QStringList args;
        int wqCount = -1;
        args << QStringLiteral("-p") << QString::number(processPId) << QStringLiteral("-o") << QStringLiteral("wq=");
        auto begin = std::chrono::high_resolution_clock::now();
        do {
            if (!d->runCommand(QStringLiteral("ps"), args, &wqStr)) {
                success = false;
                break;
            }
            bool validInt = false;
            wqCount = wqStr.toInt(&validInt);
            if (!validInt) {
                wqCount = -1;
            }
        } while (wqCount < 0 && !checkForTimeout(begin));
        success = wqCount >= 0;
    } else {
        qCDebug(simulatorLog) << "Wait for spawned failed. Invalid Process ID." << processPId;
    }
    return success;
}

SimulatorControlPrivate::SimDeviceInfo SimulatorControlPrivate::deviceInfo(const QString &simUdid) const
{
    SimDeviceInfo info;
    bool found = false;
    if (!simUdid.isEmpty()) {
        // It might happend that the simulator is not started by SimControl.
        // Check of intances started externally.
        const QByteArray output = runSimCtlCommand({QLatin1String("list"), QLatin1String("-j"), QLatin1String("devices")});
        QJsonDocument doc = QJsonDocument::fromJson(output);
        if (!doc.isNull()) {
            const QJsonObject buildInfo = doc.object().value(QStringLiteral("devices")).toObject();
            foreach (const QString &buildVersion, buildInfo.keys()) {
                QJsonArray devices = buildInfo.value(buildVersion).toArray();
                foreach (const QJsonValue device, devices) {
                    QJsonObject deviceInfo = device.toObject();
                    QString deviceUdid = deviceInfo.value(QStringLiteral("udid")).toString();
                    if (deviceUdid.compare(simUdid) == 0) {
                        found = true;
                        info.name = deviceInfo.value(QStringLiteral("name")).toString();
                        info.udid = deviceUdid;
                        info.state = deviceInfo.value(QStringLiteral("state")).toString();
                        info.sdk = buildVersion;
                        info.availability = deviceInfo.value(QStringLiteral("availability")).toString();
                        break;
                    }
                }
                if (found)
                    break;
            }
        } else {
            qCDebug(simulatorLog) << "Cannot find device info. Error parsing json output from simctl. Output:" << output;
        }
    } else {
        qCDebug(simulatorLog) << "Cannot find device info. Invalid UDID.";
    }
    return info;
}

bool SimulatorControlPrivate::runCommand(QString command, const QStringList &args, QByteArray *output)
{
    bool success = false;
    QProcess process;
    process.start(command, args);
    success = process.waitForFinished();
    if (output)
        *output = process.readAll().trimmed();
    return success;
}

} // namespace Internal
} // namespace Ios
#include "simulatorcontrol.moc"
