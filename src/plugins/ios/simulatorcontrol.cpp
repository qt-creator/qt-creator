// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "simulatorcontrol.h"
#include "iosconfigurations.h"
#include "iostr.h"

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

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

const std::chrono::seconds simulatorStartTimeout = std::chrono::seconds(60);

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

static expected_str<void> runCommand(
    const CommandLine &command,
    QString *stdOutput,
    QString *allOutput = nullptr,
    std::function<bool()> shouldStop = [] { return false; })
{
    Process p;
    p.setCommand(command);
    p.start();

    if (!p.waitForStarted())
        return make_unexpected(Tr::tr("Failed to start process."));

    forever {
        if (shouldStop() || p.waitForFinished(1000))
            break;
    }

    if (p.state() != QProcess::ProcessState::NotRunning) {
        p.kill();
        if (shouldStop())
            return make_unexpected(Tr::tr("Process was canceled."));

        return make_unexpected(Tr::tr("Process was forced to exit."));
    }

    if (stdOutput)
        *stdOutput = p.cleanedStdOut();
    if (allOutput)
        *allOutput = p.allOutput();

    if (p.result() != ProcessResult::FinishedWithSuccess)
        return make_unexpected(p.errorString());

    return {};
}

static expected_str<void> runSimCtlCommand(
    QStringList args,
    QString *output,
    QString *allOutput = nullptr,
    std::function<bool()> shouldStop = [] { return false; })
{
    args.prepend("simctl");

    // Cache xcrun's path, as this function will be called often.
    static FilePath xcrun = FilePath::fromString("xcrun").searchInPath();
    if (xcrun.isEmpty())
        return make_unexpected(Tr::tr("Cannot find xcrun."));
    else if (!xcrun.isExecutableFile())
        return make_unexpected(Tr::tr("xcrun is not executable."));
    return runCommand({xcrun, args}, output, allOutput, shouldStop);
}

