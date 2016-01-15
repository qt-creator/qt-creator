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

#include "androiderrormessage.h"

#include <QObject>

namespace Android {
namespace Internal {

QString AndroidErrorMessage::getMessage(ErrorCode errorCode, const QVariantList &parameters)
{
    Q_UNUSED(parameters);
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
