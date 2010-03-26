/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "settings.h"

#include <QtCore/QVariant>
#include <QtCore/QSettings>

static const char groupC[] = "CodePaster";
static const char userNameKeyC[] = "UserName";
static const char defaultProtocolKeyC[] = "DefaultProtocol";
static const char copyToClipboardKeyC[] = "CopyToClipboard";
static const char displayOutputKeyC[] = "DisplayOutput";
static const char defaultProtocolC[] = "CodePaster";

namespace CodePaster {

Settings::Settings() : copyToClipboard(true), displayOutput(true)
{
}

bool Settings::equals(const Settings &rhs) const
{
    return copyToClipboard == rhs.copyToClipboard && displayOutput == rhs.displayOutput
            && username == rhs.username && protocol == rhs.protocol;
}

void Settings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(groupC));
    settings->setValue(QLatin1String(userNameKeyC), username);
    settings->setValue(QLatin1String(defaultProtocolKeyC), protocol);
    settings->setValue(QLatin1String(copyToClipboardKeyC), copyToClipboard);
    settings->setValue(QLatin1String(displayOutputKeyC), displayOutput);
    settings->endGroup();
}

void Settings::fromSettings(const QSettings *settings)
{
    const QString rootKey = QLatin1String(groupC) + QLatin1Char('/');
#ifdef Q_OS_WIN
    const QString defaultUser = qgetenv("USERNAME");
#else
    const QString defaultUser = qgetenv("USER");
#endif
    username = settings->value(rootKey + QLatin1String(userNameKeyC), defaultUser).toString();
    protocol = settings->value(rootKey + QLatin1String(defaultProtocolKeyC), QLatin1String(defaultProtocolC)).toString();
    copyToClipboard = settings->value(rootKey + QLatin1String(copyToClipboardKeyC), true).toBool();
    displayOutput = settings->value(rootKey + QLatin1String(displayOutputKeyC), true).toBool();
}

} // namespace CodePaster
