/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "iosdevicemanager.h"
#include <QLibrary>
#include <QDebug>
#include <QStringList>
#include <QString>
#include <QHash>
#include <QMultiHash>
#include <QTimer>
#include <QThread>
#include <QSettings>
#include <QRegExp>
#include <QUrl>
#include <mach/error.h>

/* // annoying to import, do without
#include <QtCore/private/qcore_mac_p.h>
*/
/* standard calling convention under Win32 is __stdcall */
/* Note: When compiling Intel EFI (Extensible Firmware Interface) under MS Visual Studio, the */
/* _WIN32 symbol is defined by the compiler even though it's NOT compiling code for Windows32 */
#if defined(_WIN32) && !defined(EFI32) && !defined(EFI64)
#define MDEV_API __stdcall
#else
#define MDEV_API
#endif

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFRunLoop.h>

#ifdef MOBILE_DEV_DIRECT_LINK
#include "MobileDevice.h"
#endif

static const bool debugGdbServer = false;
static const bool debugAll = false;
static const bool verbose = true;

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

extern "C" {
typedef unsigned int ServiceSocket; // match_port_t (i.e. natural_t) or socket (on windows, i.e sock_t)
typedef unsigned int am_res_t; // mach_error_t

#ifndef MOBILE_DEV_DIRECT_LINK
class AMDeviceNotification;
typedef const AMDeviceNotification *AMDeviceNotificationRef;
class AMDevice;

struct AMDeviceNotificationCallbackInfo {
    AMDevice *_device;
    unsigned int _message;
    AMDeviceNotification *_subscription;
};
typedef void (MDEV_API *AMDeviceNotificationCallback)(AMDeviceNotificationCallbackInfo *, void *);
typedef am_res_t (MDEV_API *AMDeviceInstallApplicationCallback)(CFDictionaryRef, void *);
typedef AMDevice *AMDeviceRef;
#endif
typedef am_res_t (MDEV_API *AMDeviceMountImageCallback)(void *, void *);



typedef void (MDEV_API *AMDSetLogLevelPtr)(int);
typedef am_res_t (MDEV_API *AMDeviceNotificationSubscribePtr)(AMDeviceNotificationCallback,
                                                               unsigned int, unsigned int, void *,
                                                               const AMDeviceNotification **);
typedef am_res_t (MDEV_API *AMDeviceNotificationUnsubscribePtr)(void *);
typedef CFPropertyListRef (MDEV_API *AMDeviceCopyValuePtr)(AMDeviceRef,CFStringRef,CFStringRef);
typedef unsigned int (MDEV_API *AMDeviceGetConnectionIDPtr)(AMDeviceRef);
typedef CFStringRef (MDEV_API *AMDeviceCopyDeviceIdentifierPtr)(AMDeviceRef);
typedef am_res_t (MDEV_API *AMDeviceConnectPtr)(AMDeviceRef);
typedef am_res_t (MDEV_API *AMDevicePairPtr)(AMDeviceRef);
typedef am_res_t (MDEV_API *AMDeviceIsPairedPtr)(AMDeviceRef);
typedef am_res_t (MDEV_API *AMDeviceValidatePairingPtr)(AMDeviceRef);
typedef am_res_t (MDEV_API *AMDeviceStartSessionPtr)(AMDeviceRef);
typedef am_res_t (MDEV_API *AMDeviceStopSessionPtr)(AMDeviceRef);
typedef am_res_t (MDEV_API *AMDeviceDisconnectPtr)(AMDeviceRef);
typedef am_res_t (MDEV_API *AMDeviceMountImagePtr)(AMDeviceRef, CFStringRef, CFDictionaryRef,
                                                        AMDeviceMountImageCallback, void *);
typedef am_res_t (MDEV_API *AMDeviceStartServicePtr)(AMDeviceRef, CFStringRef, ServiceSocket *, void *);
typedef am_res_t (MDEV_API *AMDeviceTransferApplicationPtr)(ServiceSocket, CFStringRef, CFDictionaryRef,
                                                                 AMDeviceInstallApplicationCallback,
                                                                 void *);
typedef am_res_t (MDEV_API *AMDeviceInstallApplicationPtr)(ServiceSocket, CFStringRef, CFDictionaryRef,
                                                                AMDeviceInstallApplicationCallback,
                                                                void*);
typedef am_res_t (MDEV_API *AMDeviceUninstallApplicationPtr)(ServiceSocket, CFStringRef, CFDictionaryRef,
                                                                AMDeviceInstallApplicationCallback,
                                                                void*);
typedef am_res_t (MDEV_API *AMDeviceLookupApplicationsPtr)(AMDeviceRef, CFDictionaryRef, CFDictionaryRef *);
typedef am_res_t (MDEV_API *USBMuxConnectByPortPtr)(unsigned int, int, ServiceSocket*);

} // extern C

} // anonymous namespace

namespace Ios {
namespace Internal {

class MobileDeviceLib {
public :
    MobileDeviceLib();

    bool load();
    bool isLoaded();
    QStringList errors();
//

    void setLogLevel(int i) ;
    am_res_t deviceNotificationSubscribe(AMDeviceNotificationCallback callback,
                                         unsigned int v1, unsigned int v2, void *v3,
                                         const AMDeviceNotification **handle);
    am_res_t deviceNotificationUnsubscribe(void *handle);
    CFPropertyListRef deviceCopyValue(AMDeviceRef,CFStringRef,CFStringRef);
    unsigned int deviceGetConnectionID(AMDeviceRef);
    CFStringRef deviceCopyDeviceIdentifier(AMDeviceRef);
    am_res_t deviceConnect(AMDeviceRef);
    am_res_t devicePair(AMDeviceRef);
    am_res_t deviceIsPaired(AMDeviceRef);
    am_res_t deviceValidatePairing(AMDeviceRef);
    am_res_t deviceStartSession(AMDeviceRef);
    am_res_t deviceStopSession(AMDeviceRef);
    am_res_t deviceDisconnect(AMDeviceRef);
    am_res_t deviceMountImage(AMDeviceRef, CFStringRef, CFDictionaryRef,
                                    AMDeviceMountImageCallback, void *);
    am_res_t deviceStartService(AMDeviceRef, CFStringRef, ServiceSocket *, void *);
    am_res_t deviceTransferApplication(int, CFStringRef, CFDictionaryRef,
                                       AMDeviceInstallApplicationCallback,
                                       void *);
    am_res_t deviceInstallApplication(int, CFStringRef, CFDictionaryRef,
                                      AMDeviceInstallApplicationCallback,
                                      void*);
    am_res_t deviceUninstallApplication(int, CFStringRef, CFDictionaryRef,
                                                                    AMDeviceInstallApplicationCallback,
                                                                    void*);
    am_res_t deviceLookupApplications(AMDeviceRef, CFDictionaryRef, CFDictionaryRef *);
    am_res_t connectByPort(unsigned int connectionId, int port, ServiceSocket *resFd);

