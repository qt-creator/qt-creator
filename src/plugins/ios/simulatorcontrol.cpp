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
#include "iosconfigurations.h"

#include "utils/algorithm.h"
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
#include <QProcess>

using namespace std;

namespace {
Q_LOGGING_CATEGORY(simulatorLog, "qtc.ios.simulator")
}

namespace Ios {
namespace Internal {

static int SIMULATOR_START_TIMEOUT = 60000;

// simctl Json Tags and tokens.
static QString DeviceTypeTag = QStringLiteral("devicetypes");
static QString DevicesTag = QStringLiteral("devices");
static QString AvailabilityTag = QStringLiteral("availability");
static QString UnavailabilityToken = QStringLiteral("unavailable");
static QString IdentifierTag = QStringLiteral("identifier");
static QString RuntimesTag = QStringLiteral("runtimes");
static QString NameTag = QStringLiteral("name");
static QString StateTag = QStringLiteral("state");
static QString UdidTag = QStringLiteral("udid");
static QString RuntimeVersionTag = QStringLiteral("version");
static QString BuildVersionTag = QStringLiteral("buildversion");

static bool checkForTimeout(const chrono::high_resolution_clock::time_point &start, int msecs = 10000)
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
    p.setTimeoutS(-1);
    Utils::SynchronousProcessResponse resp = p.runBlocking(command, args);
    if (output)
        *output = resp.allRawOutput();
    return resp.result == Utils::SynchronousProcessResponse::Finished;
}

static bool runSimCtlCommand(QStringList args, QByteArray *output)
{
    args.prepend(QStringLiteral("simctl"));
    return runCommand(QStringLiteral("xcrun"), args, output);
}

static QList<DeviceTypeInfo> getAvailableDeviceTypes()
{
    QList<DeviceTypeInfo> deviceTypes;
    QByteArray output;
    runSimCtlCommand({QLatin1String("list"), QLatin1String("-j"), DeviceTypeTag}, &output);
    QJsonDocument doc = QJsonDocument::fromJson(output);
    if (!doc.isNull()) {
        const QJsonArray runtimesArray = doc.object().value(DeviceTypeTag).toArray();
        foreach (const QJsonValue deviceTypeValue, runtimesArray) {
            QJsonObject deviceTypeObject = deviceTypeValue.toObject();
            if (!deviceTypeObject.value(AvailabilityTag).toString().contains(UnavailabilityToken)) {
                DeviceTypeInfo deviceType;
                deviceType.name = deviceTypeObject.value(NameTag).toString("unknown");
                deviceType.identifier = deviceTypeObject.value(IdentifierTag).toString("unknown");
                deviceTypes.append(deviceType);
            }
        }
        stable_sort(deviceTypes.begin(), deviceTypes.end());
    } else {
        qCDebug(simulatorLog) << "Error parsing json output from simctl. Output:" << output;
    }
    return deviceTypes;
}

static QList<RuntimeInfo> getAvailableRuntimes()
{
    QList<RuntimeInfo> runtimes;
    QByteArray output;
    runSimCtlCommand({QLatin1String("list"), QLatin1String("-j"), RuntimesTag}, &output);
    QJsonDocument doc = QJsonDocument::fromJson(output);
    if (!doc.isNull()) {
        const QJsonArray runtimesArray = doc.object().value(RuntimesTag).toArray();
        foreach (const QJsonValue runtimeValue, runtimesArray) {
            QJsonObject runtimeObject = runtimeValue.toObject();
            if (!runtimeObject.value(AvailabilityTag).toString().contains(UnavailabilityToken)) {
                RuntimeInfo runtime;
                runtime.name = runtimeObject.value(NameTag).toString("unknown");
                runtime.build = runtimeObject.value(BuildVersionTag).toString("unknown");
                runtime.identifier = runtimeObject.value(IdentifierTag).toString("unknown");
                runtime.version = runtimeObject.value(RuntimeVersionTag).toString("unknown");
                runtimes.append(runtime);
            }
        }
        stable_sort(runtimes.begin(), runtimes.end());
    } else {
        qCDebug(simulatorLog) << "Error parsing json output from simctl. Output:" << output;
    }
    return runtimes;
}

class SimulatorControlPrivate {
private:
    SimulatorControlPrivate();
    ~SimulatorControlPrivate();

