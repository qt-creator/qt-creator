/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "settingshandler.h"

#include "mcusupportconstants.h"
#include "mcusupportsdk.h"

#include <coreplugin/icore.h>
#include <utils/filepath.h>

namespace McuSupport::Internal {

using Utils::FilePath;

static FilePath packagePathFromSettings(const QString &settingsKey,
                                        QSettings &settings,
                                        const FilePath &defaultPath)
{
    const QString key = QLatin1String(Constants::SETTINGS_GROUP) + '/'
                        + QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX) + settingsKey;
    const QString path = settings.value(key, defaultPath.toUserOutput()).toString();
    return FilePath::fromUserInput(path);
}

FilePath SettingsHandler::getPath(const QString &settingsKey,
                                  QSettings::Scope scope,
                                  const Utils::FilePath &defaultPath) const
{
    return packagePathFromSettings(settingsKey, *Core::ICore::settings(scope), defaultPath);
}

bool SettingsHandler::write(const QString &settingsKey,
                            const Utils::FilePath &path,
                            const Utils::FilePath &defaultPath) const
{
    const FilePath savedPath = packagePathFromSettings(settingsKey,
                                                       *Core::ICore::settings(QSettings::UserScope),
                                                       defaultPath);
    const QString key = QLatin1String(Constants::SETTINGS_GROUP) + '/'
                        + QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX) + settingsKey;
    Core::ICore::settings()->setValueWithDefault(key,
                                                 path.toUserOutput(),
                                                 defaultPath.toUserOutput());

    return savedPath != path;
}
} // namespace McuSupport::Internal
