// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androiderrormessage.h"
#include "androidtr.h"

#include <QObject>

namespace Android {
namespace Internal {

QString AndroidErrorMessage::getMessage(ErrorCode errorCode, const QVariantList &parameters)
{
    Q_UNUSED(parameters)
    switch (errorCode) {
    case SDKInstallationError:
        return Tr::tr("Android: SDK installation error 0x%1").arg(errorCode, 0, 16);

    case NDKInstallationError:
        return Tr::tr("Android: NDK installation error 0x%1").arg(errorCode, 0, 16);

    case JavaInstallationError:
        return Tr::tr("Android: Java installation error 0x%1").arg(errorCode, 0, 16);

    case AntInstallationError:
        return Tr::tr("Android: ant installation error 0x%1").arg(errorCode, 0, 16);

    case AdbInstallationError:
        return Tr::tr("Android: adb installation error 0x%1").arg(errorCode, 0, 16);

    case DeviceConnectionError:
        return Tr::tr("Android: Device connection error 0x%1").arg(errorCode, 0, 16);

    case DevicePermissionError:
        return Tr::tr("Android: Device permission error 0x%1").arg(errorCode, 0, 16);

    case DeviceAuthorizationError:
        return Tr::tr("Android: Device authorization error 0x%1").arg(errorCode, 0, 16);

    case DeviceAPILevelError:
        return Tr::tr("Android: Device API level not supported: error 0x%1").arg(errorCode, 0, 16);

    default:
        return Tr::tr("Android: Unknown error 0x%1").arg(errorCode, 0, 16);
    }
}

} // namespace Internal
} // namespace Android