    static SimulatorInfo deviceInfo(const QString &simUdid);
    static QString bundleIdentifier(const Utils::FileName &bundlePath);
    static QString bundleExecutable(const Utils::FileName &bundlePath);

    void startSimulator(QFutureInterface<SimulatorControl::ResponseData> &fi, const QString &simUdid);
    void installApp(QFutureInterface<SimulatorControl::ResponseData> &fi, const QString &simUdid,
                    const Utils::FileName &bundlePath);
    void launchApp(QFutureInterface<SimulatorControl::ResponseData> &fi, const QString &simUdid,
                   const QString &bundleIdentifier, bool waitForDebugger,
                   const QStringList &extraArgs, const QString &stdoutPath,
                   const QString &stderrPath);
    void deleteSimulator(QFutureInterface<SimulatorControl::ResponseData> &fi,
                         const QString &simUdid);
    void resetSimulator(QFutureInterface<SimulatorControl::ResponseData> &fi,
                        const QString &simUdid);
    void renameSimulator(QFutureInterface<SimulatorControl::ResponseData> &fi,
                        const QString &simUdid, const QString &newName);
    void createSimulator(QFutureInterface<SimulatorControl::ResponseData> &fi,
                         const QString &name, const DeviceTypeInfo &deviceType,
                         const RuntimeInfo &runtime);
    void takeSceenshot(QFutureInterface<SimulatorControl::ResponseData> &fi, const QString &simUdid,
                       const QString &filePath);