    void addError(const QString &msg);
    void addError(const char *msg);
    QStringList m_errors;
private:
    QLibrary lib;
    QList<QLibrary *> deps;
    AMDSetLogLevelPtr m_AMDSetLogLevel;
    AMDeviceNotificationSubscribePtr m_AMDeviceNotificationSubscribe;
    AMDeviceNotificationUnsubscribePtr m_AMDeviceNotificationUnsubscribe;
    AMDeviceCopyValuePtr m_AMDeviceCopyValue;
    AMDeviceGetConnectionIDPtr m_AMDeviceGetConnectionID;
    AMDeviceCopyDeviceIdentifierPtr m_AMDeviceCopyDeviceIdentifier;
    AMDeviceConnectPtr m_AMDeviceConnect;
    AMDevicePairPtr m_AMDevicePair;
    AMDeviceIsPairedPtr m_AMDeviceIsPaired;
    AMDeviceValidatePairingPtr m_AMDeviceValidatePairing;
    AMDeviceStartSessionPtr m_AMDeviceStartSession;
    AMDeviceStopSessionPtr m_AMDeviceStopSession;
    AMDeviceDisconnectPtr m_AMDeviceDisconnect;
    AMDeviceMountImagePtr m_AMDeviceMountImage;
    AMDeviceStartServicePtr m_AMDeviceStartService;
    AMDeviceTransferApplicationPtr m_AMDeviceTransferApplication;
    AMDeviceInstallApplicationPtr m_AMDeviceInstallApplication;
    AMDeviceUninstallApplicationPtr m_AMDeviceUninstallApplication;
    AMDeviceLookupApplicationsPtr m_AMDeviceLookupApplications;
    USBMuxConnectByPortPtr m_USBMuxConnectByPort;
};

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
    bool startService(const QString &service, ServiceSocket &fd);
    void stopService(ServiceSocket fd);
    void startDeviceLookup(int timeout);
    bool connectToPort(quint16 port, ServiceSocket *fd) override;
    int qmljsDebugPort() const override;
    void addError(const QString &msg);
    bool writeAll(ServiceSocket fd, const char *cmd, qptrdiff len = -1);
    bool sendGdbCommand(ServiceSocket fd, const char *cmd, qptrdiff len = -1);
    QByteArray readGdbReply(ServiceSocket fd);
    bool expectGdbReply(ServiceSocket gdbFd, QByteArray expected);
    bool expectGdbOkReply(ServiceSocket gdbFd);

    MobileDeviceLib *lib();

    AMDeviceRef device;
    int progressBase;
    int unexpectedChars;
    bool aknowledge;
private:
    bool checkRead(qptrdiff nRead, int &maxRetry);
    int handleChar(int sock, QByteArray &res, char c, int status);
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
    void addDevice(AMDeviceRef device);
    void removeDevice(AMDeviceRef device);
    void checkPendingLookups();
    MobileDeviceLib *lib();
    void didTransferApp(const QString &bundlePath, const QString &deviceId,
                        Ios::IosDeviceManager::OpStatus status);
    void didStartApp(const QString &bundlePath, const QString &deviceId,
                     Ios::IosDeviceManager::OpStatus status, int gdbFd, DeviceSession *deviceSession);
    void isTransferringApp(const QString &bundlePath, const QString &deviceId, int progress,
                           const QString &info);
    void deviceWithId(QString deviceId, int timeout, DeviceAvailableCallback callback, void *userData);
    int processGdbServer(int fd);
    void stopGdbServer(int fd, int phase);
private:
    IosDeviceManager *q;
    QMutex m_sendMutex;
    QHash<QString, AMDeviceRef> m_devices;
    QMultiHash<QString, PendingDeviceLookup *> m_pendingLookups;
    AMDeviceNotificationRef m_notification;
    MobileDeviceLib m_lib;
};

class DevInfoSession: public CommandSession {
public:
    DevInfoSession(const QString &deviceId);

