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

#include "qbsprojectmanagersettings.h"

#include <coreplugin/icore.h>

namespace QbsProjectManager {
namespace Internal {

static QString settingsId() { return QLatin1String("QbsProjectManager"); }
static QString useCreatorDirKey() { return QLatin1String("useCreatorDir"); }

QbsProjectManagerSettings::QbsProjectManagerSettings()
{
    readSettings();
}

void QbsProjectManagerSettings::readSettings()
{
    QSettings * const settings = Core::ICore::settings();
    settings->beginGroup(settingsId());
    m_useCreatorSettings = settings->value(useCreatorDirKey(), true).toBool();
    settings->endGroup();
}

void QbsProjectManagerSettings::doWriteSettings()
{
    QSettings * const settings = Core::ICore::settings();
    settings->beginGroup(settingsId());
    settings->setValue(useCreatorDirKey(), m_useCreatorSettings);
    settings->endGroup();
}

QbsProjectManagerSettings &QbsProjectManagerSettings::instance()
{
    static QbsProjectManagerSettings settings;
    return settings;
}

void QbsProjectManagerSettings::setUseCreatorSettingsDirForQbs(bool useCreatorDir)
{
    if (instance().m_useCreatorSettings == useCreatorDir)
        return;
    instance().m_useCreatorSettings = useCreatorDir;
    emit instance().settingsBaseChanged();
}

bool QbsProjectManagerSettings::useCreatorSettingsDirForQbs()
{
    return instance().m_useCreatorSettings;
}

QString QbsProjectManagerSettings::qbsSettingsBaseDir()
{
    return instance().m_useCreatorSettings ? Core::ICore::userResourcePath() : QString();
}

} // namespace Internal
} // namespace QbsProjectManager
