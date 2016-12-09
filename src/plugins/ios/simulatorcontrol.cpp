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

#include "utils/runextensions.h"
#include "utils/qtcassert.h"
#include "utils/synchronousprocess.h"

#ifdef Q_OS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <chrono>
#include <memory>

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

using namespace std;

namespace {
Q_LOGGING_CATEGORY(simulatorLog, "qtc.ios.simulator")
}

namespace Ios {
namespace Internal {

static int COMMAND_TIMEOUT = 10000;
static int SIMULATOR_START_TIMEOUT = 60000;
static QString SIM_UDID_TAG = QStringLiteral("SimUdid");

static bool checkForTimeout(const chrono::time_point< chrono::high_resolution_clock, chrono::nanoseconds> &start, int msecs = COMMAND_TIMEOUT)
{
    bool timedOut = false;
    auto end = chrono::high_resolution_clock::now();
    if (chrono::duration_cast<chrono::milliseconds>(end-start).count() > msecs)
        timedOut = true;
    return timedOut;
}

static bool runCommand(QString command, const QStringList &args, QByteArray *output)
{
    Utils::SynchronousProcess p;
    Utils::SynchronousProcessResponse resp = p.runBlocking(command, args);
    if (output)
        *output = resp.allRawOutput();
    return resp.result == Utils::SynchronousProcessResponse::Finished;
}

static QByteArray runSimCtlCommand(QStringList args)
{
    QByteArray output;
    args.prepend(QStringLiteral("simctl"));
    runCommand(QStringLiteral("xcrun"), args, &output);
    return output;
}

static bool waitForProcessSpawn(qint64 processPId, QFutureInterface<SimulatorControl::ResponseData> &fi)
{
    bool success = false;
    if (processPId != -1) {
        // Wait for app to reach intruptible sleep state.
        const QStringList args = {QStringLiteral("-p"), QString::number(processPId),
                                  QStringLiteral("-o"), QStringLiteral("wq=")};
        int wqCount = -1;
        QByteArray wqStr;
        auto begin = chrono::high_resolution_clock::now();
        do {
            if (fi.isCanceled() || !runCommand(QStringLiteral("ps"), args, &wqStr))
                break;
            bool validInt = false;
            wqCount = wqStr.trimmed().toInt(&validInt);
            if (!validInt)
                wqCount = -1;
        } while (wqCount < 0 && !checkForTimeout(begin));
        success = wqCount >= 0;
    } else {
        qCDebug(simulatorLog) << "Wait for spawned failed. Invalid Process ID." << processPId;
    }
    return success;
}

class SimulatorControlPrivate {
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

    SimulatorControlPrivate();
    ~SimulatorControlPrivate();

    static SimDeviceInfo deviceInfo(const QString &simUdid);
    static QString bundleIdentifier(const Utils::FileName &bundlePath);
    static QString bundleExecutable(const Utils::FileName &bundlePath);

    void startSimulator(QFutureInterface<SimulatorControl::ResponseData> &fi, const QString &simUdid);
    void installApp(QFutureInterface<SimulatorControl::ResponseData> &fi, const QString &simUdid,
                    const Utils::FileName &bundlePath);
    void spawnAppProcess(QFutureInterface<SimulatorControl::ResponseData> &fi, const QString &simUdid,
                         const Utils::FileName &bundlePath, bool waitForDebugger, QStringList extraArgs,
                         QThread *mainThread);
    void launchApp(QFutureInterface<SimulatorControl::ResponseData> &fi, const QString &simUdid,
                   const QString &bundleIdentifier, qint64 spawnedPID);