static expected_str<void> launchSimulator(const QString &simUdid, std::function<bool()> shouldStop)
{
    QTC_ASSERT(!simUdid.isEmpty(), return make_unexpected(Tr::tr("Invalid Empty UDID.")));
    const FilePath simulatorAppPath = IosConfigurations::developerPath()
            .pathAppended("Applications/Simulator.app/Contents/MacOS/Simulator");

    if (IosConfigurations::xcodeVersion() >= QVersionNumber(9)) {
        // For XCode 9 boot the second device instead of launching simulator app twice.
        QString psOutput;
        expected_str<void> result
            = runCommand({"ps", {"-A", "-o", "comm"}}, &psOutput, nullptr, shouldStop);
        if (!result)
            return result;

        for (const QString &comm : psOutput.split('\n')) {
            if (comm == simulatorAppPath.toString())
                return runSimCtlCommand({"boot", simUdid}, nullptr, nullptr, shouldStop);
        }
    }
    const bool started = Process::startDetached(
        {simulatorAppPath, {"--args", "-CurrentDeviceUDID", simUdid}});
    if (!started)
        return make_unexpected(Tr::tr("Failed to start simulator app."));
    return {};
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

static void startSimulator(QPromise<SimulatorControl::Response> &promise, const QString &simUdid);
static void installApp(QPromise<SimulatorControl::Response> &promise,
                       const QString &simUdid,
                       const Utils::FilePath &bundlePath);
static void launchApp(QPromise<SimulatorControl::Response> &promise,
                      const QString &simUdid,
                      const QString &bundleIdentifier,
                      bool waitForDebugger,
                      const QStringList &extraArgs,
                      const QString &stdoutPath,
                      const QString &stderrPath);
static void deleteSimulator(QPromise<SimulatorControl::Response> &promise, const QString &simUdid);
static void resetSimulator(QPromise<SimulatorControl::Response> &promise, const QString &simUdid);
static void renameSimulator(QPromise<SimulatorControl::Response> &promise,
                            const QString &simUdid,
                            const QString &newName);
static void createSimulator(QPromise<SimulatorControl::Response> &promise,
                            const QString &name,
                            const DeviceTypeInfo &deviceType,
                            const RuntimeInfo &runtime);
static void takeSceenshot(QPromise<SimulatorControl::Response> &promise,
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

QFuture<SimulatorControl::Response> SimulatorControl::startSimulator(const QString &simUdid)
{
    return Utils::asyncRun(Internal::startSimulator, simUdid);
}

QFuture<SimulatorControl::Response> SimulatorControl::installApp(const QString &simUdid,
                                                                 const Utils::FilePath &bundlePath)
{
    return Utils::asyncRun(Internal::installApp, simUdid, bundlePath);
}

QFuture<SimulatorControl::Response> SimulatorControl::launchApp(const QString &simUdid,
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

QFuture<SimulatorControl::Response> SimulatorControl::deleteSimulator(const QString &simUdid)
{
    return Utils::asyncRun(Internal::deleteSimulator, simUdid);
}

QFuture<SimulatorControl::Response> SimulatorControl::resetSimulator(const QString &simUdid)
{
    return Utils::asyncRun(Internal::resetSimulator, simUdid);
}

QFuture<SimulatorControl::Response> SimulatorControl::renameSimulator(const QString &simUdid,
                                                                      const QString &newName)
{
    return Utils::asyncRun(Internal::renameSimulator, simUdid, newName);
}

QFuture<SimulatorControl::Response> SimulatorControl::createSimulator(
    const QString &name, const DeviceTypeInfo &deviceType, const RuntimeInfo &runtime)
{
    return Utils::asyncRun(Internal::createSimulator, name, deviceType, runtime);
}

QFuture<SimulatorControl::Response> SimulatorControl::takeSceenshot(const QString &simUdid,
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

void startSimulator(QPromise<SimulatorControl::Response> &promise, const QString &simUdid)
{
    SimulatorControl::ResponseData response(simUdid);
    SimulatorInfo simInfo = deviceInfo(simUdid);

    if (!simInfo.available) {
        promise.addResult(
            make_unexpected(Tr::tr("Simulator device is not available. (%1)").arg(simUdid)));
        return;
    }
    // Shutting down state checks are for the case when simulator start is called within a short
    // interval of closing the previous interval of the simulator. We wait untill the shutdown
    // process is complete.
    QDeadlineTimer simulatorStartDeadline(simulatorStartTimeout);
    while (simInfo.isShuttingDown() && !simulatorStartDeadline.hasExpired()) {
        // Wait till the simulator shuts down, if doing so.
        if (promise.isCanceled()) {
            promise.addResult(make_unexpected(Tr::tr("Simulator start was canceled.")));
            return;
        }

        QThread::msleep(100);
        simInfo = deviceInfo(simUdid);
    }

    if (simInfo.isShuttingDown()) {
        promise.addResult(
            make_unexpected(Tr::tr("Cannot start Simulator device. Previous instance taking "
                                   "too long to shut down. (%1)")
                                .arg(simInfo.toString())));
        return;
    }

    if (!simInfo.isShutdown()) {
        promise.addResult(make_unexpected(
            Tr::tr("Cannot start Simulator device. Simulator not in shutdown state. (%1)")
                .arg(simInfo.toString())));
        return;
    }

    expected_str<void> result = launchSimulator(simUdid,
                                                [&promise] { return promise.isCanceled(); });
    if (!result) {
        promise.addResult(make_unexpected(result.error()));
        return;
    }

    // At this point the sim device exists, available and was not running.
    // So the simulator is started and we'll wait for it to reach to a state
    // where we can interact with it.
    // We restart the deadline to give it some more time.
    simulatorStartDeadline = QDeadlineTimer(simulatorStartTimeout);
    SimulatorInfo info;
    do {
        info = deviceInfo(simUdid);
        if (promise.isCanceled()) {
            promise.addResult(make_unexpected(Tr::tr("Simulator start was canceled.")));
            return;
        }
    } while (!info.isBooted() && !simulatorStartDeadline.hasExpired());

    if (!info.isBooted()) {
        promise.addResult(make_unexpected(
            Tr::tr("Cannot start Simulator device. Simulator not in booted state. (%1)")
                .arg(info.toString())));
        return;
    }

    promise.addResult(response);
}

void installApp(QPromise<SimulatorControl::Response> &promise,
                const QString &simUdid,
                const Utils::FilePath &bundlePath)
{
    SimulatorControl::ResponseData response(simUdid);

    if (!bundlePath.exists()) {
        promise.addResult(make_unexpected(Tr::tr("Bundle path does not exist.")));
        return;
    }

    expected_str<void> result = runSimCtlCommand({"install", simUdid, bundlePath.toString()},
                                                 nullptr,
                                                 &response.commandOutput,
                                                 [&promise] { return promise.isCanceled(); });
    if (!result)
        promise.addResult(make_unexpected(result.error()));
    else
        promise.addResult(response);
}

void launchApp(QPromise<SimulatorControl::Response> &promise,
               const QString &simUdid,
               const QString &bundleIdentifier,
               bool waitForDebugger,
               const QStringList &extraArgs,
               const QString &stdoutPath,
               const QString &stderrPath)
{
    SimulatorControl::ResponseData response(simUdid);

    if (bundleIdentifier.isEmpty()) {
        promise.addResult(make_unexpected(Tr::tr("Invalid (empty) bundle identifier.")));
        return;
    }

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
    expected_str<void> result = runSimCtlCommand(args,
                                                 &stdOutput,
                                                 &response.commandOutput,
                                                 [&promise] { return promise.isCanceled(); });

    if (!result) {
        promise.addResult(make_unexpected(result.error()));
        return;
    }

    const QString pIdStr = stdOutput.trimmed().split(' ').last().trimmed();
    bool validPid = false;
    response.inferiorPid = pIdStr.toLongLong(&validPid);

    if (!validPid) {
        promise.addResult(
            make_unexpected(Tr::tr("Failed to convert inferior pid. (%1)").arg(pIdStr)));
        return;
    }

    promise.addResult(response);
}

void deleteSimulator(QPromise<SimulatorControl::Response> &promise, const QString &simUdid)
{
    SimulatorControl::ResponseData response(simUdid);
    expected_str<void> result = runSimCtlCommand({"delete", simUdid},
                                                 nullptr,
                                                 &response.commandOutput,
                                                 [&promise] { return promise.isCanceled(); });

    if (!result)
        promise.addResult(make_unexpected(result.error()));
    else
        promise.addResult(response);
}

void resetSimulator(QPromise<SimulatorControl::Response> &promise, const QString &simUdid)
{
    SimulatorControl::ResponseData response(simUdid);
    expected_str<void> result = runSimCtlCommand({"erase", simUdid},
                                                 nullptr,
                                                 &response.commandOutput,
                                                 [&promise] { return promise.isCanceled(); });

    if (!result)
        promise.addResult(make_unexpected(result.error()));
    else
        promise.addResult(response);
}

void renameSimulator(QPromise<SimulatorControl::Response> &promise,
                     const QString &simUdid,
                     const QString &newName)
{
    SimulatorControl::ResponseData response(simUdid);
    expected_str<void> result = runSimCtlCommand({"rename", simUdid, newName},
                                                 nullptr,
                                                 &response.commandOutput,
                                                 [&promise] { return promise.isCanceled(); });
    if (!result)
        promise.addResult(make_unexpected(result.error()));
    else
        promise.addResult(response);
}

void createSimulator(QPromise<SimulatorControl::Response> &promise,
                     const QString &name,
                     const DeviceTypeInfo &deviceType,
                     const RuntimeInfo &runtime)
{
    SimulatorControl::ResponseData response("Invalid");

    if (name.isEmpty()) {
        promise.addResult(response);
        return;
    }

    QString stdOutput;
    expected_str<void> result
        = runSimCtlCommand({"create", name, deviceType.identifier, runtime.identifier},
                           &stdOutput,
                           &response.commandOutput,
                           [&promise] { return promise.isCanceled(); });

    if (result)
        response.simUdid = stdOutput.trimmed();

    if (!result)
        promise.addResult(make_unexpected(result.error()));
    else
        promise.addResult(response);
}

void takeSceenshot(QPromise<SimulatorControl::Response> &promise,
                   const QString &simUdid,
                   const QString &filePath)
{
    SimulatorControl::ResponseData response(simUdid);
    expected_str<void> result = runSimCtlCommand({"io", simUdid, "screenshot", filePath},
                                                 nullptr,
                                                 &response.commandOutput,
                                                 [&promise] { return promise.isCanceled(); });

    if (!result)
        promise.addResult(make_unexpected(result.error()));
    else
        promise.addResult(response);
}

QString SimulatorInfo::toString() const
{
    return QString("Name: %1 UDID: %2 Availability: %3 State: %4 Runtime: %5")
        .arg(name)
        .arg(identifier)
        .arg(available)
        .arg(state)
        .arg(runtimeName);
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
