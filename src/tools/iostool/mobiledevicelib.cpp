// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mobiledevicelib.h"
#include "cfutils.h"

#include <QDebug>
#include <QLoggingCategory>
#include <QUrl>

namespace {
Q_LOGGING_CATEGORY(loggingCategory, "qtc.iostool.mobiledevicelib")
}

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
        const QStringList errs = errors();
        for (const QString &msg : errs)
            addError(msg);
    }
    setLogLevel(5);
}

template<typename T>
void MobileDeviceLib::resolveFunction(const char *functionName, T &functionPtr) {
    functionPtr = reinterpret_cast<T>(m_mobileDeviceLib.resolve(functionName));
    if (!functionPtr) {
        addError(QLatin1String("MobileDeviceLib does not define ") + functionName);
    }
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
    m_mobileDeviceLib.setFileName("/System/Library/PrivateFrameworks/MobileDevice.framework/MobileDevice");
    if (!m_mobileDeviceLib.load())
        return false;

    resolveFunction("AMDSetLogLevel", m_AMDSetLogLevel);
    resolveFunction("AMDeviceSecureInstallApplicationBundle", m_AMDeviceSecureInstallApplicationBundle);
    resolveFunction("AMDeviceNotificationSubscribe", m_AMDeviceNotificationSubscribe);
    resolveFunction("AMDeviceNotificationUnsubscribe", m_AMDeviceNotificationUnsubscribe);
    resolveFunction("AMDeviceGetInterfaceType", m_AMDeviceGetInterfaceType);
    resolveFunction("AMDeviceCopyValue", m_AMDeviceCopyValue);
    resolveFunction("AMDeviceGetConnectionID", m_AMDeviceGetConnectionID);
    resolveFunction("AMDeviceCopyDeviceIdentifier", m_AMDeviceCopyDeviceIdentifier);
    resolveFunction("AMDeviceConnect", m_AMDeviceConnect);
    resolveFunction("AMDevicePair", m_AMDevicePair);
    resolveFunction("AMDeviceIsPaired", m_AMDeviceIsPaired);
    resolveFunction("AMDeviceValidatePairing", m_AMDeviceValidatePairing);
    resolveFunction("AMDeviceStartSession", m_AMDeviceStartSession);
    resolveFunction("AMDeviceStopSession", m_AMDeviceStopSession);
    resolveFunction("AMDeviceDisconnect", m_AMDeviceDisconnect);
    resolveFunction("AMDeviceMountImage", m_AMDeviceMountImage);
    resolveFunction("AMDeviceSecureStartService", m_AMDeviceSecureStartService);
    resolveFunction("AMDeviceSecureTransferPath", m_AMDeviceSecureTransferPath);
    resolveFunction("AMDeviceSecureInstallApplication", m_AMDeviceSecureInstallApplication);
    resolveFunction("AMDServiceConnectionGetSocket", m_AMDServiceConnectionGetSocket);
    resolveFunction("AMDeviceUninstallApplication", m_AMDeviceUninstallApplication);
    resolveFunction("AMDServiceConnectionSend", m_AMDServiceConnectionSend);
    resolveFunction("AMDServiceConnectionReceive", m_AMDServiceConnectionReceive);
    resolveFunction("AMDServiceConnectionInvalidate", m_AMDServiceConnectionInvalidate);
    resolveFunction("AMDeviceIsAtLeastVersionOnPlatform", m_AMDeviceIsAtLeastVersionOnPlatform);
    resolveFunction("AMDeviceLookupApplications", m_AMDeviceLookupApplications);
    resolveFunction("AMDErrorString", m_AMDErrorString);
    resolveFunction("MISCopyErrorStringForErrorCode", m_MISCopyErrorStringForErrorCode);
    resolveFunction("USBMuxConnectByPort", m_USBMuxConnectByPort);
#endif
    return true;
}

bool MobileDeviceLib::isLoaded()
{
    return m_mobileDeviceLib.isLoaded();
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
                                                      unsigned int v1,
                                                      unsigned int v2,
                                                      void *callbackArgs,
                                                      const AMDeviceNotification **handle)
{
    if (m_AMDeviceNotificationSubscribe)
        return m_AMDeviceNotificationSubscribe(callback, v1, v2, callbackArgs, handle);
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

CFPropertyListRef MobileDeviceLib::deviceCopyValue(AMDeviceRef device,
                                                   CFStringRef group,
                                                   CFStringRef key)
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

am_res_t MobileDeviceLib::deviceMountImage(AMDeviceRef device,
                                           CFStringRef imagePath,
                                           CFDictionaryRef options,
                                           AMDeviceMountImageCallback callback,
                                           void *callbackExtraArgs)
{
    if (m_AMDeviceMountImage)
        return m_AMDeviceMountImage(device, imagePath, options, callback, callbackExtraArgs);
    return -1;
}

am_res_t MobileDeviceLib::deviceUninstallApplication(int serviceFd,
                                                     CFStringRef bundleId,
                                                     CFDictionaryRef options,
                                                     AMDeviceInstallApplicationCallback callback,
                                                     void *callbackExtraArgs)
{
    if (m_AMDeviceUninstallApplication)
        return m_AMDeviceUninstallApplication(serviceFd,
                                              bundleId,
                                              options,
                                              callback,
                                              callbackExtraArgs);
    return -1;
}

am_res_t MobileDeviceLib::deviceLookupApplications(AMDeviceRef device,
                                                   CFDictionaryRef options,
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
    qCWarning(loggingCategory) << "MobileDeviceLib ERROR:" << msg;
    m_errors << QLatin1String("MobileDeviceLib ERROR:") << msg;
}

void MobileDeviceLib::addError(const char *msg)
{
    addError(QLatin1String(msg));
}

am_res_t MobileDeviceLib::deviceSecureStartService(AMDeviceRef device,
                                                   CFStringRef serviceName,
                                                   ServiceConnRef *fdRef)
{
    if (m_AMDeviceSecureStartService)
        return m_AMDeviceSecureStartService(device, serviceName, nullptr, fdRef);
    return 0;
}

int MobileDeviceLib::deviceSecureTransferApplicationPath(
    int zero,
    AMDeviceRef device,
    CFURLRef url,
    CFDictionaryRef dict,
    AMDeviceSecureInstallApplicationCallback callback,
    int args)
{
    int returnCode = -1;

    if (m_AMDeviceSecureTransferPath)
        returnCode = m_AMDeviceSecureTransferPath(zero, device, url, dict, callback, args);
    return returnCode;
}

int MobileDeviceLib::deviceSecureInstallApplicationBundle(
    int zero,
    AMDeviceRef device,
    CFURLRef url,
    CFDictionaryRef options,
    AMDeviceSecureInstallApplicationCallback callback)
{
    int returnCode = -1;

    if (m_AMDeviceSecureInstallApplicationBundle) {
        returnCode = m_AMDeviceSecureInstallApplicationBundle(device, url, options, callback, zero);
    }
    return returnCode;
}

int MobileDeviceLib::deviceSecureInstallApplication(int zero,
                                                    AMDeviceRef device,
                                                    CFURLRef url,
                                                    CFDictionaryRef options,
                                                    AMDeviceSecureInstallApplicationCallback callback,
                                                    int arg)
{
    int returnCode = -1;
    if (m_AMDeviceSecureInstallApplication) {
        returnCode = m_AMDeviceSecureInstallApplication(zero, device, url, options, callback, arg);
    }
    return returnCode;
}

int MobileDeviceLib::deviceConnectionGetSocket(ServiceConnRef ref)
{
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

} // namespace Ios
