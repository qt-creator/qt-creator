// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "androiderrormessage.h"

#include <QObject>

namespace Android {
namespace Internal {

QString AndroidErrorMessage::getMessage(ErrorCode errorCode, const QVariantList &parameters)
{
    Q_UNUSED(parameters)
    switch (errorCode) {
    case SDKInstallationError:
        return tr("Android: SDK installation error 0x%1").arg(errorCode, 0, 16);

    case NDKInstallationError:
        return tr("Android: NDK installation error 0x%1").arg(errorCode, 0, 16);

    case JavaInstallationError:
        return tr("Android: Java installation error 0x%1").arg(errorCode, 0, 16);

    case AntInstallationError:
        return tr("Android: ant installation error 0x%1").arg(errorCode, 0, 16);

    case AdbInstallationError:
        return tr("Android: adb installation error 0x%1").arg(errorCode, 0, 16);

    case DeviceConnectionError:
        return tr("Android: Device connection error 0x%1").arg(errorCode, 0, 16);

    case DevicePermissionError:
        return tr("Android: Device permission error 0x%1").arg(errorCode, 0, 16);

    case DeviceAuthorizationError:
        return tr("Android: Device authorization error 0x%1").arg(errorCode, 0, 16);

    case DeviceAPILevelError:
        return tr("Android: Device API level not supported: error 0x%1").arg(errorCode, 0, 16);

    default:
        return tr("Android: Unknown error 0x%1").arg(errorCode, 0, 16);
    }
}

} // namespace Internal
} // namespace Android
