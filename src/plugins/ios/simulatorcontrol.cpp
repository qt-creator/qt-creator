// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "simulatorcontrol.h"
#include "iosconfigurations.h"

#include <utils/algorithm.h>
#include <utils/asynctask.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#ifdef Q_OS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <chrono>
#include <memory>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

using namespace Utils;
using namespace std;

namespace {
static Q_LOGGING_CATEGORY(simulatorLog, "qtc.ios.simulator", QtWarningMsg)
}

namespace Ios::Internal {

const int simulatorStartTimeout = 60000;

// simctl Json Tags and tokens.
const char deviceTypeTag[] = "devicetypes";
const char devicesTag[] = "devices";
const char availabilityTag[] = "availability";
const char unavailabilityToken[] = "unavailable";
const char availabilityTagNew[] = "isAvailable"; // at least since Xcode 10
const char identifierTag[] = "identifier";
const char runtimesTag[] = "runtimes";
const char nameTag[] = "name";
const char stateTag[] = "state";
const char udidTag[] = "udid";
const char runtimeVersionTag[] = "version";
const char buildVersionTag[] = "buildversion";

static bool checkForTimeout(const chrono::high_resolution_clock::time_point &start, int msecs = 10000)
{
    bool timedOut = false;
    auto end = chrono::high_resolution_clock::now();
    if (chrono::duration_cast<chrono::milliseconds>(end-start).count() > msecs)
        timedOut = true;
    return timedOut;
}

static bool runCommand(const CommandLine &command, QString *stdOutput, QString *allOutput = nullptr)
{
    QtcProcess p;
    p.setTimeoutS(-1);
    p.setCommand(command);
    p.runBlocking();
    if (stdOutput)
        *stdOutput = p.cleanedStdOut();
    if (allOutput)
        *allOutput = p.allOutput();
    return p.result() == ProcessResult::FinishedWithSuccess;
}

static bool runSimCtlCommand(QStringList args, QString *output, QString *allOutput = nullptr)
{
    args.prepend("simctl");

    // Cache xcrun's path, as this function will be called often.
    static FilePath xcrun = FilePath::fromString("xcrun").searchInPath();
    QTC_ASSERT(!xcrun.isEmpty() && xcrun.isExecutableFile(), xcrun.clear(); return false);
    return runCommand({xcrun, args}, output, allOutput);
}

static bool launchSimulator(const QString &simUdid) {
    QTC_ASSERT(!simUdid.isEmpty(), return false);
    const FilePath simulatorAppPath = IosConfigurations::developerPath()
            .pathAppended("Applications/Simulator.app/Contents/MacOS/Simulator");

    if (IosConfigurations::xcodeVersion() >= QVersionNumber(9)) {
        // For XCode 9 boot the second device instead of launching simulator app twice.
        QString psOutput;
        if (runCommand({"ps", {"-A", "-o", "comm"}}, &psOutput)) {
            for (const QString &comm : psOutput.split('\n')) {
                if (comm == simulatorAppPath.toString())
                    return runSimCtlCommand({"boot", simUdid}, nullptr);
            }
        } else {
            qCDebug(simulatorLog) << "Cannot start Simulator device."
                                  << "Error probing Simulator.app instance";
            return false;
        }
    }

    return QtcProcess::startDetached({simulatorAppPath, {"--args", "-CurrentDeviceUDID", simUdid}});
}

static bool isAvailable(const QJsonObject &object)
{
    return object.contains(availabilityTagNew)
               ? object.value(availabilityTagNew).toBool()
               : !object.value(availabilityTag).toString().contains(unavailabilityToken);
}

static QList<DeviceTypeInfo> getAvailableDeviceTypes()
{
    QList<DeviceTypeInfo> deviceTypes;
    QString output;
    runSimCtlCommand({"list", "-j", deviceTypeTag}, &output);
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    if (!doc.isNull()) {
        const QJsonArray runtimesArray = doc.object().value(deviceTypeTag).toArray();
        for (const QJsonValue deviceTypeValue : runtimesArray) {
            QJsonObject deviceTypeObject = deviceTypeValue.toObject();
            if (isAvailable(deviceTypeObject)) {
                DeviceTypeInfo deviceType;
                deviceType.name = deviceTypeObject.value(nameTag).toString("unknown");
                deviceType.identifier = deviceTypeObject.value(identifierTag).toString("unknown");
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
    QString output;
    runSimCtlCommand({"list", "-j", runtimesTag}, &output);
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    if (!doc.isNull()) {
        const QJsonArray runtimesArray = doc.object().value(runtimesTag).toArray();
        for (const QJsonValue runtimeValue : runtimesArray) {
            QJsonObject runtimeObject = runtimeValue.toObject();
            if (isAvailable(runtimeObject)) {
                RuntimeInfo runtime;
                runtime.name = runtimeObject.value(nameTag).toString("unknown");
                runtime.build = runtimeObject.value(buildVersionTag).toString("unknown");
                runtime.identifier = runtimeObject.value(identifierTag).toString("unknown");
                runtime.version = runtimeObject.value(runtimeVersionTag).toString("unknown");
                runtimes.append(runtime);
            }
        }
        stable_sort(runtimes.begin(), runtimes.end());
    } else {
        qCDebug(simulatorLog) << "Error parsing json output from simctl. Output:" << output;
    }
    return runtimes;
}

static SimulatorInfo deviceInfo(const QString &simUdid);
static QString bundleIdentifier(const Utils::FilePath &bundlePath);
static QString bundleExecutable(const Utils::FilePath &bundlePath);

static void startSimulator(QPromise<SimulatorControl::ResponseData> &promise,
                           const QString &simUdid);
static void installApp(QPromise<SimulatorControl::ResponseData> &promise,
                       const QString &simUdid,
                       const Utils::FilePath &bundlePath);
static void launchApp(QPromise<SimulatorControl::ResponseData> &promise,
                      const QString &simUdid,
                      const QString &bundleIdentifier,
                      bool waitForDebugger,
                      const QStringList &extraArgs,
                      const QString &stdoutPath,
                      const QString &stderrPath);
static void deleteSimulator(QPromise<SimulatorControl::ResponseData> &promise,
                            const QString &simUdid);
static void resetSimulator(QPromise<SimulatorControl::ResponseData> &promise,
                           const QString &simUdid);
static void renameSimulator(QPromise<SimulatorControl::ResponseData> &promise,
                            const QString &simUdid,
                            const QString &newName);
static void createSimulator(QPromise<SimulatorControl::ResponseData> &promise,
                            const QString &name,
                            const DeviceTypeInfo &deviceType,
                            const RuntimeInfo &runtime);
static void takeSceenshot(QPromise<SimulatorControl::ResponseData> &promise,
                          const QString &simUdid,
                          const QString &filePath);

static QList<SimulatorInfo> s_availableDevices;
static QList<DeviceTypeInfo> s_availableDeviceTypes;
static QList<RuntimeInfo> s_availableRuntimes;

QList<SimulatorInfo> SimulatorControl::availableSimulators()
{
    return s_availableDevices;
}

static QList<SimulatorInfo> getAllSimulatorDevices()
{
    QList<SimulatorInfo> simulatorDevices;
    QString output;
    runSimCtlCommand({"list", "-j", devicesTag}, &output);
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    if (!doc.isNull()) {
        const QJsonObject runtimeObject = doc.object().value(devicesTag).toObject();
        const QStringList keys = runtimeObject.keys();
        for (const QString &runtime : keys) {
            const QJsonArray devices = runtimeObject.value(runtime).toArray();
            for (const QJsonValue deviceValue : devices) {
                QJsonObject deviceObject = deviceValue.toObject();
                SimulatorInfo device;
                device.identifier = deviceObject.value(udidTag).toString();
                device.name = deviceObject.value(nameTag).toString();
                device.runtimeName = runtime;
                device.available = isAvailable(deviceObject);
                device.state = deviceObject.value(stateTag).toString();
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

QFuture<QList<DeviceTypeInfo>> SimulatorControl::updateDeviceTypes(QObject *context)
{
    QFuture<QList<DeviceTypeInfo>> future = Utils::asyncRun(getAvailableDeviceTypes);
    Utils::onResultReady(future, context, [](const QList<DeviceTypeInfo> &deviceTypes) {
        s_availableDeviceTypes = deviceTypes;
    });
    return future;
}

QList<RuntimeInfo> SimulatorControl::availableRuntimes()
{
    return s_availableRuntimes;
}

QFuture<QList<RuntimeInfo>> SimulatorControl::updateRuntimes(QObject *context)
{
    QFuture<QList<RuntimeInfo>> future = Utils::asyncRun(getAvailableRuntimes);
    Utils::onResultReady(future, context, [](const QList<RuntimeInfo> &runtimes) {
        s_availableRuntimes = runtimes;
    });
    return future;
}

QFuture<QList<SimulatorInfo>> SimulatorControl::updateAvailableSimulators(QObject *context)
{
    QFuture<QList<SimulatorInfo>> future = Utils::asyncRun(getAvailableSimulators);
    Utils::onResultReady(future, context, [](const QList<SimulatorInfo> &devices) {
        s_availableDevices = devices;
    });
    return future;
}

bool SimulatorControl::isSimulatorRunning(const QString &simUdid)
{
    if (simUdid.isEmpty())
        return false;
    return deviceInfo(simUdid).isBooted();
}

QString SimulatorControl::bundleIdentifier(const Utils::FilePath &bundlePath)
{
    return Internal::bundleIdentifier(bundlePath);
}

QString SimulatorControl::bundleExecutable(const Utils::FilePath &bundlePath)
{
    return Internal::bundleExecutable(bundlePath);
}

QFuture<SimulatorControl::ResponseData> SimulatorControl::startSimulator(const QString &simUdid)
{
    return Utils::asyncRun(Internal::startSimulator, simUdid);
}

QFuture<SimulatorControl::ResponseData> SimulatorControl::installApp(
    const QString &simUdid, const Utils::FilePath &bundlePath)
{
    return Utils::asyncRun(Internal::installApp, simUdid, bundlePath);
}

QFuture<SimulatorControl::ResponseData> SimulatorControl::launchApp(const QString &simUdid,
                                                                    const QString &bundleIdentifier,
                                                                    bool waitForDebugger,
                                                                    const QStringList &extraArgs,
                                                                    const QString &stdoutPath,
                                                                    const QString &stderrPath)
{
    return Utils::asyncRun(Internal::launchApp,
                           simUdid,
                           bundleIdentifier,
                           waitForDebugger,
                           extraArgs,
                           stdoutPath,
                           stderrPath);
}

QFuture<SimulatorControl::ResponseData> SimulatorControl::deleteSimulator(const QString &simUdid)
{
    return Utils::asyncRun(Internal::deleteSimulator, simUdid);
}

QFuture<SimulatorControl::ResponseData> SimulatorControl::resetSimulator(const QString &simUdid)
{
    return Utils::asyncRun(Internal::resetSimulator, simUdid);
}

QFuture<SimulatorControl::ResponseData> SimulatorControl::renameSimulator(const QString &simUdid,
                                                                          const QString &newName)
{
    return Utils::asyncRun(Internal::renameSimulator, simUdid, newName);
}

QFuture<SimulatorControl::ResponseData>
SimulatorControl::createSimulator(const QString &name,
                                  const DeviceTypeInfo &deviceType,
                                  const RuntimeInfo &runtime)
{
    return Utils::asyncRun(Internal::createSimulator, name, deviceType, runtime);
}

QFuture<SimulatorControl::ResponseData> SimulatorControl::takeSceenshot(const QString &simUdid,
                                                                        const QString &filePath)
{
    return Utils::asyncRun(Internal::takeSceenshot, simUdid, filePath);
}

// Static members

SimulatorInfo deviceInfo(const QString &simUdid)
{
    auto matchDevice = [simUdid](const SimulatorInfo &device) {
        return device.identifier == simUdid;
    };
    SimulatorInfo device = Utils::findOrDefault(getAllSimulatorDevices(), matchDevice);
    if (device.identifier.isEmpty())
        qCDebug(simulatorLog) << "Cannot find device info. Invalid UDID.";

    return device;
}

QString bundleIdentifier(const Utils::FilePath &bundlePath)
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

QString bundleExecutable(const Utils::FilePath &bundlePath)
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

void startSimulator(QPromise<SimulatorControl::ResponseData> &promise, const QString &simUdid)
{
    SimulatorControl::ResponseData response(simUdid);
    SimulatorInfo simInfo = deviceInfo(simUdid);

    if (!simInfo.available) {
        qCDebug(simulatorLog) << "Simulator device is not available." << simUdid;
        return;
    }

    // Shutting down state checks are for the case when simulator start is called within a short
    // interval of closing the previous interval of the simulator. We wait untill the shutdown
    // process is complete.
    auto start = chrono::high_resolution_clock::now();
    while (simInfo.isShuttingDown() && !checkForTimeout(start, simulatorStartTimeout)) {
        // Wait till the simulator shuts down, if doing so.
        QThread::msleep(100);
        simInfo = deviceInfo(simUdid);
    }

    if (simInfo.isShuttingDown()) {
        qCDebug(simulatorLog) << "Cannot start Simulator device. "
                              << "Previous instance taking too long to shutdown." << simInfo;
        return;
    }

    if (simInfo.isShutdown()) {
        if (launchSimulator(simUdid)) {
            if (promise.isCanceled())
                return;
            // At this point the sim device exists, available and was not running.
            // So the simulator is started and we'll wait for it to reach to a state
            // where we can interact with it.
            start = chrono::high_resolution_clock::now();
            SimulatorInfo info;
            do {
                info = deviceInfo(simUdid);
                if (promise.isCanceled())
                    return;
            } while (!info.isBooted() && !checkForTimeout(start, simulatorStartTimeout));
            if (info.isBooted())
                response.success = true;
        } else {
            qCDebug(simulatorLog) << "Error starting simulator.";
        }
    } else {
       qCDebug(simulatorLog) << "Cannot start Simulator device. Simulator not in shutdown state."
                             << simInfo;
    }

    if (!promise.isCanceled())
        promise.addResult(response);
}

void installApp(QPromise<SimulatorControl::ResponseData> &promise,
                const QString &simUdid, const Utils::FilePath &bundlePath)
{
    QTC_CHECK(bundlePath.exists());

    SimulatorControl::ResponseData response(simUdid);
    response.success = runSimCtlCommand({"install", simUdid, bundlePath.toString()},
                                        nullptr,
                                        &response.commandOutput);
    if (!promise.isCanceled())
        promise.addResult(response);
}

void launchApp(QPromise<SimulatorControl::ResponseData> &promise,
               const QString &simUdid,
               const QString &bundleIdentifier,
               bool waitForDebugger,
               const QStringList &extraArgs,
               const QString &stdoutPath,
               const QString &stderrPath)
{
    SimulatorControl::ResponseData response(simUdid);
    if (!bundleIdentifier.isEmpty() && !promise.isCanceled()) {
        QStringList args({"launch", simUdid, bundleIdentifier});

        // simctl usage documentation : Note: Log output is often directed to stderr, not stdout.
        if (!stdoutPath.isEmpty())
            args.insert(1, QString("--stderr=%1").arg(stdoutPath));

        if (!stderrPath.isEmpty())
            args.insert(1, QString("--stdout=%1").arg(stderrPath));

        if (waitForDebugger)
            args.insert(1, "-w");

        for (const QString &extraArgument : extraArgs) {
            if (!extraArgument.trimmed().isEmpty())
                args << extraArgument;
        }

        QString stdOutput;
        if (runSimCtlCommand(args, &stdOutput, &response.commandOutput)) {
            const QString pIdStr = stdOutput.trimmed().split(' ').last().trimmed();
            bool validPid = false;
            response.pID = pIdStr.toLongLong(&validPid);
            response.success = validPid;
        }
    }

    if (!promise.isCanceled())
        promise.addResult(response);
}

void deleteSimulator(QPromise<SimulatorControl::ResponseData> &promise, const QString &simUdid)
{
    SimulatorControl::ResponseData response(simUdid);
    response.success = runSimCtlCommand({"delete", simUdid}, nullptr, &response.commandOutput);

    if (!promise.isCanceled())
        promise.addResult(response);
}

void resetSimulator(QPromise<SimulatorControl::ResponseData> &promise, const QString &simUdid)
{
    SimulatorControl::ResponseData response(simUdid);
    response.success = runSimCtlCommand({"erase", simUdid}, nullptr, &response.commandOutput);

    if (!promise.isCanceled())
        promise.addResult(response);
}

void renameSimulator(QPromise<SimulatorControl::ResponseData> &promise,
                     const QString &simUdid,
                     const QString &newName)
{
    SimulatorControl::ResponseData response(simUdid);
    response.success = runSimCtlCommand({"rename", simUdid, newName},
                                        nullptr,
                                        &response.commandOutput);
    if (!promise.isCanceled())
        promise.addResult(response);
}

void createSimulator(QPromise<SimulatorControl::ResponseData> &promise,
                     const QString &name,
                     const DeviceTypeInfo &deviceType,
                     const RuntimeInfo &runtime)
{
    SimulatorControl::ResponseData response("Invalid");
    if (!name.isEmpty()) {
        QString stdOutput;
        response.success
            = runSimCtlCommand({"create", name, deviceType.identifier, runtime.identifier},
                               &stdOutput,
                               &response.commandOutput);
        response.simUdid = response.success ? stdOutput.trimmed() : QString();
    }

    if (!promise.isCanceled())
        promise.addResult(response);
}

void takeSceenshot(QPromise<SimulatorControl::ResponseData> &promise,
                   const QString &simUdid,
                   const QString &filePath)
{
    SimulatorControl::ResponseData response(simUdid);
    response.success = runSimCtlCommand({"io", simUdid, "screenshot", filePath},
                                        nullptr,
                                        &response.commandOutput);
    if (!promise.isCanceled())
        promise.addResult(response);
}

QDebug &operator<<(QDebug &stream, const SimulatorInfo &info)
{
    stream << "Name: " << info.name << "UDID: " << info.identifier
           << "Availability: " << info.available << "State: " << info.state
           << "Runtime: " << info.runtimeName;
    return stream;
}

bool SimulatorInfo::operator==(const SimulatorInfo &other) const
{
    return identifier == other.identifier
            && state == other.state
            && name == other.name
            && available == other.available
            && runtimeName == other.runtimeName;
}

} // Ios::Internal
