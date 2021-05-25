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

#include "iosdevicemanager.h"

#include "mobiledevicelib.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QLibrary>
#include <QMultiHash>
#include <QMutex>
#include <QMutexLocker>
#include <QProcess>
#include <QRegularExpression>
#include <QRegularExpression>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QVersionNumber>

#include <CoreFoundation/CoreFoundation.h>

#include <mach/error.h>
#include <dlfcn.h>

static const bool debugGdbServer = false;
static const bool debugAll = false;
static const bool verbose = true;
static const bool noWifi = true;

// ------- MobileDeviceLib interface --------
namespace {

#ifndef MOBILE_DEV_DIRECT_LINK
/* Messages passed to device notification callbacks: passed as part of
 * AMDeviceNotificationCallbackInfo. */
enum ADNCI_MSG {
    ADNCI_MSG_CONNECTED    = 1,
    ADNCI_MSG_DISCONNECTED = 2,
    ADNCI_MSG_UNSUBSCRIBED = 3
};
#endif



} // anonymous namespace

namespace Ios {
namespace Internal {

static const am_res_t kAMDMobileImageMounterImageMountFailed = 0xe8000076;
static const QString DebugServiceName = "com.apple.debugserver";
static const QString DebugSecureServiceName = "com.apple.debugserver.DVTSecureSocketProxy";


static QString mobileDeviceErrorString(am_res_t code)
{
    QString s = QStringLiteral("Unknown error (0x%08x)").arg(code);

    MobileDeviceLib &mLib = MobileDeviceLib::instance();
    // AMDErrors, 0x0, 0xe8000001-0xe80000db
    if (char *ptr = mLib.errorString(code)) {
        CFStringRef key = QString::fromLatin1(ptr).toCFString();

        CFURLRef url = QUrl::fromLocalFile(
            QStringLiteral("/System/Library/PrivateFrameworks/MobileDevice.framework")).toCFURL();
        CFBundleRef mobileDeviceBundle = CFBundleCreate(kCFAllocatorDefault, url);
        CFRelease(url);

        if (mobileDeviceBundle) {
            CFStringRef str = CFCopyLocalizedStringFromTableInBundle(key, CFSTR("Localizable"),
                                                                     mobileDeviceBundle, nil);
            s = QString::fromCFString(str);
            CFRelease(str);
        }

        CFRelease(key);
    } else if (CFStringRef str = mLib.misErrorStringForErrorCode(code)) {
        // MIS errors, 0xe8008001-0xe800801e
        s = QString::fromCFString(str);
        CFRelease(str);
    }

    return s;
}

static qint64 toBuildNumber(const QString &versionStr)
{
    QString buildNumber;
    const QRegularExpression re("\\s\\((\\X+)\\)");
    const QRegularExpressionMatch m = re.match(versionStr);
    if (m.hasMatch())
        buildNumber = m.captured(1);
    return buildNumber.toLongLong(nullptr, 16);
}

static bool findXcodePath(QString *xcodePath)
{
    if (!xcodePath)
        return false;

    QProcess process;
    process.start("/bin/bash", QStringList({"-c", "xcode-select -p"}));
    if (!process.waitForFinished(3000))
        return false;

    *xcodePath = QString::fromLatin1(process.readAllStandardOutput()).trimmed();
    return (process.exitStatus() == QProcess::NormalExit && QFile::exists(*xcodePath));
}

/*!
 * \brief Finds the \e DeveloperDiskImage.dmg path corresponding to \a versionStr and \a buildStr.
 *
 * The algorithm sorts the \e DeviceSupport directories with decreasing order of their version and
 * build number to find the appropriate \e DeviceSupport directory corresponding to \a versionStr
 * and \a buildStr. Directories starting with major build number of \a versionStr are considered
 * only, rest are filtered out.
 *
 * Returns \c true if \e DeveloperDiskImage.dmg is found, otherwise returns \c false. The absolute
 * path to the \e DeveloperDiskImage.dmg is enumerated in \a path.
 */
static bool findDeveloperDiskImage(const QString &versionStr, const QString &buildStr,
                                   QString *path = nullptr)
{
    const QVersionNumber deviceVersion = QVersionNumber::fromString(versionStr);
    if (deviceVersion.isNull())
        return false;

    QString xcodePath;
    if (!findXcodePath(&xcodePath)) {
        if (debugAll)
            qDebug() << "Error getting xcode installation path.";
        return false;
    }

    // Returns device support directories matching the major version.
    const auto findDeviceSupportDirs = [xcodePath, deviceVersion](const QString &subFolder) {
        const QDir rootDir(QString("%1/%2").arg(xcodePath).arg(subFolder));
        const QStringList filter(QString("%1.*").arg(deviceVersion.majorVersion()));
        return rootDir.entryInfoList(filter, QDir::Dirs | QDir::NoDotAndDotDot);
    };

    // Compares version strings(considers build number) and return true if versionStrA > versionStrB.
    const auto compareVersion = [](const QString &versionStrA, const QString &versionStrB) {
        const auto versionA = QVersionNumber::fromString(versionStrA);
        const auto versionB = QVersionNumber::fromString(versionStrB);
        if (versionA == versionB)
            return toBuildNumber(versionStrA) > toBuildNumber(versionStrB);
        return versionA > versionB;
    };

    // To filter dirs without DeveloperDiskImage.dmg and dirs having higher version.
    const auto filterDir = [compareVersion, versionStr, buildStr](const QFileInfo d) {
        const auto devDiskImage = QString("%1/DeveloperDiskImage.dmg").arg(d.absoluteFilePath());
        if (!QFile::exists(devDiskImage))
            return true; // Dir's without DeveloperDiskImage.dmg
        return compareVersion(d.fileName(), QString("%1 (%2)").arg(versionStr).arg(buildStr));
    };

    QFileInfoList deviceSupportDirs(findDeviceSupportDirs("iOS DeviceSupport"));
    deviceSupportDirs << findDeviceSupportDirs("Platforms/iPhoneOS.platform/DeviceSupport");

    // Remove dirs without DeveloperDiskImage.dmg and dirs having higher version.
    auto endItr = std::remove_if(deviceSupportDirs.begin(), deviceSupportDirs.end(), filterDir);
    deviceSupportDirs.erase(endItr, deviceSupportDirs.end());

    if (deviceSupportDirs.isEmpty())
        return false;

    // Sort device support directories.
    std::sort(deviceSupportDirs.begin(), deviceSupportDirs.end(),
              [compareVersion](const QFileInfo &a, const QFileInfo &b) {
        return compareVersion(a.fileName(), b.fileName());
    });

    if (path)
        *path = QString("%1/DeveloperDiskImage.dmg").arg(deviceSupportDirs[0].absoluteFilePath());
    return true;
}

bool disable_ssl(ServiceConnRef ref)
{
    typedef void (*SSL_free_t)(void*);
    static SSL_free_t SSL_free = nullptr;
    if (!SSL_free)
        SSL_free = (SSL_free_t)dlsym(RTLD_DEFAULT, "SSL_free");
    if (!SSL_free)
        return false;
    SSL_free(ref->sslContext);
    ref->sslContext = nullptr;
    return true;
}

extern "C" {
typedef void (*DeviceAvailableCallback)(QString deviceId, AMDeviceRef, void *userData);
}

class PendingDeviceLookup {
public:
    QTimer timer;
    DeviceAvailableCallback callback;
    void *userData;
};

class CommandSession : public DeviceSession
{
public:
    virtual ~CommandSession();
    explicit CommandSession(const QString &deviceId);

