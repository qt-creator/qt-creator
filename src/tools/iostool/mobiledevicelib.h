// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "iostooltypes.h"

#include <QLibrary>
#include <QStringList>


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

namespace Ios {
extern "C" {

#ifndef MOBILE_DEV_DIRECT_LINK
class AMDeviceNotification;
typedef const AMDeviceNotification *AMDeviceNotificationRef;
class AMDevice;
struct AMDeviceNotificationCallbackInfo {
    AMDevice *_device;
    unsigned int _message;
    AMDeviceNotification *_subscription;
};

enum DeviceInterfaceType {
    UNKNOWN = 0,
    WIRED,
    WIFI
};

typedef unsigned int am_res_t; // mach_error_t

typedef void (MDEV_API *AMDeviceNotificationCallback)(AMDeviceNotificationCallbackInfo *, void *);
typedef am_res_t (MDEV_API *AMDeviceInstallApplicationCallback)(CFDictionaryRef, void *);
typedef mach_error_t (MDEV_API *AMDeviceSecureInstallApplicationCallback)(CFDictionaryRef, int);


typedef AMDevice *AMDeviceRef;
#endif
typedef void (MDEV_API *AMDeviceMountImageCallback)(CFDictionaryRef, int);

typedef void (MDEV_API *AMDSetLogLevelPtr)(int);
typedef am_res_t (MDEV_API *AMDeviceNotificationSubscribePtr)(AMDeviceNotificationCallback,
                                                               unsigned int, unsigned int, void *,
                                                               const AMDeviceNotification **);
typedef am_res_t (MDEV_API *AMDeviceNotificationUnsubscribePtr)(void *);
typedef int (MDEV_API* AMDeviceGetInterfaceTypePtr)(AMDeviceRef device);
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
typedef am_res_t (MDEV_API *AMDeviceUninstallApplicationPtr)(ServiceSocket, CFStringRef, CFDictionaryRef,
                                                                AMDeviceInstallApplicationCallback,
                                                                void*);
typedef am_res_t (MDEV_API *AMDeviceLookupApplicationsPtr)(AMDeviceRef, CFDictionaryRef, CFDictionaryRef *);
typedef char * (MDEV_API *AMDErrorStringPtr)(am_res_t);
typedef CFStringRef (MDEV_API *MISCopyErrorStringForErrorCodePtr)(am_res_t);
typedef am_res_t (MDEV_API *USBMuxConnectByPortPtr)(unsigned int, int, ServiceSocket*);
// secure Api's
typedef am_res_t (MDEV_API *AMDeviceSecureStartServicePtr)(AMDeviceRef, CFStringRef, unsigned int *, ServiceConnRef *);
typedef int (MDEV_API *AMDeviceSecureTransferPathPtr)(int, AMDeviceRef, CFURLRef, CFDictionaryRef, AMDeviceSecureInstallApplicationCallback, int);
typedef int (MDEV_API *AMDeviceSecureInstallApplicationBundlePtr)(AMDeviceRef, CFURLRef, CFDictionaryRef, AMDeviceSecureInstallApplicationCallback, int zero);
typedef int (MDEV_API *AMDeviceSecureInstallApplicationPtr)(int, AMDeviceRef, CFURLRef, CFDictionaryRef, AMDeviceSecureInstallApplicationCallback, int);
typedef int (MDEV_API *AMDServiceConnectionGetSocketPtr)(ServiceConnRef);

typedef int (MDEV_API *AMDServiceConnectionSendPtr)(ServiceConnRef, const void *, size_t);
typedef int (MDEV_API *AMDServiceConnectionReceivePtr)(ServiceConnRef, void *, size_t);
typedef void (MDEV_API *AMDServiceConnectionInvalidatePtr)(ServiceConnRef);
typedef bool (MDEV_API *AMDeviceIsAtLeastVersionOnPlatformPtr)(AMDeviceRef, CFDictionaryRef);
}

class MobileDeviceLib {
    MobileDeviceLib();
    bool load();

public:
    MobileDeviceLib( const MobileDeviceLib& ) = delete;
    MobileDeviceLib &operator=( const MobileDeviceLib& ) = delete;