    static QList<SimulatorInfo> availableDevices;
    static QList<DeviceTypeInfo> availableDeviceTypes;
    static QList<RuntimeInfo> availableRuntimes;
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

QList<SimulatorInfo> SimulatorControl::availableSimulators()
{
    return SimulatorControlPrivate::availableDevices;
}

static QList<SimulatorInfo> getAllSimulatorDevices()
{
    QList<SimulatorInfo> simulatorDevices;
    QByteArray output;
    runSimCtlCommand({QLatin1String("list"), QLatin1String("-j"), DevicesTag}, &output);
    QJsonDocument doc = QJsonDocument::fromJson(output);
    if (!doc.isNull()) {
        const QJsonObject runtimeObject = doc.object().value(DevicesTag).toObject();
        foreach (const QString &runtime, runtimeObject.keys()) {
            const QJsonArray devices = runtimeObject.value(runtime).toArray();
            foreach (const QJsonValue deviceValue, devices) {
                QJsonObject deviceObject = deviceValue.toObject();
                SimulatorInfo device;
                device.identifier = deviceObject.value(UdidTag).toString();
                device.name = deviceObject.value(NameTag).toString();
                device.runtimeName = runtime;
                const QString availableStr = deviceObject.value(AvailabilityTag).toString();
                device.available = !availableStr.contains(UnavailabilityToken);
                device.state = deviceObject.value(StateTag).toString();
                simulatorDevices.append(device);
            }
        }
        stable_sort(simulatorDevices.begin(), simulatorDevices.end());
    } else {
        qCDebug(simulatorLog) << "Error parsing json output from simctl. Output:" << output;
    }
    return simulatorDevices;
}

static QList<SimulatorInfo> getAvailableSimulators()
{
    auto filterSim = [](const SimulatorInfo &device) { return device.available;};
    QList<SimulatorInfo> availableDevices = Utils::filtered(getAllSimulatorDevices(), filterSim);
    return availableDevices;
}

QFuture<QList<DeviceTypeInfo> > SimulatorControl::updateDeviceTypes()
{
    QFuture< QList<DeviceTypeInfo> > future = Utils::runAsync(getAvailableDeviceTypes);
    Utils::onResultReady(future, [](const QList<DeviceTypeInfo> &deviceTypes) {
        SimulatorControlPrivate::availableDeviceTypes = deviceTypes;
    });
    return future;
}

QList<RuntimeInfo> SimulatorControl::availableRuntimes()
{
    return SimulatorControlPrivate::availableRuntimes;
}

QFuture<QList<RuntimeInfo> > SimulatorControl::updateRuntimes()
{
    QFuture< QList<RuntimeInfo> > future = Utils::runAsync(getAvailableRuntimes);
    Utils::onResultReady(future, [](const QList<RuntimeInfo> &runtimes) {
        SimulatorControlPrivate::availableRuntimes = runtimes;
    });
    return future;
}

QFuture< QList<SimulatorInfo> > SimulatorControl::updateAvailableSimulators()
{
    QFuture< QList<SimulatorInfo> > future = Utils::runAsync(getAvailableSimulators);
    Utils::onResultReady(future, [](const QList<SimulatorInfo> &devices) {
        SimulatorControlPrivate::availableDevices = devices;
    });
    return future;
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

QFuture<SimulatorControl::ResponseData> SimulatorControl::startSimulator(const QString &simUdid) const
{
    return Utils::runAsync(&SimulatorControlPrivate::startSimulator, d, simUdid);
}

QFuture<SimulatorControl::ResponseData>
SimulatorControl::installApp(const QString &simUdid, const Utils::FileName &bundlePath) const
{
    return Utils::runAsync(&SimulatorControlPrivate::installApp, d, simUdid, bundlePath);
}

QFuture<SimulatorControl::ResponseData>
SimulatorControl::launchApp(const QString &simUdid, const QString &bundleIdentifier,
                            bool waitForDebugger, const QStringList &extraArgs,
                            const QString &stdoutPath, const QString &stderrPath) const
{
    return Utils::runAsync(&SimulatorControlPrivate::launchApp, d, simUdid, bundleIdentifier,
                           waitForDebugger, extraArgs, stdoutPath, stderrPath);
}

QFuture<SimulatorControl::ResponseData> SimulatorControl::deleteSimulator(const QString &simUdid) const
{
    return Utils::runAsync(&SimulatorControlPrivate::deleteSimulator, d, simUdid);
}

QFuture<SimulatorControl::ResponseData> SimulatorControl::resetSimulator(const QString &simUdid) const
{
    return Utils::runAsync(&SimulatorControlPrivate::resetSimulator, d, simUdid);
}

QFuture<SimulatorControl::ResponseData> SimulatorControl::renameSimulator(const QString &simUdid,
                                                                          const QString &newName) const
{
    return Utils::runAsync(&SimulatorControlPrivate::renameSimulator, d, simUdid, newName);
}

QFuture<SimulatorControl::ResponseData>
SimulatorControl::createSimulator(const QString &name,
                                  const DeviceTypeInfo &deviceType,
                                  const RuntimeInfo &runtime)
{
    return Utils::runAsync(&SimulatorControlPrivate::createSimulator, d, name, deviceType, runtime);
}

QFuture<SimulatorControl::ResponseData> SimulatorControl::takeSceenshot(const QString &simUdid,
                                                                        const QString &filePath)
{
    return Utils::runAsync(&SimulatorControlPrivate::takeSceenshot, d, simUdid, filePath);
}

// Static members
QList<SimulatorInfo> SimulatorControlPrivate::availableDevices;
QList<DeviceTypeInfo> SimulatorControlPrivate::availableDeviceTypes;
QList<RuntimeInfo> SimulatorControlPrivate::availableRuntimes;

SimulatorControlPrivate::SimulatorControlPrivate()
{
}

SimulatorControlPrivate::~SimulatorControlPrivate()
{

}

SimulatorInfo SimulatorControlPrivate::deviceInfo(const QString &simUdid)
{
    auto matchDevice = [simUdid](const SimulatorInfo &device) {
        return device.identifier == simUdid;
    };
    SimulatorInfo device = Utils::findOrDefault(getAllSimulatorDevices(), matchDevice);
    if (device.identifier.isEmpty())
        qCDebug(simulatorLog) << "Cannot find device info. Invalid UDID.";

    return device;
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
    SimulatorInfo simInfo = deviceInfo(simUdid);
    if (simInfo.available && simInfo.isShutdown()) {
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
            SimulatorInfo info;
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
        fi.reportResult(response);
    }
}

void SimulatorControlPrivate::installApp(QFutureInterface<SimulatorControl::ResponseData> &fi,
                                         const QString &simUdid, const Utils::FileName &bundlePath)
{
    QTC_CHECK(bundlePath.exists());

    SimulatorControl::ResponseData response(simUdid);
    response.success = runSimCtlCommand({QStringLiteral("install"), simUdid, bundlePath.toString()},
                                        &response.commandOutput);
    if (!fi.isCanceled())
        fi.reportResult(response);
}

void SimulatorControlPrivate::launchApp(QFutureInterface<SimulatorControl::ResponseData> &fi,
                                        const QString &simUdid, const QString &bundleIdentifier,
                                        bool waitForDebugger, const QStringList &extraArgs,
                                        const QString &stdoutPath, const QString &stderrPath)
{
    SimulatorControl::ResponseData response(simUdid);
    if (!bundleIdentifier.isEmpty() && !fi.isCanceled()) {
        QStringList args({QStringLiteral("launch"), simUdid, bundleIdentifier});

        // simctl usage documentation : Note: Log output is often directed to stderr, not stdout.
        if (!stdoutPath.isEmpty())
            args.insert(1, QStringLiteral("--stderr=%1").arg(stdoutPath));

        if (!stderrPath.isEmpty())
            args.insert(1, QStringLiteral("--stdout=%1").arg(stderrPath));

        if (waitForDebugger)
            args.insert(1, QStringLiteral("-w"));

        foreach (const QString extraArgument, extraArgs) {
            if (!extraArgument.trimmed().isEmpty())
                args << extraArgument;
        }

        if (runSimCtlCommand(args, &response.commandOutput)) {
            const QByteArray pIdStr = response.commandOutput.trimmed().split(' ').last().trimmed();
            bool validPid = false;
            response.pID = pIdStr.toLongLong(&validPid);
            response.success = validPid;
        }
    }

    if (!fi.isCanceled()) {
        fi.reportResult(response);
    }
}

void SimulatorControlPrivate::deleteSimulator(QFutureInterface<SimulatorControl::ResponseData> &fi,
                                              const QString &simUdid)
{
    SimulatorControl::ResponseData response(simUdid);
    response.success = runSimCtlCommand({QStringLiteral("delete"), simUdid}, &response.commandOutput);

    if (!fi.isCanceled())
        fi.reportResult(response);
}

void SimulatorControlPrivate::resetSimulator(QFutureInterface<SimulatorControl::ResponseData> &fi,
                                             const QString &simUdid)
{
    SimulatorControl::ResponseData response(simUdid);
    response.success = runSimCtlCommand({QStringLiteral("erase"), simUdid}, &response.commandOutput);

    if (!fi.isCanceled())
        fi.reportResult(response);
}

void SimulatorControlPrivate::renameSimulator(QFutureInterface<SimulatorControl::ResponseData> &fi,
                                             const QString &simUdid, const QString &newName)
{
    SimulatorControl::ResponseData response(simUdid);
    response.success = runSimCtlCommand({QStringLiteral("rename"), simUdid, newName},
                                        &response.commandOutput);

    if (!fi.isCanceled())
        fi.reportResult(response);
}

void SimulatorControlPrivate::createSimulator(QFutureInterface<SimulatorControl::ResponseData> &fi,
                                              const QString &name,
                                              const DeviceTypeInfo &deviceType,
                                              const RuntimeInfo &runtime)
{
    SimulatorControl::ResponseData response("Invalid");
    if (!name.isEmpty()) {
        response.success = runSimCtlCommand({QStringLiteral("create"), name,
                                             deviceType.identifier,
                                             runtime.identifier},
                                            &response.commandOutput);
        response.simUdid = response.success ? QString::fromLatin1(response.commandOutput.trimmed())
                                            : QString();
    }

    if (!fi.isCanceled())
        fi.reportResult(response);
}

void SimulatorControlPrivate::takeSceenshot(QFutureInterface<SimulatorControl::ResponseData> &fi,
                                            const QString &simUdid, const QString &filePath)
{
    SimulatorControl::ResponseData response(simUdid);
    response.success = runSimCtlCommand({QStringLiteral("io"), simUdid, QStringLiteral("screenshot"),
                                         filePath},
                                        &response.commandOutput);
    if (!fi.isCanceled())
        fi.reportResult(response);
}

} // namespace Internal
} // namespace Ios