    void internalDeviceAvailableCallback(QString deviceId, AMDeviceRef device);
    virtual void deviceCallbackReturned() { }
    virtual am_res_t appTransferCallback(CFDictionaryRef) { return 0; }
    virtual am_res_t appInstallCallback(CFDictionaryRef) { return 0; }
    virtual void reportProgress(CFDictionaryRef dict);
    virtual void reportProgress2(int progress, const QString &status);
    virtual QString commandName();

    bool connectDevice();
    bool disconnectDevice();
    void stopService(ServiceSocket fd);
    void startDeviceLookup(int timeout);
    bool connectToPort(quint16 port, ServiceSocket *fd) override;
    int qmljsDebugPort() const override;
    void addError(const QString &msg);
    bool writeAll(ServiceSocket fd, const char *cmd, qptrdiff len = -1);
    bool mountDeveloperDiskImage();
    bool sendGdbCommand(ServiceConnRef conn, const char *cmd, qptrdiff len = -1);
    QByteArray readGdbReply(ServiceConnRef conn);
    bool expectGdbReply(ServiceConnRef conn, QByteArray expected);
    bool expectGdbOkReply(ServiceConnRef conn);
    bool startServiceSecure(const QString &serviceName, ServiceConnRef &conn);

    AMDeviceRef device;
    int progressBase;
    int unexpectedChars;
    bool aknowledge;

private:
    bool developerDiskImagePath(QString *path, QString *signaturePath);

private:
    bool checkRead(qptrdiff nRead, int &maxRetry);
    int handleChar(ServiceConnRef conn, QByteArray &res, char c, int status);
};

// ------- IosManagerPrivate interface --------

class IosDeviceManagerPrivate {
public:
    static IosDeviceManagerPrivate *instance();
    explicit IosDeviceManagerPrivate (IosDeviceManager *q);
    bool watchDevices();
    void requestAppOp(const QString &bundlePath, const QStringList &extraArgs,
                            Ios::IosDeviceManager::AppOp appOp, const QString &deviceId, int timeout);
    void requestDeviceInfo(const QString &deviceId, int timeout);
    QStringList errors();
    void addError(QString errorMsg);
    QString deviceId(AMDeviceRef device);
    void addDevice(AMDeviceRef device);
    void removeDevice(AMDeviceRef device);
    void checkPendingLookups();
    MobileDeviceLib *lib();
    void didTransferApp(const QString &bundlePath, const QString &deviceId,
                        Ios::IosDeviceManager::OpStatus status);
    void didStartApp(const QString &bundlePath, const QString &deviceId,
                     Ios::IosDeviceManager::OpStatus status, ServiceConnRef conn, int gdbFd, DeviceSession *deviceSession);
    void isTransferringApp(const QString &bundlePath, const QString &deviceId, int progress,
                           const QString &info);
    void deviceWithId(QString deviceId, int timeout, DeviceAvailableCallback callback, void *userData);
    int processGdbServer(ServiceConnRef conn);
    void stopGdbServer(ServiceConnRef conn, int phase);
private:
    IosDeviceManager *q;
    QMutex m_sendMutex;
    QHash<QString, AMDeviceRef> m_devices;
    QMultiHash<QString, PendingDeviceLookup *> m_pendingLookups;
    AMDeviceNotificationRef m_notification;
};

class DevInfoSession: public CommandSession {
public:
    DevInfoSession(const QString &deviceId);

    void deviceCallbackReturned();
    QString commandName();
    QString getStringValue(AMDevice *device,
                           CFStringRef domain,
                           CFStringRef key,
                           const QString &fallback = QString());
};

class AppOpSession: public CommandSession {
public:
    QString bundlePath;
    QStringList extraArgs;
    Ios::IosDeviceManager::AppOp appOp;


    AppOpSession(const QString &deviceId, const QString &bundlePath,
                        const QStringList &extraArgs, Ios::IosDeviceManager::AppOp appOp);

    void deviceCallbackReturned() override;
    bool installApp();
    bool runApp();
    int qmljsDebugPort() const override;
    am_res_t appTransferCallback(CFDictionaryRef dict) override;
    am_res_t appInstallCallback(CFDictionaryRef dict) override;
    void reportProgress2(int progress, const QString &status) override;
    QString appPathOnDevice();
    QString appId();
    QString commandName() override;
};

}

DeviceSession::DeviceSession(const QString &deviceId) :
    deviceId(deviceId)
{
}

DeviceSession::~DeviceSession()
{
}

// namespace Internal
} // namespace Ios


namespace {
// ------- callbacks --------

extern "C" void deviceNotificationCallback(Ios::AMDeviceNotificationCallbackInfo *info, void *user)
{
    if (info == 0)
        Ios::Internal::IosDeviceManagerPrivate::instance()->addError(QLatin1String("null info in deviceNotificationCallback"));
    if (debugAll) {
        QDebug dbg=qDebug();
        dbg << "device_notification_callback(";
        if (info)
            dbg << " dev:" << info->_device << " msg:" << info->_message << " subscription:" << info->_subscription;
        else
            dbg << "*NULL*";
        dbg << "," << user << ")";
    }
    switch (info->_message) {
    case ADNCI_MSG_CONNECTED:
        Ios::Internal::IosDeviceManagerPrivate::instance()->addDevice(info->_device);
        break;
    case ADNCI_MSG_DISCONNECTED:
        Ios::Internal::IosDeviceManagerPrivate::instance()->removeDevice(info->_device);
        break;
    case ADNCI_MSG_UNSUBSCRIBED:
        break;
    default:
        Ios::Internal::IosDeviceManagerPrivate::instance()->addError(QLatin1String("unexpected notification message value ")
                                                + QString::number(info->_message));
    }
}

extern "C" void deviceAvailableSessionCallback(QString deviceId, Ios::AMDeviceRef device, void *userData)
{
    if (debugAll)
        qDebug() << "deviceAvailableSessionCallback" << QThread::currentThread();
    if (userData == 0) {
        Ios::Internal::IosDeviceManagerPrivate::instance()->addError(QLatin1String("deviceAvailableSessionCallback called with null userData"));
        return;
    }
    Ios::Internal::CommandSession *session = static_cast<Ios::Internal::CommandSession *>(userData);
    session->internalDeviceAvailableCallback(deviceId, device);
}

extern "C" Ios::am_res_t appTransferSessionCallback(CFDictionaryRef dict, void *userData)
{
    if (debugAll) {
        qDebug() << "appTransferSessionCallback" << QThread::currentThread();
        CFShow(dict);
    }
    if (userData == 0) {
        Ios::Internal::IosDeviceManagerPrivate::instance()->addError(QLatin1String("appTransferSessionCallback called with null userData"));
        return 0; // return -1?
    }
    Ios::Internal::CommandSession *session = static_cast<Ios::Internal::CommandSession *>(userData);
    return session->appTransferCallback(dict);
}


extern "C" mach_error_t appSecureTransferSessionCallback(CFDictionaryRef dict, int arg)
{
    Q_UNUSED(arg)
    CFStringRef cfStatus = reinterpret_cast<CFStringRef>(CFDictionaryGetValue(dict, CFSTR("Status")));
    const QString status = QString::fromCFString(cfStatus);

    quint32 percent = 0;
    CFNumberRef cfProgress;
    if (CFDictionaryGetValueIfPresent(dict, CFSTR("PercentComplete"), reinterpret_cast<const void **>(&cfProgress))) {
        if (cfProgress && CFGetTypeID(cfProgress) == CFNumberGetTypeID())
            CFNumberGetValue(cfProgress, kCFNumberSInt32Type, reinterpret_cast<const void **>(&percent));
    }

    QString path;
    if (status == "CopyingFile") {
        CFStringRef cfPath = reinterpret_cast<CFStringRef>(CFDictionaryGetValue(dict, CFSTR("Path")));
        path = QString::fromCFString(cfPath);
    }

    if (debugAll) {
        qDebug() << "["<<percent<<"]" << status << path;
    } else {
        static QString oldStatus;
        if (oldStatus != status) {
            qDebug() << status;
            oldStatus = status;
        }
    }
    return 0;
}

extern "C" Ios::am_res_t appInstallSessionCallback(CFDictionaryRef dict, void *userData)
{
    if (debugAll) {
        qDebug() << "appInstallSessionCallback" << QThread::currentThread();
        CFShow(dict);
    }
    if (userData == 0) {
        Ios::Internal::IosDeviceManagerPrivate::instance()->addError(QLatin1String("appInstallSessionCallback called with null userData"));
        return 0; // return -1?
    }
    Ios::Internal::CommandSession *session = static_cast<Ios::Internal::CommandSession *>(userData);
    return session->appInstallCallback(dict);
}

} // anonymous namespace