    static QList<IosDeviceType> availableDevices;
    friend class SimulatorControl;
};

SimulatorControl::SimulatorControl(QObject *parent) :
    QObject(parent),
    d(new SimulatorControlPrivate)
{
}

SimulatorControl::~SimulatorControl()
{
    delete d;
}

QList<Ios::Internal::IosDeviceType> SimulatorControl::availableSimulators()
{
    return SimulatorControlPrivate::availableDevices;
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
        stable_sort(availableDevices.begin(), availableDevices.end());
    } else {
        qCDebug(simulatorLog) << "Error parsing json output from simctl. Output:" << output;
    }
    return availableDevices;
}

void SimulatorControl::updateAvailableSimulators()
{
    QFuture< QList<IosDeviceType> > future = Utils::runAsync(getAvailableSimulators);
    Utils::onResultReady(future, [](const QList<IosDeviceType> &devices) {
        SimulatorControlPrivate::availableDevices = devices;
    });
}

bool SimulatorControl::isSimulatorRunning(const QString &simUdid)
{
    if (simUdid.isEmpty())
        return false;
    return SimulatorControlPrivate::deviceInfo(simUdid).isBooted();
}

QString SimulatorControl::bundleIdentifier(const Utils::FileName &bundlePath)
{
    return SimulatorControlPrivate::bundleIdentifier(bundlePath);
}

QString SimulatorControl::bundleExecutable(const Utils::FileName &bundlePath)
{
    return SimulatorControlPrivate::bundleExecutable(bundlePath);
}

QFuture<SimulatorControl::ResponseData> SimulatorControl::startSimulator(const QString &simUdid)
{
    return Utils::runAsync(&SimulatorControlPrivate::startSimulator, d, simUdid);
}

QFuture<SimulatorControl::ResponseData>
SimulatorControl::installApp(const QString &simUdid, const Utils::FileName &bundlePath) const
{
    return Utils::runAsync(&SimulatorControlPrivate::installApp, d, simUdid, bundlePath);
}

QFuture<SimulatorControl::ResponseData>
SimulatorControl::spawnAppProcess(const QString &simUdid, const Utils::FileName &bundlePath,
                                  bool waitForDebugger, const QStringList &extraArgs) const
{
    return Utils::runAsync(&SimulatorControlPrivate::spawnAppProcess, d, simUdid, bundlePath,
                           waitForDebugger, extraArgs, QThread::currentThread());
}

QFuture<SimulatorControl::ResponseData>
SimulatorControl::launchApp(const QString &simUdid, const QString &bundleIdentifier,
                            qint64 spawnedPID) const
{
    return Utils::runAsync(&SimulatorControlPrivate::launchApp, d, simUdid,
                                    bundleIdentifier, spawnedPID);
}

QList<IosDeviceType> SimulatorControlPrivate::availableDevices;

SimulatorControlPrivate::SimulatorControlPrivate()
{
}

SimulatorControlPrivate::~SimulatorControlPrivate()
{

}

SimulatorControlPrivate::SimDeviceInfo SimulatorControlPrivate::deviceInfo(const QString &simUdid)
{
    SimDeviceInfo info;
    bool found = false;
    if (!simUdid.isEmpty()) {
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

QString SimulatorControlPrivate::bundleIdentifier(const Utils::FileName &bundlePath)
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

QString SimulatorControlPrivate::bundleExecutable(const Utils::FileName &bundlePath)
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

void SimulatorControlPrivate::startSimulator(QFutureInterface<SimulatorControl::ResponseData> &fi,
                                             const QString &simUdid)
{
    SimulatorControl::ResponseData response(simUdid);
    if (deviceInfo(simUdid).isAvailable()) {
        // Simulator is available.
        const QString cmd = IosConfigurations::developerPath()
                .appendPath(QStringLiteral("/Applications/Simulator.app/Contents/MacOS/Simulator"))
                .toString();
        const QStringList args({QStringLiteral("--args"), QStringLiteral("-CurrentDeviceUDID"), simUdid});

        if (QProcess::startDetached(cmd, args)) {
            if (fi.isCanceled())
                return;
            // At this point the sim device exists, available and was not running.
            // So the simulator is started and we'll wait for it to reach to a state
            // where we can interact with it.
            auto start = chrono::high_resolution_clock::now();
            SimulatorControlPrivate::SimDeviceInfo info;
            do {
                info = deviceInfo(simUdid);
                if (fi.isCanceled())
                    return;
            } while (!info.isBooted()
                     && !checkForTimeout(start, SIMULATOR_START_TIMEOUT));
            if (info.isBooted()) {
                response.success = true;
            }
        } else {
            qCDebug(simulatorLog) << "Error starting simulator.";
        }
    }

    if (!fi.isCanceled()) {
        QThread::msleep(500); // give it some time. TODO: find an actual fix.
        fi.reportResult(response);
    }
}

void SimulatorControlPrivate::installApp(QFutureInterface<SimulatorControl::ResponseData> &fi,
                                         const QString &simUdid, const Utils::FileName &bundlePath)
{
    QTC_CHECK(bundlePath.exists());
    QByteArray output = runSimCtlCommand({QStringLiteral("install"), simUdid, bundlePath.toString()});
    SimulatorControl::ResponseData response(simUdid);
    response.success = output.isEmpty();
    response.commandOutput = output;

    if (!fi.isCanceled()) {
        QThread::msleep(500); // give it some time. TODO: find an actual fix.
        fi.reportResult(response);
    }
}

void SimulatorControlPrivate::spawnAppProcess(QFutureInterface<SimulatorControl::ResponseData> &fi,
                                              const QString &simUdid, const Utils::FileName &bundlePath,
                                              bool waitForDebugger, QStringList extraArgs, QThread *mainThread)
{
    SimulatorControl::ResponseData response(simUdid);

    // Find the path of the installed app.
    QString bundleId = bundleIdentifier(bundlePath);
    QByteArray appContainer = runSimCtlCommand({QStringLiteral("get_app_container"), simUdid, bundleId});
    QString appPath = QString::fromLocal8Bit(appContainer.trimmed());

    if (fi.isCanceled())
        return;

    QString executableName = bundleExecutable(bundlePath);
    if (!appPath.isEmpty() && !executableName.isEmpty()) {
        appPath.append('/' + executableName);
        QStringList args = {QStringLiteral("simctl"), QStringLiteral("spawn"), simUdid, appPath};
        if (waitForDebugger)
            args.insert(2, QStringLiteral("-w"));
        args << extraArgs;

        // Spawn the app. The spawned app is started in suspended mode.
        shared_ptr<QProcess> simCtlProcess(new QProcess, [](QProcess *p) {
            if (p->state() != QProcess::NotRunning) {
                p->kill();
                p->waitForFinished(COMMAND_TIMEOUT);
            }
            delete p;
        });
        simCtlProcess->start(QStringLiteral("xcrun"), args);
        if (simCtlProcess->waitForStarted()) {
            if (fi.isCanceled())
                return;
            // Find the process id of the spawned app.
            qint64 simctlPId = simCtlProcess->processId();
            QByteArray commandOutput;
            const QStringList pGrepArgs = {QStringLiteral("-f"), appPath};
            auto begin = chrono::high_resolution_clock::now();
            int processID = -1;
            while (processID == -1 && runCommand(QStringLiteral("pgrep"), pGrepArgs, &commandOutput)) {
                if (fi.isCanceled()) {
                    qCDebug(simulatorLog) <<"Spawning the app failed. Future cancelled.";
                    return;
                }
                foreach (auto pidStr, commandOutput.trimmed().split('\n')) {
                    qint64 parsedPId = pidStr.toLongLong();
                    if (parsedPId != simctlPId)
                        processID = parsedPId;
                }
                if (checkForTimeout(begin)) {
                    qCDebug(simulatorLog) << "Spawning the app failed. Process timed out";
                    break;
                }
            }

            if (processID == -1) {
                qCDebug(simulatorLog) << "Spawning the app failed. App PID not found.";
                simCtlProcess->waitForReadyRead(COMMAND_TIMEOUT);
                response.commandOutput = simCtlProcess->readAllStandardError();
            } else {
                response.processInstance = simCtlProcess;
                response.processInstance->moveToThread(mainThread);
                response.pID = processID;
                response.success = true;
            }
        } else {
            qCDebug(simulatorLog) << "Spawning the app failed." << simCtlProcess->errorString();
            response.commandOutput = simCtlProcess->errorString().toLatin1();
        }
    } else {
        qCDebug(simulatorLog) << "Spawning the app failed. Check installed app." << appPath;
    }

    if (!fi.isCanceled()) {
        QThread::msleep(500); // give it some time. TODO: find an actual fix.
        fi.reportResult(response);
    }
}

void SimulatorControlPrivate::launchApp(QFutureInterface<SimulatorControl::ResponseData> &fi,
                                        const QString &simUdid, const QString &bundleIdentifier,
                                        qint64 spawnedPID)
{
    SimulatorControl::ResponseData response(simUdid);
    if (!bundleIdentifier.isEmpty()) {
        bool processSpawned = true;
        // Wait for the process to be spawned properly before launching app.
        if (spawnedPID > -1)
            processSpawned = waitForProcessSpawn(spawnedPID, fi);

        if (fi.isCanceled())
            return;

        if (processSpawned) {
            QThread::msleep(500); // give it some time. TODO: find an actual fix.
            const QStringList args({QStringLiteral("launch"), simUdid , bundleIdentifier});
            response.commandOutput = runSimCtlCommand(args);
            const QByteArray pIdStr = response.commandOutput.trimmed().split(' ').last().trimmed();
            bool validInt = false;
            response.pID = pIdStr.toLongLong(&validInt);
            if (!validInt) {
                // Launch Failed.
                qCDebug(simulatorLog) << "Launch app failed. Process id returned is not valid. PID =" << pIdStr;
                response.pID = -1;
            }
        }
    }

    if (!fi.isCanceled()) {
        fi.reportResult(response);
    }
}

} // namespace Internal
} // namespace Ios
