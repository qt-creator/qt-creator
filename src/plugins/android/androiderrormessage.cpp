/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