namespace Ios {
namespace Internal {

// ------- IosManagerPrivate implementation --------

IosDeviceManagerPrivate *IosDeviceManagerPrivate::instance()
{
    return IosDeviceManager::instance()->d;
}

IosDeviceManagerPrivate::IosDeviceManagerPrivate (IosDeviceManager *q) : q(q), m_notification(0) { }

bool IosDeviceManagerPrivate::watchDevices()
{
    MobileDeviceLib &mLib = MobileDeviceLib::instance();
    am_res_t e = mLib.deviceNotificationSubscribe(&deviceNotificationCallback, 0, 0,
                                                         0, &m_notification);
    if (e != 0) {
        addError(QLatin1String("AMDeviceNotificationSubscribe failed"));
        return false;
    }
    if (debugAll)
        qDebug() << "AMDeviceNotificationSubscribe successful, m_notificationIter:" << m_notification << QThread::currentThread();

    return true;
}

void IosDeviceManagerPrivate::requestAppOp(const QString &bundlePath,
                                                 const QStringList &extraArgs,
                                                 IosDeviceManager::AppOp appOp,
                                                 const QString &deviceId, int timeout)
{
    AppOpSession *session = new AppOpSession(deviceId, bundlePath, extraArgs, appOp);
    session->startDeviceLookup(timeout);
}

void IosDeviceManagerPrivate::requestDeviceInfo(const QString &deviceId, int timeout)
{
    DevInfoSession *session = new DevInfoSession(deviceId);
    session->startDeviceLookup(timeout);
}

QStringList IosDeviceManagerPrivate::errors()
{
    MobileDeviceLib &mLib = MobileDeviceLib::instance();
    return mLib.errors();
}

void IosDeviceManagerPrivate::addError(QString errorMsg)
{
    if (debugAll)
        qDebug() << "IosManagerPrivate ERROR: " << errorMsg;
    emit q->errorMsg(errorMsg);
}

QString IosDeviceManagerPrivate::deviceId(AMDeviceRef device)
{
    MobileDeviceLib &mLib = MobileDeviceLib::instance();
    CFStringRef s = mLib.deviceCopyDeviceIdentifier(device);
    // remove dashes as a hotfix for QTCREATORBUG-21291
    const auto id = QString::fromCFString(s).remove('-');
    if (s) CFRelease(s);
    return id;
}

void IosDeviceManagerPrivate::addDevice(AMDeviceRef device)
{
    const QString devId = deviceId(device);
    CFRetain(device);

    MobileDeviceLib &mLib = MobileDeviceLib::instance();
    DeviceInterfaceType interfaceType = static_cast<DeviceInterfaceType>(mLib.deviceGetInterfaceType(device));
    if (interfaceType == DeviceInterfaceType::UNKNOWN) {
        if (debugAll)
            qDebug() << "Skipping device." << devId << "Interface type: Unknown.";
        return;
    }

    // Skip the wifi connections as debugging over wifi is not supported.
    if (noWifi && interfaceType == DeviceInterfaceType::WIFI) {
        if (debugAll)
            qDebug() << "Skipping device." << devId << "Interface type: WIFI. Debugging over WIFI is not supported.";
        return;
    }

    if (debugAll)
        qDebug() << "addDevice " << devId;
    if (m_devices.contains(devId)) {
        if (m_devices.value(devId) == device) {
            addError(QLatin1String("double add of device ") + devId);
            return;
        }
        addError(QLatin1String("device change without remove ") + devId);
        removeDevice(m_devices.value(devId));
    }
    m_devices.insert(devId, device);
    emit q->deviceAdded(devId);
    if (m_pendingLookups.contains(devId) || m_pendingLookups.contains(QString())) {
        QList<PendingDeviceLookup *> devices = m_pendingLookups.values(devId);
        m_pendingLookups.remove(devId);
        devices << m_pendingLookups.values(QString());
        m_pendingLookups.remove(QString());
        foreach (PendingDeviceLookup *devLookup, devices) {
            if (debugAll) qDebug() << "found pending op";
            devLookup->timer.stop();
            devLookup->callback(devId, device, devLookup->userData);
            delete devLookup;
        }
    }
}

void IosDeviceManagerPrivate::removeDevice(AMDeviceRef device)
{
    const QString devId = deviceId(device);
    if (debugAll)
        qDebug() << "removeDevice " << devId;
    if (m_devices.contains(devId)) {
        if (m_devices.value(devId) == device) {
            CFRelease(device);
            m_devices.remove(devId);
            emit q->deviceRemoved(devId);
            return;
        }
        addError(QLatin1String("ignoring remove of changed device ") + devId);
    } else {
        addError(QLatin1String("removal of unknown device ") + devId);
    }
}

void IosDeviceManagerPrivate::checkPendingLookups()
{
    foreach (const QString &deviceId, m_pendingLookups.keys()) {
        foreach (PendingDeviceLookup *deviceLookup, m_pendingLookups.values(deviceId)) {
            if (!deviceLookup->timer.isActive()) {
                m_pendingLookups.remove(deviceId, deviceLookup);
                deviceLookup->callback(deviceId, 0, deviceLookup->userData);
                delete deviceLookup;
            }
        }
    }
}

void IosDeviceManagerPrivate::didTransferApp(const QString &bundlePath, const QString &deviceId,
                                       IosDeviceManager::OpStatus status)
{
    emit IosDeviceManagerPrivate::instance()->q->didTransferApp(bundlePath, deviceId, status);
}

void IosDeviceManagerPrivate::didStartApp(const QString &bundlePath, const QString &deviceId,
                                          IosDeviceManager::OpStatus status, ServiceConnRef conn,
                                          int gdbFd, DeviceSession *deviceSession)
{
    emit IosDeviceManagerPrivate::instance()->q->didStartApp(bundlePath, deviceId, status, conn,
                                                             gdbFd, deviceSession);
}

void IosDeviceManagerPrivate::isTransferringApp(const QString &bundlePath, const QString &deviceId,
                                                int progress, const QString &info)
{
    emit IosDeviceManagerPrivate::instance()->q->isTransferringApp(bundlePath, deviceId, progress, info);
}

void IosDeviceManagerPrivate::deviceWithId(QString deviceId, int timeout,
                                     DeviceAvailableCallback callback,
                                     void *userData)
{
    if (!m_notification) {
        qDebug() << "null notification!!";
        /*if (!watchDevices()) {
            callback(deviceId, 0, userData);
            return;
        }*/
    }
    if (deviceId.isEmpty() && !m_devices.isEmpty()) {
        QHash<QString,AMDeviceRef>::iterator i = m_devices.begin();
        callback(i.key(), i.value() , userData);
        return;
    }
    if (m_devices.contains(deviceId)) {
        callback(deviceId, m_devices.value(deviceId), userData);
        return;
    }
    if (timeout <= 0) {
        callback(deviceId, 0, userData);
        return;
    }
    PendingDeviceLookup *pendingLookup = new PendingDeviceLookup;
    pendingLookup->callback = callback;
    pendingLookup->userData = userData;
    pendingLookup->timer.setSingleShot(true);
    pendingLookup->timer.setInterval(timeout);
    QObject::connect(&(pendingLookup->timer), &QTimer::timeout, q, &IosDeviceManager::checkPendingLookups);
    m_pendingLookups.insert(deviceId, pendingLookup);
    pendingLookup->timer.start();
}
enum GdbServerStatus {
    NORMAL_PROCESS,
    PROTOCOL_ERROR,
    STOP_FOR_SIGNAL,
    INFERIOR_EXITED,
    PROTOCOL_UNHANDLED
};

int IosDeviceManagerPrivate::processGdbServer(ServiceConnRef conn)
{
    CommandSession session((QString()));
    {
        QMutexLocker l(&m_sendMutex);
        session.sendGdbCommand(conn, "vCont;c"); // resume all threads
    }
    GdbServerStatus state = NORMAL_PROCESS;
    int maxRetry = 10;
    int maxSignal = 5;
    while (state == NORMAL_PROCESS) {
        QByteArray repl = session.readGdbReply(conn);
        int signal = 0;
        if (repl.size() > 0) {
            state = PROTOCOL_ERROR;
            switch (repl.at(0)) {
            case 'S':
                if (repl.size() < 3) {
                    addError(QString::fromLatin1("invalid S signal message %1")
                             .arg(QString::fromLatin1(repl.constData(), repl.size())));
                    state = PROTOCOL_ERROR;
                } else {
                    signal = QByteArray::fromHex(repl.mid(1,2)).at(0);
                    addError(QString::fromLatin1("program received signal %1")
                             .arg(signal));
                    state = STOP_FOR_SIGNAL;
                }
                break;
            case 'T':
                if (repl.size() < 3) {
                    addError(QString::fromLatin1("invalid T signal message %1")
                             .arg(QString::fromLatin1(repl.constData(), repl.size())));
                    state = PROTOCOL_ERROR;
                } else {
                    signal = QByteArray::fromHex(repl.mid(1,2)).at(0);
                    addError(QString::fromLatin1("program received signal %1, %2")
                             .arg(signal)
                             .arg(QString::fromLatin1(repl.mid(3,repl.size()-3))));
                    state = STOP_FOR_SIGNAL;
                }
                break;
            case 'W':
                if (repl.size() < 3) {
                    addError(QString::fromLatin1("invalid W signal message %1")
                             .arg(QString::fromLatin1(repl.constData(), repl.size())));
                    state = PROTOCOL_ERROR;
                } else {
                    int exitCode = QByteArray::fromHex(repl.mid(1,2)).at(0);
                    addError(QString::fromLatin1("exited with exit code %1, %2")
                             .arg(exitCode)
                             .arg(QString::fromLatin1(repl.mid(3,repl.size()-3))));
                    state = INFERIOR_EXITED;
                }
                break;
            case 'X':
                if (repl.size() < 3) {
                    addError(QString::fromLatin1("invalid X signal message %1")
                             .arg(QString::fromLatin1(repl.constData(), repl.size())));
                    state = PROTOCOL_ERROR;
                } else {
                    int exitCode = QByteArray::fromHex(repl.mid(1,2)).at(0);
                    addError(QString::fromLatin1("exited due to signal %1, %2")
                             .arg(exitCode)
                             .arg(QString::fromLatin1(repl.mid(3,repl.size()-3))));
                    state = INFERIOR_EXITED;
                }
                break;
            case 'O':
                emit q->appOutput(QString::fromLocal8Bit(QByteArray::fromHex(repl.mid(1))));
                state = NORMAL_PROCESS;
                break;
            default:
                addError(QString::fromLatin1("non handled response:")
                         + QString::fromLatin1(repl.constData(), repl.size()));
                state = PROTOCOL_UNHANDLED;
            }
            if (state == STOP_FOR_SIGNAL) {
                QList<int> okSig = QList<int>() << SIGCHLD << SIGCONT << SIGALRM << SIGURG
                                                << SIGUSR1 << SIGUSR2 << SIGPIPE
                                                << SIGPROF << SIGWINCH << SIGINFO;
                if (signal == 9) {
                    break;
                } else if (!okSig.contains(signal) && --maxSignal < 0) {
                    addError(QLatin1String("hit maximum number of consecutive signals, stopping"));
                    break;
                }
                {
                    if (signal == 17) {
                        state = NORMAL_PROCESS; // Ctrl-C to kill the process
                    } else {
                        QMutexLocker l(&m_sendMutex);
                        if (session.sendGdbCommand(conn, "vCont;c"))
                            state = NORMAL_PROCESS;
                        else
                            break;
                    }
                }
            } else {
                maxSignal = 5;
            }
            maxRetry = 10;
        } else {
            if (--maxRetry < 0)
                break;
        }
    }
    return state != INFERIOR_EXITED;
}

void IosDeviceManagerPrivate::stopGdbServer(ServiceConnRef conn, int phase)
{
    CommandSession session((QString()));
    QMutexLocker l(&m_sendMutex);
    MobileDeviceLib &mLib = MobileDeviceLib::instance();
    if (phase == 0)
        mLib.serviceConnectionSend(conn, "\x03", 1);
    else
        session.sendGdbCommand(conn, "k", 1);
}

// ------- ConnectSession implementation --------

CommandSession::CommandSession(const QString &deviceId) : DeviceSession(deviceId), device(0),
    progressBase(0), unexpectedChars(0), aknowledge(true)
{ }

CommandSession::~CommandSession() { }

bool CommandSession::connectDevice()
{
    if (!device)
        return false;

    MobileDeviceLib &mLib = MobileDeviceLib::instance();
    if (am_res_t error1 = mLib.deviceConnect(device)) {
        addError(QString::fromLatin1("connectDevice %1 failed, AMDeviceConnect returned %2 (0x%3)")
                 .arg(deviceId).arg(mobileDeviceErrorString(error1)).arg(error1));
        return false;
    }
    if (mLib.deviceIsPaired(device) == 0) { // not paired
        if (am_res_t error = mLib.devicePair(device)) {
            addError(QString::fromLatin1("connectDevice %1 failed, AMDevicePair returned %2 (0x%3)")
                     .arg(deviceId).arg(mobileDeviceErrorString(error)).arg(error));
            return false;
        }
    }
    if (am_res_t error2 = mLib.deviceValidatePairing(device)) {
        addError(QString::fromLatin1("connectDevice %1 failed, AMDeviceValidatePairing returned %2 (0x%3)")
                 .arg(deviceId).arg(mobileDeviceErrorString(error2)).arg(error2));
        return false;
    }
    if (am_res_t error3 = mLib.deviceStartSession(device)) {
        addError(QString::fromLatin1("connectDevice %1 failed, AMDeviceStartSession returned %2 (0x%3)")
                 .arg(deviceId).arg(mobileDeviceErrorString(error3)).arg(error3));
        return false;
    }
    return true;
}

bool CommandSession::disconnectDevice()
{
    MobileDeviceLib &mLib = MobileDeviceLib::instance();
    if (am_res_t error = mLib.deviceStopSession(device)) {
        addError(QString::fromLatin1("stopSession %1 failed, AMDeviceStopSession returned %2 (0x%3)")
                 .arg(deviceId).arg(mobileDeviceErrorString(error)).arg(error));
        return false;
    }
    if (am_res_t error = mLib.deviceDisconnect(device)) {
        addError(QString::fromLatin1("disconnectDevice %1 failed, AMDeviceDisconnect returned %2 (0x%3)")
                          .arg(deviceId).arg(mobileDeviceErrorString(error)).arg(error));
        return false;
    }
    return true;
}

bool CommandSession::startServiceSecure(const QString &serviceName, ServiceConnRef &conn)
{
    bool success = true;

    // Connect device. AMDeviceConnect + AMDeviceIsPaired + AMDeviceValidatePairing + AMDeviceStartSession
    if (connectDevice()) {
        CFStringRef cfsService = serviceName.toCFString();
        MobileDeviceLib &mLib = MobileDeviceLib::instance();
        if (am_res_t error = mLib.deviceSecureStartService(device, cfsService, &conn)) {
            addError(QString::fromLatin1("Starting(Secure) service \"%1\" on device %2 failed, AMDeviceStartSecureService returned %3 (0x%4)")
                     .arg(serviceName).arg(deviceId).arg(mobileDeviceErrorString(error)).arg(QString::number(error, 16)));
            success = false;
        } else {
            if (!conn) {
                addError(QString("Starting(Secure) service \"%1\" on device %2 failed."
                         "Invalid service connection").arg(serviceName).arg(deviceId));
            }
        }
        disconnectDevice();
        CFRelease(cfsService);
    } else {
        addError(QString::fromLatin1("Starting(Secure) service \"%1\" on device %2 failed. Cannot connect to device.")
                 .arg(serviceName).arg(deviceId));
        success = false;
    }
    return success;
}

bool CommandSession::connectToPort(quint16 port, ServiceSocket *fd)
{
    if (!fd)
        return false;
    bool failure = false;
    *fd = 0;
    ServiceSocket fileDescriptor;
    if (!connectDevice())
        return false;

    MobileDeviceLib &mLib = MobileDeviceLib::instance();
    if (am_res_t error = mLib.connectByPort(mLib.deviceGetConnectionID(device), htons(port), &fileDescriptor)) {
        addError(QString::fromLatin1("connectByPort on device %1 port %2 failed, AMDeviceStartService returned %3")
                 .arg(deviceId).arg(port).arg(error));
        failure = true;
        *fd = -1;
    } else {
        *fd = fileDescriptor;
    }
    disconnectDevice();
    return !failure;
}

int CommandSession::qmljsDebugPort() const
{
    return 0;
}

void CommandSession::stopService(ServiceSocket fd)
{
    // would be close socket on windows
    close(fd);
}

void CommandSession::startDeviceLookup(int timeout)
{
    IosDeviceManagerPrivate::instance()->deviceWithId(deviceId, timeout,
                                                &deviceAvailableSessionCallback, this);
}

void CommandSession::addError(const QString &msg)
{
    if (verbose)
        qDebug() << "CommandSession ERROR: " << msg;
    IosDeviceManagerPrivate::instance()->addError(commandName() + msg);
}

bool CommandSession::writeAll(ServiceSocket fd, const char *cmd, qptrdiff len)
{
    if (len == -1)
        len = strlen(cmd);
    if (debugGdbServer) {
        QByteArray cmdBA(cmd,len);
        qDebug() << "writeAll(" << fd << "," << QString::fromLocal8Bit(cmdBA.constData(), cmdBA.size())
                 << " (" << cmdBA.toHex() << "))";
    }
    qptrdiff i = 0;
    int maxRetry = 10;
    while (i < len) {
        qptrdiff nWritten = write(fd, cmd, len - i);
        if (nWritten < 1) {
            --maxRetry;
            if (nWritten == -1 && errno != 0 && errno != EINTR) {
                char buf[256];
                if (!strerror_r(errno, buf, sizeof(buf))) {
                    buf[sizeof(buf)-1] = 0;
                    addError(QString::fromLocal8Bit(buf));
                } else {
                    addError(QLatin1String("Unknown writeAll error"));
                }
                return false;
            }
            if (maxRetry <= 0) {
                addError(QLatin1String("Hit maximum retries in writeAll"));
                return false;
            }
        } else {
            maxRetry = 10;
        }
        i += nWritten;
    }
    return true;
}

void mountCallback(CFDictionaryRef dict, int arg)
{
    Q_UNUSED(arg)
    CFStringRef cfStatus = reinterpret_cast<CFStringRef>(CFDictionaryGetValue(dict, CFSTR("Status")));
    qDebug() << "Mounting dev Image :"<<QString::fromCFString(cfStatus);
}

bool CommandSession::mountDeveloperDiskImage() {
    bool success = false;
    QString imagePath;
    QString signaturePath;
    if (developerDiskImagePath(&imagePath, &signaturePath)) {
        QFile sigFile(signaturePath);
        if (sigFile.open(QFile::ReadOnly)) {
            // Read the signature.
            const QByteArray signatureData = sigFile.read(128);
            sigFile.close();

            CFDataRef sig_data = signatureData.toRawCFData();
            CFTypeRef keys[] = { CFSTR("ImageSignature"), CFSTR("ImageType") };
            CFTypeRef values[] = { sig_data, CFSTR("Developer") };
            CFDictionaryRef options = CFDictionaryCreate(NULL, (const void **)&keys, (const void **)&values,
                                                         2, &kCFTypeDictionaryKeyCallBacks,
                                                         &kCFTypeDictionaryValueCallBacks);

            if (connectDevice()) {
                CFStringRef cfImgPath = imagePath.toCFString();
                MobileDeviceLib &mLib = MobileDeviceLib::instance();
                am_res_t result = mLib.deviceMountImage(device, cfImgPath, options, &mountCallback, 0);
                if (result == 0 || result == kAMDMobileImageMounterImageMountFailed)  {
                    // Mounting succeeded or developer image already installed
                    success = true;
                } else {
                    addError(QString::fromLatin1("Mount Developer Disk Image \"%1\" failed, AMDeviceMountImage returned %2 (0x%3)")
                             .arg(imagePath).arg(mobileDeviceErrorString(result)).arg(QString::number(result, 16)));
                }
                CFRelease(cfImgPath);
                disconnectDevice();
            } else
                addError(QString::fromLatin1("Mount Developer Disk Image \"%1\" failed. Cannot connect to device \"%2\".")
                         .arg(imagePath).arg(deviceId));
        } else {
            addError(QString::fromLatin1("Mount Developer Disk Image \"%1\" failed. Unable to open disk image.")
                     .arg(imagePath));
        }
    } else {
        addError(QString::fromLatin1("Mount Developer Disk Image failed. Unable to fetch developer disk image path."));
    }
    return success;
}

bool CommandSession::sendGdbCommand(ServiceConnRef conn, const char *cmd, qptrdiff len)
{
    if (len == -1)
        len = strlen(cmd);
    unsigned char checkSum = 0;
    for (int i = 0; i < len; ++i)
        checkSum += static_cast<unsigned char>(cmd[i]);

    MobileDeviceLib &mLib = MobileDeviceLib::instance();
    bool failure = mLib.serviceConnectionSend(conn, "$", 1) == 0;
    if (!failure)
        failure = mLib.serviceConnectionSend(conn, cmd, len) == 0;
    char buf[3];
    buf[0] = '#';
    const char *hex = "0123456789abcdef";
    buf[1]   = hex[(checkSum >> 4) & 0xF];
    buf[2] = hex[checkSum & 0xF];
    if (!failure)
        failure = mLib.serviceConnectionSend(conn, buf, 3) == 0;
    return !failure;
}

bool CommandSession::checkRead(qptrdiff nRead, int &maxRetry)
{
    if (nRead < 1 || nRead > 4) {
        --maxRetry;
        if ((nRead < 0 || nRead > 4) && errno != 0 && errno != EINTR) {
            char buf[256];
            if (!strerror_r(errno, buf, sizeof(buf))) {
                buf[sizeof(buf)-1] = 0;
                addError(QString::fromLocal8Bit(buf));
            } else {
                addError(QLatin1String("Unknown writeAll error"));
            }
            return false;
        }
        if (maxRetry <= 0) {
            addError(QLatin1String("Hit maximum retries in readGdbReply"));
            return false;
        }
    } else {
        maxRetry = 10;
    }
    return true;
}

int CommandSession::handleChar(ServiceConnRef conn, QByteArray &res, char c, int status)
{
    switch (status) {
    case 0:
        if (c == '$')
            return 1;
        if (c != '+' && c != '-') {
            if (unexpectedChars < 10) {
                addError(QString::fromLatin1("unexpected char %1 in readGdbReply looking for $")
                         .arg(QChar::fromLatin1(c)));
                ++unexpectedChars;
            } else if (unexpectedChars == 10) {
                addError(QString::fromLatin1("hit maximum number of unexpected chars, ignoring them in readGdbReply looking for $"));
                ++unexpectedChars;
            }
        }
        return 0;
    case 1:
        if (c != '#') {
            res.append(c);
            return 1;
        }
        return 2;
    case 2:
    case 3:
        if ((c < '0' || c > '9') && (c < 'a' || c > 'f') && (c < 'A' || c > 'F')) {
            if (unexpectedChars < 15) {
                addError(QString::fromLatin1("unexpected char %1 in readGdbReply as checksum")
                         .arg(QChar::fromLatin1(c)));
                ++unexpectedChars;
            } else if (unexpectedChars == 15) {
                addError(QString::fromLatin1("hit maximum number of unexpected chars in checksum, ignoring them in readGdbReply"));
                ++unexpectedChars;
            }
        }
        if (status == 3 && aknowledge) {
            MobileDeviceLib &mLib = MobileDeviceLib::instance();
            mLib.serviceConnectionSend(conn, "+", 1);
        }
        return status + 1;
    case 4:
        addError(QString::fromLatin1("gone past end in readGdbReply"));
        return 5;
    case 5:
        return 5;
    default:
        addError(QString::fromLatin1("unexpected status readGdbReply"));
        return 5;
    }
}

QByteArray CommandSession::readGdbReply(ServiceConnRef conn)
{
    // read with minimal buffering because we might want to give the socket away...
    QByteArray res;
    char buf[5];
    int maxRetry = 10;
    int status = 0;
    int toRead = 4;
    MobileDeviceLib &mLib = MobileDeviceLib::instance();
    while (status < 4 && toRead > 0) {
        qptrdiff nRead = mLib.serviceConnectionReceive(conn, buf, toRead);
        if (!checkRead(nRead, maxRetry))
            return QByteArray();
        if (debugGdbServer) {
            buf[nRead] = 0;
            qDebug() << "gdbReply read " << buf;
        }
        for (qptrdiff i = 0; i< nRead; ++i)
            status = handleChar(conn, res, buf[i], status);
        toRead = 4 - status;
    }
    if (status != 4) {
        addError(QString::fromLatin1("unexpected parser status %1 in readGdbReply").arg(status));
        return QByteArray();
    }
    return res;
}

void CommandSession::reportProgress(CFDictionaryRef dict)
{
    QString status;
    CFStringRef cfStatus;
    if (CFDictionaryGetValueIfPresent(dict, CFSTR("Status"), reinterpret_cast<const void **>(&cfStatus))) {
        if (cfStatus && CFGetTypeID(cfStatus) == CFStringGetTypeID())
            status = QString::fromCFString(cfStatus);
    }
    quint32 progress = 0;
    CFNumberRef cfProgress;
    if (CFDictionaryGetValueIfPresent(dict, CFSTR("PercentComplete"), reinterpret_cast<const void **>(&cfProgress))) {
        if (cfProgress && CFGetTypeID(cfProgress) == CFNumberGetTypeID())
            CFNumberGetValue(cfProgress,kCFNumberSInt32Type, reinterpret_cast<const void **>(&progress));
    }
    reportProgress2(progressBase + progress, status);
}

void CommandSession::reportProgress2(int progress, const QString &status)
{
    Q_UNUSED(progress)
    Q_UNUSED(status)
}

QString CommandSession::commandName()
{
    return QString::fromLatin1("CommandSession(%1)").arg(deviceId);
}

bool CommandSession::expectGdbReply(ServiceConnRef conn, QByteArray expected)
{
    QByteArray repl = readGdbReply(conn);
    if (repl != expected) {
        addError(QString::fromLatin1("Unexpected reply: %1 (%2) vs %3 (%4)")
                 .arg(QString::fromLocal8Bit(repl.constData(), repl.size()))
                 .arg(QString::fromLatin1(repl.toHex().constData(), 2*repl.size()))
                 .arg(QString::fromLocal8Bit(expected.constData(), expected.size()))
                 .arg(QString::fromLocal8Bit(expected.toHex().constData(), 2*expected.size())));
        return false;
    }
    return true;
}

bool CommandSession::expectGdbOkReply(ServiceConnRef conn)
{
    return expectGdbReply(conn, QByteArray("OK"));
}

bool CommandSession::developerDiskImagePath(QString *path, QString *signaturePath)
{
    if (device && path && connectDevice()) {
        MobileDeviceLib &mLib = MobileDeviceLib::instance();
        CFPropertyListRef cfProductVersion = mLib.deviceCopyValue(device, 0, CFSTR("ProductVersion"));
        QString versionString;
        if (cfProductVersion && CFGetTypeID(cfProductVersion) == CFStringGetTypeID()) {
            versionString = QString::fromCFString(reinterpret_cast<CFStringRef>(cfProductVersion));
        }
        CFRelease(cfProductVersion);

        CFPropertyListRef cfBuildVersion = mLib.deviceCopyValue(device, 0, CFSTR("BuildVersion"));
        QString buildString;
        if (cfBuildVersion && CFGetTypeID(cfBuildVersion) == CFStringGetTypeID()) {
            buildString = QString::fromCFString(reinterpret_cast<CFStringRef>(cfBuildVersion));
        }
        CFRelease(cfBuildVersion);
        disconnectDevice();

        if (findDeveloperDiskImage(versionString, buildString, path)) {
            if (debugAll)
                qDebug() << "Developers disk image found at" << path;
            if (signaturePath) {
                *signaturePath = QString("%1.%2").arg(*path).arg("signature");
                return QFile::exists(*signaturePath);
            }
            return true;
        }
    }
    return false;
}

AppOpSession::AppOpSession(const QString &deviceId, const QString &bundlePath,
                                       const QStringList &extraArgs, IosDeviceManager::AppOp appOp):
    CommandSession(deviceId), bundlePath(bundlePath), extraArgs(extraArgs), appOp(appOp)
{ }

QString AppOpSession::commandName()
{
    return QString::fromLatin1("TransferAppSession(%1, %2)").arg(deviceId, bundlePath);
}

bool AppOpSession::installApp()
{
    bool success = false;
    if (device != 0) {
        CFURLRef bundleUrl = QUrl::fromLocalFile(bundlePath).toCFURL();
        CFStringRef key[1] = {CFSTR("PackageType")};
        CFStringRef value[1] = {CFSTR("Developer")};
        CFDictionaryRef options = CFDictionaryCreate(0, reinterpret_cast<const void**>(&key[0]),
                reinterpret_cast<const void**>(&value[0]), 1,
                                                     &kCFTypeDictionaryKeyCallBacks,
                                                     &kCFTypeDictionaryValueCallBacks);

        MobileDeviceLib &mLib = MobileDeviceLib::instance();
        // Transfer bundle with secure API AMDeviceTransferApplication.
        if (int error = mLib.deviceSecureTransferApplicationPath(0, device, bundleUrl, options,
                                                         &appSecureTransferSessionCallback,0)) {
            addError(QString::fromLatin1("TransferAppSession(%1,%2) failed, AMDeviceTransferApplication returned %3 (0x%4)")
                     .arg(bundlePath, deviceId).arg(mobileDeviceErrorString(error)).arg(error));
            success = false;
        } else {
            // App is transferred. Try installing.
            if (connectDevice()) {
                // Secure install app api requires device to be connected.
                if (am_res_t error = mLib.deviceSecureInstallApplication(0, device, bundleUrl, options,
                                                                           &appSecureTransferSessionCallback,0)) {
                    const QString errorString = mobileDeviceErrorString(error);
                    if (!errorString.isEmpty()) {
                        addError(errorString
                                 + QStringLiteral(" (0x")
                                 + QString::number(error, 16)
                                 + QStringLiteral(")"));
                    } else {
                        addError(QString::fromLatin1("InstallAppSession(%1,%2) failed, "
                                                     "AMDeviceInstallApplication returned 0x%3")
                                 .arg(bundlePath, deviceId).arg(QString::number(error, 16)));
                    }
                    success = false;
                } else {
                    // App is installed.
                    success = true;
                }
                disconnectDevice();
            }
        }

        if (debugAll) {
            qDebug() << "AMDeviceSecureTransferApplication finished request with " << (success ? "Success" : "Failure");
        }

        CFRelease(options);
        CFRelease(bundleUrl);

        progressBase += 100;
    }


    if (success) {
        sleep(5); // after installation the device needs a bit of quiet....
    }

    if (debugAll) {
        qDebug() << "AMDeviceSecureInstallApplication finished request with " << (success ? "Success" : "Failure");
    }

    IosDeviceManagerPrivate::instance()->didTransferApp(bundlePath, deviceId,
                (success ? IosDeviceManager::Success : IosDeviceManager::Failure));
    return success;
}

void AppOpSession::deviceCallbackReturned()
{
    switch (appOp) {
    case IosDeviceManager::None:
        break;
    case IosDeviceManager::Install:
        installApp();
        break;
    case IosDeviceManager::InstallAndRun:
        if (installApp())
            runApp();
        break;
    case IosDeviceManager::Run:
        runApp();
        break;
    }
}

int AppOpSession::qmljsDebugPort() const
{
    const QRegularExpression qmlPortRe(QLatin1String("-qmljsdebugger=port:([0-9]+)"));
    for (const QString &arg : qAsConst(extraArgs)) {
        const QRegularExpressionMatch match = qmlPortRe.match(arg);
        if (match.hasMatch()) {
            bool ok;
            int res = match.captured(1).toInt(&ok);
            if (ok && res >0 && res <= 0xFFFF)
                return res;
        }
    }
    return 0;
}


bool AppOpSession::runApp()
{
    bool failure = (device == 0);
    QString exe = appPathOnDevice();
    if (!mountDeveloperDiskImage()) {
        addError(QString::fromLatin1("Running app \"%1\" failed. Mount developer disk failed.").arg(bundlePath));
        failure = true;
    }

    CFStringRef keys[] = { CFSTR("MinIPhoneVersion"), CFSTR("MinAppleTVVersion") };
    CFStringRef values[] = { CFSTR("14.0"), CFSTR("14.0")};
    CFDictionaryRef version = CFDictionaryCreate(NULL, (const void **)&keys, (const void **)&values,
                                                 2, &kCFTypeDictionaryKeyCallBacks,
                                                 &kCFTypeDictionaryValueCallBacks);
    MobileDeviceLib &mLib = MobileDeviceLib::instance();
    bool useSecureProxy = mLib.deviceIsAtLeastVersionOnPlatform(device, version);
    // The debugserver service cannot be launched directly on iOS 14+
    // A secure proxy service sits between the actual debugserver service.
    const QString &serviceName = useSecureProxy ? DebugSecureServiceName : DebugServiceName;
    ServiceConnRef conn = nullptr;
    if (!failure)
        startServiceSecure(serviceName, conn);

    if (conn) {
        // For older devices remove the ssl encryption as we can directly connecting with the
        // debugserver service, i.e. not the secure proxy service.
        if (!useSecureProxy)
            disable_ssl(conn);
        // gdbServer protocol, see http://sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html#Remote-Protocol
        // and the lldb handling of that (with apple specific stuff)
        // http://llvm.org/viewvc/llvm-project/lldb/trunk/tools/debugserver/source/RNBRemote.cpp
        //failure = !sendGdbCommand(gdbFd, "QStartNoAckMode"); // avoid and send required aknowledgements?
        //if (!failure) failure = !expectGdbOkReply(gdbFd);

        // send the environment with a series of these commands...
        if (!failure) failure = !sendGdbCommand(conn, "QEnvironmentHexEncoded:");
        // avoid address randomization to debug
        if (!failure) failure = !sendGdbCommand(conn, "QSetDisableASLR:1");
        if (!failure) failure = !expectGdbOkReply(conn);
        QStringList args = extraArgs;
        QByteArray runCommand("A");
        args.insert(0, exe);
        if (debugAll)
            qDebug() << " trying to start " << args;
        for (int iarg = 0; iarg < args.size(); ++iarg) {
            if (iarg)
                runCommand.append(',');
            QByteArray arg = args.at(iarg).toLocal8Bit().toHex();
            runCommand.append(QString::number(arg.size()).toLocal8Bit());
            runCommand.append(',');
            runCommand.append(QString::number(iarg).toLocal8Bit());
            runCommand.append(',');
            runCommand.append(arg);
        }
        if (!failure) failure = !sendGdbCommand(conn, runCommand.constData(), runCommand.size());
        if (!failure) failure = !expectGdbOkReply(conn);
        if (!failure) failure = !sendGdbCommand(conn, "qLaunchSuccess");
        if (!failure) failure = !expectGdbOkReply(conn);
    } else {
        failure = true;
    }

    CFSocketNativeHandle fd = failure ? 0 : mLib.deviceConnectionGetSocket(conn);
    auto status = failure ? IosDeviceManager::Failure : IosDeviceManager::Success;
    IosDeviceManagerPrivate::instance()->didStartApp(bundlePath, deviceId, status, conn, fd, this);
    return !failure;
}

void AppOpSession::reportProgress2(int progress, const QString &status)
{
    IosDeviceManagerPrivate::instance()->isTransferringApp(
                bundlePath, deviceId, progress, status);
}

QString AppOpSession::appId()
{
    QSettings settings(bundlePath + QLatin1String("/Info.plist"), QSettings::NativeFormat);
    QString res = settings.value(QString::fromLatin1("CFBundleIdentifier")).toString();
    if (debugAll)
        qDebug() << "appId:" << res;
    return res;
}

QString AppOpSession::appPathOnDevice()
{
    QString res;
    if (!connectDevice())
        return QString();
    CFDictionaryRef apps;
    CFDictionaryRef options;
    const void *attributes[3] = { (const void*)(CFSTR("CFBundleIdentifier")),
                                  (const void*)(CFSTR("Path")), (const void*)(CFSTR("CFBundleExecutable")) };
    CFArrayRef lookupKeys = CFArrayCreate(kCFAllocatorDefault, (const void**)(&attributes[0]), 3,
            &kCFTypeArrayCallBacks);
    CFStringRef attrKey = CFSTR("ReturnAttributes");
    options = CFDictionaryCreate(kCFAllocatorDefault, (const void**)(&attrKey),
                                 (const void**)(&lookupKeys), 1,
                                 &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFRelease(lookupKeys);
    MobileDeviceLib &mLib = MobileDeviceLib::instance();
    if (int err = mLib.deviceLookupApplications(device, options, &apps)) {
        addError(QString::fromLatin1("app lookup failed, AMDeviceLookupApplications returned %1")
                 .arg(err));
    }
    CFRelease(options);
    if (debugAll)
        CFShow(apps);
    if (apps && CFGetTypeID(apps) == CFDictionaryGetTypeID()) {
        CFStringRef cfAppId = appId().toCFString();
        CFDictionaryRef cfAppInfo = 0;
        if (CFDictionaryGetValueIfPresent(apps, cfAppId, reinterpret_cast<const void**>(&cfAppInfo))) {
            if (cfAppInfo && CFGetTypeID(cfAppInfo) == CFDictionaryGetTypeID()) {
                CFStringRef cfPath, cfBundleExe;
                QString path, bundleExe;
                if (CFDictionaryGetValueIfPresent(cfAppInfo, CFSTR("Path"), reinterpret_cast<const void **>(&cfPath)))
                    path = QString::fromCFString(cfPath);
                if (CFDictionaryGetValueIfPresent(cfAppInfo, CFSTR("CFBundleExecutable"), reinterpret_cast<const void **>(&cfBundleExe)))
                    bundleExe = QString::fromCFString(cfBundleExe);
                if (!path.isEmpty() && ! bundleExe.isEmpty())
                    res = path + QLatin1Char('/') + bundleExe;
            }
        }
    }
    if (apps)
        CFRelease(apps);
    disconnectDevice();
    if (res.isEmpty())
        addError(QString::fromLatin1("failed to get app Path on device for bundle %1 with appId: %2")
                 .arg(bundlePath, appId()));
    return res;
}

am_res_t AppOpSession::appTransferCallback(CFDictionaryRef dict)
{
    if (debugAll)
        qDebug() << "TransferAppSession::appTransferCallback";
    reportProgress(dict);
    return 0;
}

am_res_t AppOpSession::appInstallCallback(CFDictionaryRef dict)
{
    if (debugAll)
        qDebug() << "TransferAppSession::appInstallCallback";
    reportProgress(dict);
    return 0;
}

DevInfoSession::DevInfoSession(const QString &deviceId) : CommandSession(deviceId)
{ }

QString DevInfoSession::commandName()
{
    return QString::fromLatin1("DevInfoSession(%1, %2)").arg(deviceId);
}

QString DevInfoSession::getStringValue(AMDevice *device,
                                       CFStringRef domain,
                                       CFStringRef key,
                                       const QString &fallback)
{
    QString value = fallback;
    MobileDeviceLib &mLib = MobileDeviceLib::instance();
    CFPropertyListRef cfValue = mLib.deviceCopyValue(device, domain, key);
    if (cfValue) {
        if (CFGetTypeID(cfValue) == CFStringGetTypeID())
            value = QString::fromCFString(reinterpret_cast<CFStringRef>(cfValue));
        CFRelease(cfValue);
    }
    return value;
}

void DevInfoSession::deviceCallbackReturned()
{
    if (debugAll)
        qDebug() << "device available";
    QMap<QString,QString> res;
    const QString deviceNameKey = "deviceName";
    const QString developerStatusKey = "developerStatus";
    const QString deviceConnectedKey = "deviceConnected";
    const QString osVersionKey = "osVersion";
    const QString cpuArchitectureKey = "cpuArchitecture";
    const QString uniqueDeviceId = "uniqueDeviceId";
    bool failure = !device;
    if (!failure) {
        failure = !connectDevice();
        if (!failure) {
            res[deviceConnectedKey] = QLatin1String("YES");
            res[deviceNameKey] = getStringValue(device, nullptr, CFSTR("DeviceName"));
            res[developerStatusKey] = getStringValue(device,
                                                     CFSTR("com.apple.xcode.developerdomain"),
                                                     CFSTR("DeveloperStatus"),
                                                     "*off*");
            res[cpuArchitectureKey] = getStringValue(device, nullptr, CFSTR("CPUArchitecture"));
            res[uniqueDeviceId] = getStringValue(device, nullptr, CFSTR("UniqueDeviceID"));
            const QString productVersion = getStringValue(device, nullptr, CFSTR("ProductVersion"));
            const QString buildVersion = getStringValue(device, nullptr, CFSTR("BuildVersion"));
            if (!productVersion.isEmpty() && !buildVersion.isEmpty())
                res[osVersionKey] = QString("%1 (%2)").arg(productVersion, buildVersion);
            else if (!productVersion.isEmpty())
                res[osVersionKey] = productVersion;
            else
                res[osVersionKey] = "*unknown*";
        }
        disconnectDevice();
    }
    if (!res.contains(deviceConnectedKey))
        res[deviceConnectedKey] = QLatin1String("NO");
    if (!res.contains(deviceNameKey))
        res[deviceNameKey] = QLatin1String("*unknown*");
    if (!res.contains(developerStatusKey))
        res[developerStatusKey] = QLatin1String("*unknown*");
    if (debugAll)
        qDebug() << "deviceInfo:" << res << ", failure:" << failure;
    emit Ios::IosDeviceManager::instance()->deviceInfo(deviceId, res);
    /* should we also check the provision profiles??? i.e.
    int fd;
    startService(QLatin1String("com.apple.misagent"), &fd);
    ... MISAgentCopyProvisioningProfiles, AMAuthInstallProvisioningGetProvisionedInfo & co still to add */
}

// ------- MobileDeviceLib implementation --------



void CommandSession::internalDeviceAvailableCallback(QString deviceId, AMDeviceRef device)
{
    if (deviceId != this->deviceId && !this->deviceId.isEmpty())
        addError(QString::fromLatin1("deviceId mismatch in deviceAvailableCallback, %1 vs %2")
                  .arg(deviceId, this->deviceId));
    this->deviceId = deviceId;
    if (this->device)
        addError(QString::fromLatin1("session had non null device in deviceAvailableCallback"));
    this->device = device;
    deviceCallbackReturned();
}

} // namespace Internal

// ------- IosManager implementation (just forwarding) --------

IosDeviceManager *IosDeviceManager::instance()
{
    static IosDeviceManager instanceVal;
    return &instanceVal;
}

IosDeviceManager::IosDeviceManager(QObject *parent) :
    QObject(parent)
{
    d = new Internal::IosDeviceManagerPrivate(this);
}

bool IosDeviceManager::watchDevices() {
    return d->watchDevices();
}

void IosDeviceManager::requestAppOp(const QString &bundlePath, const QStringList &extraArgs,
                                          AppOp appOp, const QString &deviceId, int timeout) {
    d->requestAppOp(bundlePath, extraArgs, appOp, deviceId, timeout);
}

void IosDeviceManager::requestDeviceInfo(const QString &deviceId, int timeout)
{
    d->requestDeviceInfo(deviceId, timeout);
}

int IosDeviceManager::processGdbServer(ServiceConnRef conn)
{
    return d->processGdbServer(conn);
}

void IosDeviceManager::stopGdbServer(ServiceConnRef conn, int phase)
{
    return d->stopGdbServer(conn, phase);
}

QStringList IosDeviceManager::errors() {
    return d->errors();
}

void IosDeviceManager::checkPendingLookups()
{
    d->checkPendingLookups();
}

} // namespace Ios