    void deviceCallbackReturned();
    QString commandName();
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

extern "C" void deviceNotificationCallback(AMDeviceNotificationCallbackInfo *info, void *user)
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

extern "C" void deviceAvailableSessionCallback(QString deviceId, AMDeviceRef device, void *userData)
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

extern "C" am_res_t appTransferSessionCallback(CFDictionaryRef dict, void *userData)
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

extern "C" am_res_t appInstallSessionCallback(CFDictionaryRef dict, void *userData)
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
    if (!m_lib.load())
        addError(QLatin1String("Error loading MobileDevice.framework"));
    if (!m_lib.errors().isEmpty())
        foreach (const QString &msg, m_lib.errors())
            addError(msg);
    m_lib.setLogLevel(5);
    am_res_t e = m_lib.deviceNotificationSubscribe(&deviceNotificationCallback, 0, 0,
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
    return m_lib.errors();
}

void IosDeviceManagerPrivate::addError(QString errorMsg)
{
    if (debugAll)
        qDebug() << "IosManagerPrivate ERROR: " << errorMsg;
    emit q->errorMsg(errorMsg);
}

void IosDeviceManagerPrivate::addDevice(AMDeviceRef device)
{
    CFStringRef s = m_lib.deviceCopyDeviceIdentifier(device);
    QString devId = QString::fromCFString(s);
    if (s) CFRelease(s);
    CFRetain(device);
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
    CFStringRef s = m_lib.deviceCopyDeviceIdentifier(device);
    QString devId = QString::fromCFString(s);
    if (s)
        CFRelease(s);
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

MobileDeviceLib *IosDeviceManagerPrivate::lib()
{
    return &m_lib;
}

void IosDeviceManagerPrivate::didTransferApp(const QString &bundlePath, const QString &deviceId,
                                       IosDeviceManager::OpStatus status)
{
    emit IosDeviceManagerPrivate::instance()->q->didTransferApp(bundlePath, deviceId, status);
}

void IosDeviceManagerPrivate::didStartApp(const QString &bundlePath, const QString &deviceId,
                                          IosDeviceManager::OpStatus status, int gdbFd,
                                          DeviceSession *deviceSession)
{
    emit IosDeviceManagerPrivate::instance()->q->didStartApp(bundlePath, deviceId, status, gdbFd,
                                                             deviceSession);
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
    QObject::connect(&(pendingLookup->timer), SIGNAL(timeout()), q, SLOT(checkPendingLookups()));
    m_pendingLookups.insertMulti(deviceId, pendingLookup);
    pendingLookup->timer.start();
}
enum GdbServerStatus {
    NORMAL_PROCESS,
    PROTOCOL_ERROR,
    STOP_FOR_SIGNAL,
    INFERIOR_EXITED,
    PROTOCOL_UNHANDLED
};

int IosDeviceManagerPrivate::processGdbServer(int fd)
{
    CommandSession session((QString()));
    {
        QMutexLocker l(&m_sendMutex);
        session.sendGdbCommand(fd, "vCont;c"); // resume all threads
    }
    GdbServerStatus state = NORMAL_PROCESS;
    int maxRetry = 10;
    int maxSignal = 5;
    while (state == NORMAL_PROCESS) {
        QByteArray repl = session.readGdbReply(fd);
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
                        if (session.sendGdbCommand(fd, "vCont;c"))
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

void IosDeviceManagerPrivate::stopGdbServer(int fd, int phase)
{
    CommandSession session((QString()));
    QMutexLocker l(&m_sendMutex);
    if (phase == 0)
        session.writeAll(fd,"\x03",1);
    else
        session.sendGdbCommand(fd, "k", 1);
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
    if (am_res_t error1 = lib()->deviceConnect(device)) {
        addError(QString::fromLatin1("connectDevice %1 failed, AMDeviceConnect returned %2")
                 .arg(deviceId).arg(error1));
        return false;
    }
    if (lib()->deviceIsPaired(device) == 0) { // not paired
        if (am_res_t error = lib()->devicePair(device)) {
            addError(QString::fromLatin1("connectDevice %1 failed, AMDevicePair returned %2")
                     .arg(deviceId).arg(error));
            return false;
        }
    }
    if (am_res_t error2 = lib()->deviceValidatePairing(device)) {
        addError(QString::fromLatin1("connectDevice %1 failed, AMDeviceValidatePairing returned %2")
                 .arg(deviceId).arg(error2));
        return false;
    }
    if (am_res_t error3 = lib()->deviceStartSession(device)) {
        addError(QString::fromLatin1("connectDevice %1 failed, AMDeviceStartSession returned %2")
                 .arg(deviceId).arg(error3));
        return false;
    }
    return true;
}

bool CommandSession::disconnectDevice()
{
    if (am_res_t error = lib()->deviceStopSession(device)) {
        addError(QString::fromLatin1("stopSession %1 failed, AMDeviceStopSession returned %2")
                 .arg(deviceId).arg(error));
        return false;
    }
    if (am_res_t error = lib()->deviceDisconnect(device)) {
        addError(QString::fromLatin1("disconnectDevice %1 failed, AMDeviceDisconnect returned %2")
                          .arg(deviceId).arg(error));
        return false;
    }
    return true;
}

bool CommandSession::startService(const QString &serviceName, ServiceSocket &fd)
{
    bool failure = false;
    fd = 0;
    if (!connectDevice())
        return false;
    CFStringRef cfsService = serviceName.toCFString();
    if (am_res_t error = lib()->deviceStartService(device, cfsService, &fd, 0)) {
        addError(QString::fromLatin1("startService on device %1 failed, AMDeviceStartService returned %2")
                 .arg(deviceId).arg(error));
        failure = true;
        fd = -1;
    }
    CFRelease(cfsService);
    disconnectDevice();
    return !failure;
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
    if (am_res_t error = lib()->connectByPort(lib()->deviceGetConnectionID(device), htons(port), &fileDescriptor)) {
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

bool CommandSession::sendGdbCommand(ServiceSocket fd, const char *cmd, qptrdiff len)
{
    if (len == -1)
        len = strlen(cmd);
    unsigned char checkSum = 0;
    for (int i = 0; i < len; ++i)
        checkSum += static_cast<unsigned char>(cmd[i]);
    bool failure = !writeAll(fd, "$", 1);
    if (!failure)
        failure = !writeAll(fd, cmd, len);
    char buf[3];
    buf[0] = '#';
    const char *hex = "0123456789abcdef";
    buf[1]   = hex[(checkSum >> 4) & 0xF];
    buf[2] = hex[checkSum & 0xF];
    if (!failure)
        failure = !writeAll(fd, buf, 3);
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

int CommandSession::handleChar(int fd, QByteArray &res, char c, int status)
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
        if ((c < '0' || c > '9') && (c < 'a' || c > 'f') && (c < 'A' | c > 'F')) {
            if (unexpectedChars < 15) {
                addError(QString::fromLatin1("unexpected char %1 in readGdbReply as checksum")
                         .arg(QChar::fromLatin1(c)));
                ++unexpectedChars;
            } else if (unexpectedChars == 15) {
                addError(QString::fromLatin1("hit maximum number of unexpected chars in checksum, ignoring them in readGdbReply"));
                ++unexpectedChars;
            }
        }
        if (status == 3 && aknowledge)
            writeAll(fd, "+", 1);
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

QByteArray CommandSession::readGdbReply(ServiceSocket fd)
{
    // read with minimal buffering because we might want to give the socket away...
    QByteArray res;
    char buf[5];
    int maxRetry = 10;
    int status = 0;
    int toRead = 4;
    while (status < 4 && toRead > 0) {
        qptrdiff nRead = read(fd, buf, toRead);
        if (!checkRead(nRead, maxRetry))
            return QByteArray();
        if (debugGdbServer) {
            buf[nRead] = 0;
            qDebug() << "gdbReply read " << buf;
        }
        for (qptrdiff i = 0; i< nRead; ++i)
            status = handleChar(fd, res, buf[i], status);
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
    Q_UNUSED(progress);
    Q_UNUSED(status);
}

QString CommandSession::commandName()
{
    return QString::fromLatin1("CommandSession(%1)").arg(deviceId);
}

bool CommandSession::expectGdbReply(ServiceSocket gdbFd, QByteArray expected)
{
    QByteArray repl = readGdbReply(gdbFd);
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

bool CommandSession::expectGdbOkReply(ServiceSocket gdbFd)
{
    return expectGdbReply(gdbFd, QByteArray("OK"));
}

MobileDeviceLib *CommandSession::lib()
{
    return IosDeviceManagerPrivate::instance()->lib();
}

AppOpSession::AppOpSession(const QString &deviceId, const QString &bundlePath,
                                       const QStringList &extraArgs, IosDeviceManager::AppOp appOp):
    CommandSession(deviceId), bundlePath(bundlePath), extraArgs(extraArgs), appOp(appOp)
{ }

QString AppOpSession::commandName()
{
    return QString::fromLatin1("TransferAppSession(%1, %2)").arg(deviceId, bundlePath);
}

static QString mobileDeviceErrorString(am_res_t code)
{
    static const char *errorStrings[] = {
        "kAMDSuccess", // 0x0
        "kAMDUndefinedError", // 0xe8000001
        "kAMDBadHeaderError",
        "kAMDNoResourcesError",
        "kAMDReadError",
        "kAMDWriteError",
        "kAMDUnknownPacketError",
        "kAMDInvalidArgumentError",
        "kAMDNotFoundError",
        "kAMDIsDirectoryError",
        "kAMDPermissionError",
        "kAMDNotConnectedError",
        "kAMDTimeOutError",
        "kAMDOverrunError",
        "kAMDEOFError",
        "kAMDUnsupportedError",
        "kAMDFileExistsError",
        "kAMDBusyError",
        "kAMDCryptoError",
        "kAMDInvalidResponseError",
        "kAMDMissingKeyError",
        "kAMDMissingValueError",
        "kAMDGetProhibitedError",
        "kAMDSetProhibitedError",
        "kAMDRemoveProhibitedError",
        "kAMDImmutableValueError",
        "kAMDPasswordProtectedError",
        "kAMDMissingHostIDError",
        "kAMDInvalidHostIDError",
        "kAMDSessionActiveError",
        "kAMDSessionInactiveError",
        "kAMDMissingSessionIDError",
        "kAMDInvalidSessionIDError",
        "kAMDMissingServiceError",
        "kAMDInvalidServiceError",
        "kAMDInvalidCheckinError",
        "kAMDCheckinTimeoutError",
        "kAMDMissingPairRecordError",
        "kAMDInvalidActivationRecordError",
        "kAMDMissingActivationRecordError",
        "kAMDWrongDroidError",
        "kAMDSUVerificationError",
        "kAMDSUPatchError",
        "kAMDSUFirmwareError",
        "kAMDProvisioningProfileNotValid",
        "kAMDSendMessageError",
        "kAMDReceiveMessageError",
        "kAMDMissingOptionsError",
        "kAMDMissingImageTypeError",
        "kAMDDigestFailedError",
        "kAMDStartServiceError",
        "kAMDInvalidDiskImageError",
        "kAMDMissingDigestError",
        "kAMDMuxError",
        "kAMDApplicationAlreadyInstalledError",
        "kAMDApplicationMoveFailedError",
        "kAMDApplicationSINFCaptureFailedError",
        "kAMDApplicationSandboxFailedError",
        "kAMDApplicationVerificationFailedError",
        "kAMDArchiveDestructionFailedError",
        "kAMDBundleVerificationFailedError",
        "kAMDCarrierBundleCopyFailedError",
        "kAMDCarrierBundleDirectoryCreationFailedError",
        "kAMDCarrierBundleMissingSupportedSIMsError",
        "kAMDCommCenterNotificationFailedError",
        "kAMDContainerCreationFailedError",
        "kAMDContainerP0wnFailedError",
        "kAMDContainerRemovalFailedError",
        "kAMDEmbeddedProfileInstallFailedError",
        "kAMDErrorError",
        "kAMDExecutableTwiddleFailedError",
        "kAMDExistenceCheckFailedError",
        "kAMDInstallMapUpdateFailedError",
        "kAMDManifestCaptureFailedError",
        "kAMDMapGenerationFailedError",
        "kAMDMissingBundleExecutableError",
        "kAMDMissingBundleIdentifierError",
        "kAMDMissingBundlePathError",
        "kAMDMissingContainerError",
        "kAMDNotificationFailedError",
        "kAMDPackageExtractionFailedError",
        "kAMDPackageInspectionFailedError",
        "kAMDPackageMoveFailedError",
        "kAMDPathConversionFailedError",
        "kAMDRestoreContainerFailedError",
        "kAMDSeatbeltProfileRemovalFailedError",
        "kAMDStageCreationFailedError",
        "kAMDSymlinkFailedError",
        "kAMDiTunesArtworkCaptureFailedError",
        "kAMDiTunesMetadataCaptureFailedError",
        "kAMDAlreadyArchivedError",
        "kAMDServiceLimitError",
        "kAMDInvalidPairRecordError",
        "kAMDServiceProhibitedError",
        "kAMDCheckinSetupFailedError",
        "kAMDCheckinConnectionFailedError",
        "kAMDCheckinReceiveFailedError",
        "kAMDCheckinResponseFailedError",
        "kAMDCheckinSendFailedError",
        "kAMDMuxCreateListenerError",
        "kAMDMuxGetListenerError",
        "kAMDMuxConnectError",
        "kAMDUnknownCommandError",
        "kAMDAPIInternalError",
        "kAMDSavePairRecordFailedError",
        "kAMDCheckinOutOfMemoryError",
        "kAMDDeviceTooNewError",
        "kAMDDeviceRefNoGood",
        "kAMDCannotTranslateError",
        "kAMDMobileImageMounterMissingImageSignature",
        "kAMDMobileImageMounterResponseCreationFailed",
        "kAMDMobileImageMounterMissingImageType",
        "kAMDMobileImageMounterMissingImagePath",
        "kAMDMobileImageMounterImageMapLoadFailed",
        "kAMDMobileImageMounterAlreadyMounted",
        "kAMDMobileImageMounterImageMoveFailed",
        "kAMDMobileImageMounterMountPathMissing",
        "kAMDMobileImageMounterMountPathNotEmpty",
        "kAMDMobileImageMounterImageMountFailed",
        "kAMDMobileImageMounterTrustCacheLoadFailed",
        "kAMDMobileImageMounterDigestFailed",
        "kAMDMobileImageMounterDigestCreationFailed",
        "kAMDMobileImageMounterImageVerificationFailed",
        "kAMDMobileImageMounterImageInfoCreationFailed",
        "kAMDMobileImageMounterImageMapStoreFailed",
        "kAMDBonjourSetupError",
        "kAMDDeviceOSVersionTooLow",
        "kAMDNoWifiSyncSupportError",
        "kAMDDeviceFamilyNotSupported",
        "kAMDEscrowLockedError",
        "kAMDPairingProhibitedError",
        "kAMDProhibitedBySupervision",
        "kAMDDeviceDisconnectedError",
        "kAMDTooBigError",
        "kAMDPackagePatchFailedError",
        "kAMDIncorrectArchitectureError",
        "kAMDPluginCopyFailedError",
        "kAMDBreadcrumbFailedError",
        "kAMDBreadcrumbUnlockError",
        "kAMDGeoJSONCaptureFailedError",
        "kAMDNewsstandArtworkCaptureFailedError",
        "kAMDMissingCommandError",
        "kAMDNotEntitledError",
        "kAMDMissingPackagePathError",
        "kAMDMissingContainerPathError",
        "kAMDMissingApplicationIdentifierError",
        "kAMDMissingAttributeValueError",
        "kAMDLookupFailedError",
        "kAMDDictCreationFailedError",
        "kAMDUserDeniedPairingError",
        "kAMDPairingDialogResponsePendingError",
        "kAMDInstallProhibitedError",
        "kAMDUninstallProhibitedError",
        "kAMDFMiPProtectedError",
        "kAMDMCProtected",
        "kAMDMCChallengeRequired",
        "kAMDMissingBundleVersionError" // 0xe800009c
    };

    CFStringRef key = NULL;
    static const size_t errorStringLast = ((sizeof(errorStrings) / sizeof(char *)) - 1) | 0xe8000000;
    if (code <= errorStringLast) {
        // Mask off some bits to get an index into the known error names array
        key = QString::fromLatin1(errorStrings[code & ~0xe8000000]).toCFString();
    } else {
        // Some errors don't have constant names; check a few other known error codes
        switch (code) {
        case 0xe8008015:
            key = CFSTR("A valid provisioning profile for this executable was not found.");
            break;
        case 0xe8008016:
            key = CFSTR("The executable was signed with invalid entitlements.");
            break;
        case 0xe8008017:
            key = CFSTR("A signed resource has been added, modified, or deleted.");
            break;
        case 0xe8008018:
            key = CFSTR("The identity used to sign the executable is no longer valid.");
            break;
        case 0xe8008019:
            key = CFSTR("The application does not have a valid signature.");
            break;
        case 0xe800801c:
            key = CFSTR("The signature was not valid.");
            break;
        default:
            return QString();
        }

        CFRetain(key);
    }

    CFURLRef url = QUrl::fromLocalFile(
        QStringLiteral("/System/Library/PrivateFrameworks/MobileDevice.framework")).toCFURL();
    CFBundleRef mobileDeviceBundle = CFBundleCreate(kCFAllocatorDefault, url);
    CFRelease(url);

    QString s;
    if (mobileDeviceBundle) {
        CFStringRef str = CFCopyLocalizedStringFromTableInBundle(key, CFSTR("Localizable"),
                                                                 mobileDeviceBundle, nil);

        s = QString::fromCFString(str);
        CFRelease(str);
    }

    CFRelease(key);
    return s;
}

bool AppOpSession::installApp()
{
    ServiceSocket fd;
    bool failure = (device == 0);
    if (!failure) {
        failure = !startService(QLatin1String("com.apple.afc"), fd);
        if (!failure) {
            CFStringRef cfsBundlePath = bundlePath.toCFString();
            if (am_res_t error = lib()->deviceTransferApplication(fd, cfsBundlePath, 0,
                                                             &appTransferSessionCallback,
                                                             static_cast<CommandSession *>(this))) {
                addError(QString::fromLatin1("TransferAppSession(%1,%2) failed, AMDeviceTransferApplication returned %3")
                         .arg(bundlePath, deviceId).arg(error));
                failure = true;
            }
            progressBase += 100;
            CFRelease(cfsBundlePath);
        }
        stopService(fd);
    }
    if (debugAll)
        qDebug() << "AMDeviceTransferApplication finished request with " << failure;
    if (!failure) {
        failure = !startService(QLatin1String("com.apple.mobile.installation_proxy"), fd);
        if (!failure) {
            CFStringRef cfsBundlePath = bundlePath.toCFString();

            CFStringRef key[1] = {CFSTR("PackageType")};
            CFStringRef value[1] = {CFSTR("Developer")};
            CFDictionaryRef options = CFDictionaryCreate(0, reinterpret_cast<const void**>(&key[0]),
                    reinterpret_cast<const void**>(&value[0]), 1,
                                                         &kCFTypeDictionaryKeyCallBacks,
                                                         &kCFTypeDictionaryValueCallBacks);

            if (am_res_t error = lib()->deviceInstallApplication(fd, cfsBundlePath, options,
                                                                   &appInstallSessionCallback,
                                                                   static_cast<CommandSession *>(this))) {
                const QString errorString = mobileDeviceErrorString(error);
                if (!errorString.isEmpty())
                    addError(errorString
                             + QStringLiteral(" (0x")
                             + QString::number(error, 16)
                             + QStringLiteral(")"));
                else
                    addError(QString::fromLatin1("InstallAppSession(%1,%2) failed, "
                                                 "AMDeviceInstallApplication returned 0x%3")
                             .arg(bundlePath, deviceId).arg(QString::number(error, 16)));
                failure = true;
            }
            progressBase += 100;
            CFRelease(options);
            CFRelease(cfsBundlePath);
        }
        stopService(fd);
    }
    if (!failure)
        sleep(5); // after installation the device needs a bit of quiet....
    if (debugAll)
        qDebug() << "AMDeviceInstallApplication finished request with " << failure;
    IosDeviceManagerPrivate::instance()->didTransferApp(bundlePath, deviceId,
                (failure ? IosDeviceManager::Failure : IosDeviceManager::Success));
    return !failure;
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
    QRegExp qmlPortRe = QRegExp(QLatin1String("-qmljsdebugger=port:([0-9]+)"));
    foreach (const QString &arg, extraArgs) {
        if (qmlPortRe.indexIn(arg) == 0) {
            bool ok;
            int res = qmlPortRe.cap(1).toInt(&ok);
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
    ServiceSocket gdbFd = -1;
    if (!failure && !startService(QLatin1String("com.apple.debugserver"), gdbFd))
        gdbFd = -1;
    if (gdbFd > 0) {
        // gdbServer protocol, see http://sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html#Remote-Protocol
        // and the lldb handling of that (with apple specific stuff)
        // http://llvm.org/viewvc/llvm-project/lldb/trunk/tools/debugserver/source/RNBRemote.cpp
        //failure = !sendGdbCommand(gdbFd, "QStartNoAckMode"); // avoid and send required aknowledgements?
        //if (!failure) failure = !expectGdbOkReply(gdbFd);
        if (!failure) failure = !sendGdbCommand(gdbFd, "QEnvironmentHexEncoded:"); // send the environment with a series of these commands...
        if (!failure) failure = !sendGdbCommand(gdbFd, "QSetDisableASLR:1"); // avoid address randomization to debug
        if (!failure) failure = !expectGdbOkReply(gdbFd);
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
        if (!failure) failure = !sendGdbCommand(gdbFd, runCommand.constData(), runCommand.size());
        if (!failure) failure = !expectGdbOkReply(gdbFd);
        if (!failure) failure = !sendGdbCommand(gdbFd, "qLaunchSuccess");
        if (!failure) failure = !expectGdbOkReply(gdbFd);
    }
    IosDeviceManagerPrivate::instance()->didStartApp(
                bundlePath, deviceId,
                (failure ? IosDeviceManager::Failure : IosDeviceManager::Success), gdbFd, this);
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
    if (int err = lib()->deviceLookupApplications(device, options, &apps)) {
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

void DevInfoSession::deviceCallbackReturned()
{
    if (debugAll)
        qDebug() << "device available";
    QMap<QString,QString> res;
    QString deviceNameKey = QLatin1String("deviceName");
    QString developerStatusKey = QLatin1String("developerStatus");
    QString deviceConnectedKey = QLatin1String("deviceConnected");
    QString osVersionKey = QLatin1String("osVersion");
    bool failure = !device;
    if (!failure) {
        failure = !connectDevice();
        if (!failure) {
            res[deviceConnectedKey] = QLatin1String("YES");
            CFPropertyListRef cfDeviceName = lib()->deviceCopyValue(device, 0,
                                                                   CFSTR("DeviceName"));
            // CFShow(cfDeviceName);
            if (cfDeviceName) {
                if (CFGetTypeID(cfDeviceName) == CFStringGetTypeID())
                    res[deviceNameKey] = QString::fromCFString(reinterpret_cast<CFStringRef>(cfDeviceName));
                CFRelease(cfDeviceName);
            }
            if (!res.contains(deviceNameKey))
                res[deviceNameKey] = QString();
        }
        if (!failure) {
            CFPropertyListRef cfDevStatus = lib()->deviceCopyValue(device,
                                                                   CFSTR("com.apple.xcode.developerdomain"),
                                                                   CFSTR("DeveloperStatus"));
            // CFShow(cfDevStatus);
            if (cfDevStatus) {
                if (CFGetTypeID(cfDevStatus) == CFStringGetTypeID())
                    res[developerStatusKey] = QString::fromCFString(reinterpret_cast<CFStringRef>(cfDevStatus));
                CFRelease(cfDevStatus);
            }
            if (!res.contains(developerStatusKey))
                res[developerStatusKey] = QLatin1String("*off*");
        }
        if (!failure) {
            CFPropertyListRef cfProductVersion = lib()->deviceCopyValue(device,
                                                                        0,
                                                                        CFSTR("ProductVersion"));
            //CFShow(cfProductVersion);
            CFPropertyListRef cfBuildVersion = lib()->deviceCopyValue(device,
                                                                      0,
                                                                      CFSTR("BuildVersion"));
            //CFShow(cfBuildVersion);
            QString versionString;
            if (cfProductVersion) {
                if (CFGetTypeID(cfProductVersion) == CFStringGetTypeID())
                    versionString = QString::fromCFString(reinterpret_cast<CFStringRef>(cfProductVersion));
                CFRelease(cfProductVersion);
            }
            if (cfBuildVersion) {
                if (!versionString.isEmpty() && CFGetTypeID(cfBuildVersion) == CFStringGetTypeID())
                    versionString += QString::fromLatin1(" (%1)").arg(
                                QString::fromCFString(reinterpret_cast<CFStringRef>(cfBuildVersion)));
                    CFRelease(cfBuildVersion);
            }
            if (!versionString.isEmpty())
                res[osVersionKey] = versionString;
            else
                res[osVersionKey] = QLatin1String("*unknown*");
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

MobileDeviceLib::MobileDeviceLib() { }

bool MobileDeviceLib::load()
{
#ifdef MOBILE_DEV_DIRECT_LINK
    m_AMDSetLogLevel = &AMDSetLogLevel;
    m_AMDeviceNotificationSubscribe = &AMDeviceNotificationSubscribe;
    //m_AMDeviceNotificationUnsubscribe = &AMDeviceNotificationUnsubscribe;
    m_AMDeviceCopyValue = &AMDeviceCopyValue;
    m_AMDeviceGetConnectionID = &AMDeviceGetConnectionID;
    m_AMDeviceCopyDeviceIdentifier = &AMDeviceCopyDeviceIdentifier;
    m_AMDeviceConnect = &AMDeviceConnect;
    //m_AMDevicePair = &AMDevicePair;
    m_AMDeviceIsPaired = &AMDeviceIsPaired;
    m_AMDeviceValidatePairing = &AMDeviceValidatePairing;
    m_AMDeviceStartSession = &AMDeviceStartSession;
    m_AMDeviceStopSession = &AMDeviceStopSession;
    m_AMDeviceDisconnect = &AMDeviceDisconnect;
    m_AMDeviceMountImage = &AMDeviceMountImage;
    m_AMDeviceStartService = &AMDeviceStartService;
    m_AMDeviceTransferApplication = &AMDeviceTransferApplication;
    m_AMDeviceInstallApplication = &AMDeviceInstallApplication;
    //m_AMDeviceUninstallApplication = &AMDeviceUninstallApplication;
    //m_AMDeviceLookupApplications = &AMDeviceLookupApplications;
    m_USBMuxConnectByPort = &USBMuxConnectByPort;
#else
    QLibrary *libAppleFSCompression = new QLibrary(QLatin1String("/System/Library/PrivateFrameworks/AppleFSCompression.framework/AppleFSCompression"));
    if (!libAppleFSCompression->load())
        addError("MobileDevice dependency AppleFSCompression failed to load");
    deps << libAppleFSCompression;
    QLibrary *libBom = new QLibrary(QLatin1String("/System/Library/PrivateFrameworks/Bom.framework/Bom"));
    if (!libBom->load())
        addError("MobileDevice dependency Bom failed to load");
    deps << libBom;
    lib.setFileName(QLatin1String("/System/Library/PrivateFrameworks/MobileDevice.framework/MobileDevice"));
    if (!lib.load())
        return false;
    m_AMDSetLogLevel = reinterpret_cast<AMDSetLogLevelPtr>(lib.resolve("AMDSetLogLevel"));
    if (m_AMDSetLogLevel == 0)
        addError("MobileDeviceLib does not define AMDSetLogLevel");
    m_AMDeviceNotificationSubscribe = reinterpret_cast<AMDeviceNotificationSubscribePtr>(lib.resolve("AMDeviceNotificationSubscribe"));
    if (m_AMDeviceNotificationSubscribe == 0)
        addError("MobileDeviceLib does not define AMDeviceNotificationSubscribe");
    m_AMDeviceNotificationUnsubscribe = reinterpret_cast<AMDeviceNotificationUnsubscribePtr>(lib.resolve("AMDeviceNotificationUnsubscribe"));
    if (m_AMDeviceNotificationUnsubscribe == 0)
        addError("MobileDeviceLib does not define AMDeviceNotificationUnsubscribe");
    m_AMDeviceCopyValue = reinterpret_cast<AMDeviceCopyValuePtr>(lib.resolve("AMDeviceCopyValue"));
    if (m_AMDSetLogLevel == 0)
        addError("MobileDeviceLib does not define AMDSetLogLevel");
    m_AMDeviceGetConnectionID = reinterpret_cast<AMDeviceGetConnectionIDPtr>(lib.resolve("AMDeviceGetConnectionID"));
    if (m_AMDeviceGetConnectionID == 0)
        addError("MobileDeviceLib does not define AMDeviceGetConnectionID");
    m_AMDeviceCopyDeviceIdentifier = reinterpret_cast<AMDeviceCopyDeviceIdentifierPtr>(lib.resolve("AMDeviceCopyDeviceIdentifier"));
    if (m_AMDeviceCopyDeviceIdentifier == 0)
        addError("MobileDeviceLib does not define AMDeviceCopyDeviceIdentifier");
    m_AMDeviceConnect = reinterpret_cast<AMDeviceConnectPtr>(lib.resolve("AMDeviceConnect"));
    if (m_AMDeviceConnect == 0)
        addError("MobileDeviceLib does not define AMDeviceConnect");
    m_AMDevicePair = reinterpret_cast<AMDevicePairPtr>(lib.resolve("AMDevicePair"));
    if (m_AMDevicePair == 0)
        addError("MobileDeviceLib does not define AMDevicePair");
    m_AMDeviceIsPaired = reinterpret_cast<AMDeviceIsPairedPtr>(lib.resolve("AMDeviceIsPaired"));
    if (m_AMDeviceIsPaired == 0)
        addError("MobileDeviceLib does not define AMDeviceIsPaired");
    m_AMDeviceValidatePairing = reinterpret_cast<AMDeviceValidatePairingPtr>(lib.resolve("AMDeviceValidatePairing"));
    if (m_AMDeviceValidatePairing == 0)
        addError("MobileDeviceLib does not define AMDeviceValidatePairing");
    m_AMDeviceStartSession = reinterpret_cast<AMDeviceStartSessionPtr>(lib.resolve("AMDeviceStartSession"));
    if (m_AMDeviceStartSession == 0)
        addError("MobileDeviceLib does not define AMDeviceStartSession");
    m_AMDeviceStopSession = reinterpret_cast<AMDeviceStopSessionPtr>(lib.resolve("AMDeviceStopSession"));
    if (m_AMDeviceStopSession == 0)
        addError("MobileDeviceLib does not define AMDeviceStopSession");
    m_AMDeviceDisconnect = reinterpret_cast<AMDeviceDisconnectPtr>(lib.resolve("AMDeviceDisconnect"));
    if (m_AMDeviceDisconnect == 0)
        addError("MobileDeviceLib does not define AMDeviceDisconnect");
    m_AMDeviceMountImage = reinterpret_cast<AMDeviceMountImagePtr>(lib.resolve("AMDeviceMountImage"));
    if (m_AMDeviceMountImage == 0)
        addError("MobileDeviceLib does not define AMDeviceMountImage");
    m_AMDeviceStartService = reinterpret_cast<AMDeviceStartServicePtr>(lib.resolve("AMDeviceStartService"));
    if (m_AMDeviceStartService == 0)
        addError("MobileDeviceLib does not define AMDeviceStartService");
    m_AMDeviceTransferApplication = reinterpret_cast<AMDeviceTransferApplicationPtr>(lib.resolve("AMDeviceTransferApplication"));
    if (m_AMDeviceTransferApplication == 0)
        addError("MobileDeviceLib does not define AMDeviceTransferApplication");
    m_AMDeviceInstallApplication = reinterpret_cast<AMDeviceInstallApplicationPtr>(lib.resolve("AMDeviceInstallApplication"));
    if (m_AMDeviceInstallApplication == 0)
        addError("MobileDeviceLib does not define AMDeviceInstallApplication");
    m_AMDeviceUninstallApplication = reinterpret_cast<AMDeviceUninstallApplicationPtr>(lib.resolve("AMDeviceUninstallApplication"));
    if (m_AMDeviceUninstallApplication == 0)
        addError("MobileDeviceLib does not define AMDeviceUninstallApplication");
    m_AMDeviceLookupApplications = reinterpret_cast<AMDeviceLookupApplicationsPtr>(lib.resolve("AMDeviceLookupApplications"));
    if (m_AMDeviceLookupApplications == 0)
        addError("MobileDeviceLib does not define AMDeviceLookupApplications");
    m_USBMuxConnectByPort = reinterpret_cast<USBMuxConnectByPortPtr>(lib.resolve("USBMuxConnectByPort"));
    if (m_USBMuxConnectByPort == 0)
        addError("MobileDeviceLib does not define USBMuxConnectByPort");
#endif
    return true;
}

bool MobileDeviceLib::isLoaded()
{
    return lib.isLoaded();
}

QStringList MobileDeviceLib::errors()
{
    return m_errors;
}

void MobileDeviceLib::setLogLevel(int i)
{
    if (m_AMDSetLogLevel)
        m_AMDSetLogLevel(i);
}

am_res_t MobileDeviceLib::deviceNotificationSubscribe(AMDeviceNotificationCallback callback,
                                           unsigned int v1, unsigned int v2, void *callbackArgs,
                                           const AMDeviceNotification **handle)
{
    if (m_AMDeviceNotificationSubscribe)
        return m_AMDeviceNotificationSubscribe(callback,v1,v2,callbackArgs,handle);
    return -1;
}

am_res_t MobileDeviceLib::deviceNotificationUnsubscribe(void *handle)
{
    if (m_AMDeviceNotificationUnsubscribe)
        return m_AMDeviceNotificationUnsubscribe(handle);
    return -1;
}

CFPropertyListRef MobileDeviceLib::deviceCopyValue(AMDeviceRef device,CFStringRef group,CFStringRef key)
{
    if (m_AMDeviceCopyValue)
        return m_AMDeviceCopyValue(device, group, key);
    return 0;
}

unsigned int MobileDeviceLib::deviceGetConnectionID(AMDeviceRef device)
{
    if (m_AMDeviceGetConnectionID)
        return m_AMDeviceGetConnectionID(device);
    return -1;
}

CFStringRef MobileDeviceLib::deviceCopyDeviceIdentifier(AMDeviceRef device)
{
    if (m_AMDeviceCopyDeviceIdentifier)
        return m_AMDeviceCopyDeviceIdentifier(device);
    return 0;
}

am_res_t MobileDeviceLib::deviceConnect(AMDeviceRef device)
{
    if (m_AMDeviceConnect)
        return m_AMDeviceConnect(device);
    return -1;
}

am_res_t MobileDeviceLib::devicePair(AMDeviceRef device)
{
    if (m_AMDevicePair)
        return m_AMDevicePair(device);
    return -1;
}

am_res_t MobileDeviceLib::deviceIsPaired(AMDeviceRef device)
{
    if (m_AMDeviceIsPaired)
        return m_AMDeviceIsPaired(device);
    return -1;
}

am_res_t MobileDeviceLib::deviceValidatePairing(AMDeviceRef device)
{
    if (m_AMDeviceValidatePairing)
        return m_AMDeviceValidatePairing(device);
    return -1;
}

am_res_t MobileDeviceLib::deviceStartSession(AMDeviceRef device)
{
    if (m_AMDeviceStartSession)
        return m_AMDeviceStartSession(device);
    return -1;
}

am_res_t MobileDeviceLib::deviceStopSession(AMDeviceRef device)
{
    if (m_AMDeviceStopSession)
        return m_AMDeviceStopSession(device);
    return -1;
}

am_res_t MobileDeviceLib::deviceDisconnect(AMDeviceRef device)
{
    if (m_AMDeviceDisconnect)
        return m_AMDeviceDisconnect(device);
    return -1;
}

am_res_t MobileDeviceLib::deviceMountImage(AMDeviceRef device, CFStringRef imagePath,
                                                 CFDictionaryRef options,
                                                 AMDeviceMountImageCallback callback,
                                                 void * callbackExtraArgs)
{
    if (m_AMDeviceMountImage)
        return m_AMDeviceMountImage(device, imagePath, options, callback, callbackExtraArgs);
    return -1;
}

am_res_t MobileDeviceLib::deviceStartService(AMDeviceRef device, CFStringRef serviceName,
                                                   ServiceSocket *fdRef, void *extra)
{
    if (m_AMDeviceStartService)
        return m_AMDeviceStartService(device, serviceName, fdRef, extra);
    return -1;
}

am_res_t MobileDeviceLib::deviceTransferApplication(int serviceFd, CFStringRef appPath,
                                                          CFDictionaryRef options,
                                                          AMDeviceInstallApplicationCallback callback,
                                                          void *callbackExtraArgs)
{
    if (m_AMDeviceTransferApplication)
        return m_AMDeviceTransferApplication(serviceFd, appPath, options, callback, callbackExtraArgs);
    return -1;
}

am_res_t MobileDeviceLib::deviceInstallApplication(int serviceFd, CFStringRef appPath,
                                                         CFDictionaryRef options,
                                                         AMDeviceInstallApplicationCallback callback,
                                                         void *callbackExtraArgs)
{
    if (m_AMDeviceInstallApplication)
        return m_AMDeviceInstallApplication(serviceFd, appPath, options, callback, callbackExtraArgs);
    return -1;
}

am_res_t MobileDeviceLib::deviceUninstallApplication(int serviceFd, CFStringRef bundleId,
                                                           CFDictionaryRef options,
                                                           AMDeviceInstallApplicationCallback callback,
                                                           void* callbackExtraArgs)
{
    if (m_AMDeviceUninstallApplication)
        return m_AMDeviceUninstallApplication(serviceFd, bundleId, options, callback, callbackExtraArgs);
    return -1;
}

am_res_t MobileDeviceLib::deviceLookupApplications(AMDeviceRef device, CFDictionaryRef options,
                                                         CFDictionaryRef *res)
{
    if (m_AMDeviceLookupApplications)
        return m_AMDeviceLookupApplications(device, options, res);
    return -1;
}

am_res_t MobileDeviceLib::connectByPort(unsigned int connectionId, int port, ServiceSocket *resFd)
{
    if (m_USBMuxConnectByPort)
        return m_USBMuxConnectByPort(connectionId, port, resFd);
    return -1;
}

void MobileDeviceLib::addError(const QString &msg)
{
    qDebug() << "MobileDeviceLib ERROR:" << msg;
    m_errors << QLatin1String("MobileDeviceLib ERROR:") << msg;
}

void MobileDeviceLib::addError(const char *msg)
{
    addError(QLatin1String(msg));
}

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

int IosDeviceManager::processGdbServer(int fd)
{
    return d->processGdbServer(fd);
}

void IosDeviceManager::stopGdbServer(int fd, int phase)
{
    return d->stopGdbServer(fd, phase);
}

QStringList IosDeviceManager::errors() {
    return d->errors();
}

void IosDeviceManager::checkPendingLookups()
{
    d->checkPendingLookups();
}

} // namespace Ios
