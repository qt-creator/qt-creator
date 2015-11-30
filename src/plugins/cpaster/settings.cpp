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

#include "settings.h"
#include "pastebindotcomprotocol.h"

#include <utils/environment.h>

#include <QSettings>

static const char groupC[] = "CodePaster";
static const char userNameKeyC[] = "UserName";
static const char expiryDaysKeyC[] = "ExpiryDays";
static const char defaultProtocolKeyC[] = "DefaultProtocol";
static const char copyToClipboardKeyC[] = "CopyToClipboard";
static const char displayOutputKeyC[] = "DisplayOutput";

namespace CodePaster {

bool Settings::equals(const Settings &rhs) const
{
    return copyToClipboard == rhs.copyToClipboard && displayOutput == rhs.displayOutput
           && expiryDays == rhs.expiryDays && username == rhs.username
           && protocol == rhs.protocol;
}

void Settings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(groupC));
    settings->setValue(QLatin1String(userNameKeyC), username);
    settings->setValue(QLatin1String(defaultProtocolKeyC), protocol);
    settings->setValue(QLatin1String(expiryDaysKeyC), expiryDays);
    settings->setValue(QLatin1String(copyToClipboardKeyC), copyToClipboard);
    settings->setValue(QLatin1String(displayOutputKeyC), displayOutput);
    settings->endGroup();
}

void Settings::fromSettings(const QSettings *settings)
{
    const QString rootKey = QLatin1String(groupC) + QLatin1Char('/');
    const QString defaultUser = Utils::Environment::systemEnvironment().userName();
    expiryDays = settings->value(rootKey + QLatin1String(expiryDaysKeyC), 1).toInt();
    username = settings->value(rootKey + QLatin1String(userNameKeyC), defaultUser).toString();
    protocol = settings->value(rootKey + QLatin1String(defaultProtocolKeyC), PasteBinDotComProtocol::protocolName()).toString();
    copyToClipboard = settings->value(rootKey + QLatin1String(copyToClipboardKeyC), true).toBool();
    displayOutput = settings->value(rootKey + QLatin1String(displayOutputKeyC), true).toBool();
}

} // namespace CodePaster
