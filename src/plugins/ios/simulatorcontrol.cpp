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

static QByteArray runSimCtlCommand(QStringList args)
{
    QByteArray output;
    args.prepend(QStringLiteral("simctl"));
    runCommand(QStringLiteral("xcrun"), args, &output);
    return output;
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
    void launchApp(QFutureInterface<SimulatorControl::ResponseData> &fi, const QString &simUdid,
                   const QString &bundleIdentifier, bool waitForDebugger,
                   const QStringList &extraArgs, const QString &stdoutPath,
                   const QString &stderrPath);

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
        fi.reportResult(response);
    }
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

        response.commandOutput = runSimCtlCommand(args);
        const QByteArray pIdStr = response.commandOutput.trimmed().split(' ').last().trimmed();
        bool validPid = false;
        response.pID = pIdStr.toLongLong(&validPid);
        response.success = validPid;
    }

    if (!fi.isCanceled()) {
        fi.reportResult(response);
    }
}

} // namespace Internal
} // namespace Ios
