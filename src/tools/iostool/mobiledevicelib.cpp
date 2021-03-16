/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "mobiledevicelib.h"

#include <QDebug>


#ifdef MOBILE_DEV_DIRECT_LINK
#include "MobileDevice.h"
#endif

namespace Ios {
MobileDeviceLib &MobileDeviceLib::instance()
{
    static MobileDeviceLib lib;
    return lib;
}

MobileDeviceLib::MobileDeviceLib()
{
    if (!load())
        addError(QLatin1String("Error loading MobileDevice.framework"));
    if (!errors().isEmpty()) {
        foreach (const QString &msg, errors())
            addError(msg);
    }
    setLogLevel(5);
}

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
    m_AMDeviceGetInterfaceType = reinterpret_cast<AMDeviceGetInterfaceTypePtr>(lib.resolve("AMDeviceGetInterfaceType"));
    if (m_AMDeviceGetInterfaceType == 0)
        addError("MobileDeviceLib does not define AMDeviceGetInterfaceType");
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
    m_AMDeviceSecureStartService = reinterpret_cast<AMDeviceSecureStartServicePtr>(lib.resolve("AMDeviceSecureStartService"));
    if (m_AMDeviceSecureStartService == 0)
        addError("MobileDeviceLib does not define AMDeviceSecureStartService");
    m_AMDeviceSecureTransferPath = reinterpret_cast<AMDeviceSecureTransferPathPtr>(lib.resolve("AMDeviceSecureTransferPath"));
    if (m_AMDeviceSecureTransferPath == 0)
        addError("MobileDeviceLib does not define AMDeviceSecureTransferPath");
    m_AMDeviceSecureInstallApplication = reinterpret_cast<AMDeviceSecureInstallApplicationPtr>(lib.resolve("AMDeviceSecureInstallApplication"));
    if (m_AMDeviceSecureInstallApplication == 0)
        addError("MobileDeviceLib does not define AMDeviceSecureInstallApplication");
    m_AMDServiceConnectionGetSocket = reinterpret_cast<AMDServiceConnectionGetSocketPtr>(lib.resolve("AMDServiceConnectionGetSocket"));
    if (m_AMDServiceConnectionGetSocket == nullptr)
        addError("MobileDeviceLib does not define AMDServiceConnectionGetSocket");
    m_AMDeviceUninstallApplication = reinterpret_cast<AMDeviceUninstallApplicationPtr>(lib.resolve("AMDeviceUninstallApplication"));
    m_AMDServiceConnectionSend = reinterpret_cast<AMDServiceConnectionSendPtr>(lib.resolve("AMDServiceConnectionSend"));
    if (m_AMDServiceConnectionSend == nullptr)
        addError("MobileDeviceLib does not define AMDServiceConnectionSend");
    m_AMDServiceConnectionReceive = reinterpret_cast<AMDServiceConnectionReceivePtr>(lib.resolve("AMDServiceConnectionReceive"));
    if (m_AMDServiceConnectionReceive == nullptr)
        addError("MobileDeviceLib does not define AMDServiceConnectionReceive");
    m_AMDServiceConnectionInvalidate = reinterpret_cast<AMDServiceConnectionInvalidatePtr>(lib.resolve("AMDServiceConnectionInvalidate"));
    if (m_AMDServiceConnectionInvalidate == nullptr)
        addError("MobileDeviceLib does not define AMDServiceConnectionInvalidate");
    m_AMDeviceIsAtLeastVersionOnPlatform = reinterpret_cast<AMDeviceIsAtLeastVersionOnPlatformPtr>(lib.resolve("AMDeviceIsAtLeastVersionOnPlatform"));
    if (m_AMDeviceIsAtLeastVersionOnPlatform == nullptr)
        addError("MobileDeviceLib does not define AMDeviceIsAtLeastVersionOnPlatform");
    if (m_AMDeviceUninstallApplication == 0)
        addError("MobileDeviceLib does not define AMDeviceUninstallApplication");
    m_AMDeviceLookupApplications = reinterpret_cast<AMDeviceLookupApplicationsPtr>(lib.resolve("AMDeviceLookupApplications"));
    if (m_AMDeviceLookupApplications == 0)
        addError("MobileDeviceLib does not define AMDeviceLookupApplications");
    m_AMDErrorString = reinterpret_cast<AMDErrorStringPtr>(lib.resolve("AMDErrorString"));
    if (m_AMDErrorString == 0)
        addError("MobileDeviceLib does not define AMDErrorString");
    m_MISCopyErrorStringForErrorCode = reinterpret_cast<MISCopyErrorStringForErrorCodePtr>(lib.resolve("MISCopyErrorStringForErrorCode"));
    if (m_MISCopyErrorStringForErrorCode == 0)
        addError("MobileDeviceLib does not define MISCopyErrorStringForErrorCode");
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

int MobileDeviceLib::deviceGetInterfaceType(AMDeviceRef device)
{
    if (m_AMDeviceGetInterfaceType)
        return m_AMDeviceGetInterfaceType(device);
    return DeviceInterfaceType::UNKNOWN;
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
                                                 void *callbackExtraArgs)
{
    if (m_AMDeviceMountImage)
        return m_AMDeviceMountImage(device, imagePath, options, callback, callbackExtraArgs);
    return -1;
}

am_res_t MobileDeviceLib::deviceUninstallApplication(int serviceFd, CFStringRef bundleId,
                                                           CFDictionaryRef options,
                                                           AMDeviceInstallApplicationCallback callback,
                                                           void *callbackExtraArgs)
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

char *MobileDeviceLib::errorString(am_res_t error)
{
    if (m_AMDErrorString)
        return m_AMDErrorString(error);
    return 0;
}

CFStringRef MobileDeviceLib::misErrorStringForErrorCode(am_res_t error)
{
    if (m_MISCopyErrorStringForErrorCode)
        return m_MISCopyErrorStringForErrorCode(error);
    return NULL;
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

am_res_t MobileDeviceLib::deviceSecureStartService(AMDeviceRef device, CFStringRef serviceName, ServiceConnRef *fdRef)
{
    if (m_AMDeviceSecureStartService)
        return m_AMDeviceSecureStartService(device, serviceName, nullptr, fdRef);
    return 0;
}

int MobileDeviceLib::deviceSecureTransferApplicationPath(int zero, AMDeviceRef device, CFURLRef url, CFDictionaryRef dict, AMDeviceSecureInstallApplicationCallback callback, int args)
{
    int returnCode = -1;
    if (m_AMDeviceSecureTransferPath)
        returnCode = m_AMDeviceSecureTransferPath(zero, device, url, dict, callback, args);
    return returnCode;
}

int MobileDeviceLib::deviceSecureInstallApplication(int zero, AMDeviceRef device, CFURLRef url, CFDictionaryRef options, AMDeviceSecureInstallApplicationCallback callback, int arg)
{
    int returnCode = -1;
    if (m_AMDeviceSecureInstallApplication) {
        returnCode = m_AMDeviceSecureInstallApplication(zero, device, url, options, callback, arg);
    }
    return returnCode;
}

int MobileDeviceLib::deviceConnectionGetSocket(ServiceConnRef ref) {
    int fd = 0;
    if (m_AMDServiceConnectionGetSocket)
        fd = m_AMDServiceConnectionGetSocket(ref);
    return fd;
}

int MobileDeviceLib::serviceConnectionSend(ServiceConnRef ref, const void *data, size_t size)
{
    int bytesSent = -1;
    if (m_AMDServiceConnectionSend) {
        bytesSent = m_AMDServiceConnectionSend(ref, data, size);
    }
    return bytesSent;
}

int MobileDeviceLib::serviceConnectionReceive(ServiceConnRef ref, void *data, size_t size)
{
    int bytestRead = 0;
    if (m_AMDServiceConnectionReceive) {
        bytestRead = m_AMDServiceConnectionReceive(ref, data, size);
    }
    return bytestRead;
}

void MobileDeviceLib::serviceConnectionInvalidate(ServiceConnRef serviceConnRef)
{
    if (m_AMDServiceConnectionInvalidate)
        m_AMDServiceConnectionInvalidate(serviceConnRef);
}

bool MobileDeviceLib::deviceIsAtLeastVersionOnPlatform(AMDeviceRef device, CFDictionaryRef versions)
{
    if (m_AMDeviceIsAtLeastVersionOnPlatform)
        return m_AMDeviceIsAtLeastVersionOnPlatform(device, versions);
    return false;
}

} // IOS