    static MobileDeviceLib &instance();

    bool isLoaded();
    QStringList errors();

    void setLogLevel(int i) ;
    am_res_t deviceNotificationSubscribe(AMDeviceNotificationCallback callback,
                                         unsigned int v1, unsigned int v2, void *v3,
                                         const AMDeviceNotification **handle);
    am_res_t deviceNotificationUnsubscribe(void *handle);
    int deviceGetInterfaceType(AMDeviceRef device);
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
    am_res_t deviceUninstallApplication(int, CFStringRef, CFDictionaryRef,
                                                                    AMDeviceInstallApplicationCallback,
                                                                    void*);
    am_res_t deviceLookupApplications(AMDeviceRef, CFDictionaryRef, CFDictionaryRef *);
    char *errorString(am_res_t error);
    CFStringRef misErrorStringForErrorCode(am_res_t error);
    am_res_t connectByPort(unsigned int connectionId, int port, ServiceSocket *resFd);

    void addError(const QString &msg);
    void addError(const char *msg);

    // Secure API's
    am_res_t deviceSecureStartService(AMDeviceRef, CFStringRef, ServiceConnRef *);
    int deviceConnectionGetSocket(ServiceConnRef);
    int deviceSecureTransferApplicationPath(int, AMDeviceRef, CFURLRef,
                                            CFDictionaryRef,
                                            AMDeviceSecureInstallApplicationCallback callback, int);

    int deviceSecureInstallApplicationBundle(int zero,
                                             AMDeviceRef device,
                                             CFURLRef url,
                                             CFDictionaryRef options,
                                             AMDeviceSecureInstallApplicationCallback callback);

    int deviceSecureInstallApplication(int zero, AMDeviceRef device, CFURLRef url,
                                       CFDictionaryRef options,
                                       AMDeviceSecureInstallApplicationCallback callback, int arg);

    // Use MobileDevice API's to communicate with service launched on the device.
    // The communication is encrypted if ServiceConnRef::sslContext is valid.
    int serviceConnectionSend(ServiceConnRef ref, const void *data, size_t size);
    int serviceConnectionReceive(ServiceConnRef ref, void *data, size_t size);
    void serviceConnectionInvalidate(ServiceConnRef serviceConnRef);
    bool deviceIsAtLeastVersionOnPlatform(AMDeviceRef device, CFDictionaryRef versions);

    QStringList m_errors;

private:
    template<typename T> void resolveFunction(const char *functionName, T &functionPtr);

private:
    QLibrary m_mobileDeviceLib;
    AMDSetLogLevelPtr m_AMDSetLogLevel;
    AMDeviceNotificationSubscribePtr m_AMDeviceNotificationSubscribe;
    AMDeviceNotificationUnsubscribePtr m_AMDeviceNotificationUnsubscribe;
    AMDeviceGetInterfaceTypePtr m_AMDeviceGetInterfaceType;
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
    AMDeviceSecureStartServicePtr m_AMDeviceSecureStartService;
    AMDeviceSecureTransferPathPtr m_AMDeviceSecureTransferPath;
    AMDeviceSecureInstallApplicationBundlePtr m_AMDeviceSecureInstallApplicationBundle;
    AMDeviceSecureInstallApplicationPtr m_AMDeviceSecureInstallApplication;
    AMDServiceConnectionGetSocketPtr m_AMDServiceConnectionGetSocket;
    AMDServiceConnectionSendPtr m_AMDServiceConnectionSend;
    AMDServiceConnectionReceivePtr m_AMDServiceConnectionReceive;
    AMDServiceConnectionInvalidatePtr m_AMDServiceConnectionInvalidate;
    AMDeviceIsAtLeastVersionOnPlatformPtr m_AMDeviceIsAtLeastVersionOnPlatform;
    AMDeviceUninstallApplicationPtr m_AMDeviceUninstallApplication;
    AMDeviceLookupApplicationsPtr m_AMDeviceLookupApplications;
    AMDErrorStringPtr m_AMDErrorString;
    MISCopyErrorStringForErrorCodePtr m_MISCopyErrorStringForErrorCode;
    USBMuxConnectByPortPtr m_USBMuxConnectByPort;
};
}
