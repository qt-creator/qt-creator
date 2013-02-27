/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "subversionsettings.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QSettings>

namespace Subversion {
namespace Internal {

const QLatin1String SubversionSettings::useAuthenticationKey("Authentication");
const QLatin1String SubversionSettings::userKey("User");
const QLatin1String SubversionSettings::passwordKey("Password");
const QLatin1String SubversionSettings::spaceIgnorantAnnotationKey("SpaceIgnorantAnnotation");

SubversionSettings::SubversionSettings()
{
    setSettingsGroup(QLatin1String("Subversion"));
    declareKey(binaryPathKey, QLatin1String("svn" QTC_HOST_EXE_SUFFIX));
    declareKey(logCountKey, 1000);
    declareKey(useAuthenticationKey, false);
    declareKey(userKey, QLatin1String(""));
    declareKey(passwordKey, QLatin1String(""));
    declareKey(spaceIgnorantAnnotationKey, true);
}

bool SubversionSettings::hasAuthentication() const
{
    return boolValue(useAuthenticationKey) && !stringValue(userKey).isEmpty();
}

int SubversionSettings::timeOutMs() const
{
    return 1000 * intValue(timeoutKey);
}

void SubversionSettings::readLegacySettings(const QSettings *settings)
{
    const QString keyRoot = settingsGroup() + QLatin1Char('/');
    const QString oldBinaryPathKey = keyRoot + QLatin1String("Command");
    const QString oldPromptOnSubmitKey = keyRoot + QLatin1String("PromptForSubmit");
    const QString oldTimeoutKey = keyRoot + QLatin1String("TimeOut");
    if (settings->contains(oldBinaryPathKey))
        this->setValue(binaryPathKey, settings->value(oldBinaryPathKey).toString());
    if (settings->contains(oldPromptOnSubmitKey))
        this->setValue(promptOnSubmitKey, settings->value(oldPromptOnSubmitKey).toBool());
    if (settings->contains(oldTimeoutKey))
        this->setValue(timeoutKey, settings->value(oldTimeoutKey).toInt());
}

} // namespace Internal
} // namespace Subversion
