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
static QString SIM_UDID_TAG = QStringLiteral("SimUdid");

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

    static QList<SimulatorInfo> availableDevices;
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
    runSimCtlCommand({QLatin1String("list"), QLatin1String("-j"), QLatin1String("devices")}, &output);
    QJsonDocument doc = QJsonDocument::fromJson(output);
    if (!doc.isNull()) {
        const QJsonObject runtimeObject = doc.object().value(QStringLiteral("devices")).toObject();
        foreach (const QString &runtime, runtimeObject.keys()) {
            const QJsonArray devices = runtimeObject.value(runtime).toArray();
            foreach (const QJsonValue deviceValue, devices) {
                QJsonObject deviceObject = deviceValue.toObject();
                SimulatorInfo device;
                device.identifier = deviceObject.value(QStringLiteral("udid")).toString();
                device.name = deviceObject.value(QStringLiteral("name")).toString();
                device.runtimeName = runtime;
                const QString availableStr = deviceObject.value(QStringLiteral("availability")).toString();
                device.available = !availableStr.contains("unavailable");
                device.state = deviceObject.value(QStringLiteral("state")).toString();
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

void SimulatorControl::updateAvailableSimulators()
{
    QFuture< QList<SimulatorInfo> > future = Utils::runAsync(getAvailableSimulators);
    Utils::onResultReady(future, [](const QList<SimulatorInfo> &devices) {
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

QList<SimulatorInfo> SimulatorControlPrivate::availableDevices;

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
    if (deviceInfo(simUdid).available) {
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

} // namespace Internal
} // namespace Ios
