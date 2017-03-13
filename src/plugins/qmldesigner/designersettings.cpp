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

#include "designersettings.h"

#include <qmldesignerplugin.h>

#include <QSettings>

namespace QmlDesigner {

namespace DesignerSettingsGroupKey {
    const char QML_SETTINGS_GROUP[] = "QML";
    const char QML_DESIGNER_SETTINGS_GROUP[] = "Designer";
}

DesignerSettings::DesignerSettings()
{
}

void DesignerSettings::restoreValue(QSettings *settings, const QByteArray &key, const QVariant &defaultValue)
{
    insert(key, settings->value(QString::fromLatin1(key), defaultValue));
}

void DesignerSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(DesignerSettingsGroupKey::QML_SETTINGS_GROUP));
    settings->beginGroup(QLatin1String(DesignerSettingsGroupKey::QML_DESIGNER_SETTINGS_GROUP));

    restoreValue(settings, DesignerSettingsKey::ITEMSPACING, 6);
    restoreValue(settings, DesignerSettingsKey::CONTAINERPADDING, 8);
    restoreValue(settings, DesignerSettingsKey::CANVASWIDTH, 10000);
    restoreValue(settings, DesignerSettingsKey::CANVASHEIGHT, 10000);
    restoreValue(settings, DesignerSettingsKey::WARNING_FOR_FEATURES_IN_DESIGNER, true);
    restoreValue(settings, DesignerSettingsKey::WARNING_FOR_QML_FILES_INSTEAD_OF_UIQML_FILES, true);
    restoreValue(settings, DesignerSettingsKey::WARNING_FOR_DESIGNER_FEATURES_IN_EDITOR, false);
    restoreValue(settings, DesignerSettingsKey::SHOW_DEBUGVIEW, false);
    restoreValue(settings, DesignerSettingsKey::ENABLE_DEBUGVIEW, false);
    restoreValue(settings, DesignerSettingsKey::ALWAYS_SAFE_IN_CRUMBLEBAR, false);
    restoreValue(settings, DesignerSettingsKey::USE_ONLY_FALLBACK_PUPPET, true);
    restoreValue(settings, DesignerSettingsKey::TYPE_OF_QSTR_FUNCTION, 0);
    restoreValue(settings, DesignerSettingsKey::PUPPET_FALLBACK_DIRECTORY);
    restoreValue(settings, DesignerSettingsKey::PUPPET_TOPLEVEL_BUILD_DIRECTORY);
    restoreValue(settings, DesignerSettingsKey::CONTROLS_STYLE);
    restoreValue(settings, DesignerSettingsKey::SHOW_PROPERTYEDITOR_WARNINGS, false);
    restoreValue(settings, DesignerSettingsKey::ENABLE_MODEL_EXCEPTION_OUTPUT, false);
    restoreValue(settings, DesignerSettingsKey::PUPPET_KILL_TIMEOUT, 3000); // this has no ui at the moment
    restoreValue(settings, DesignerSettingsKey::DEBUG_PUPPET, QString());
    restoreValue(settings, DesignerSettingsKey::FORWARD_PUPPET_OUTPUT, QString());
    restoreValue(settings, DesignerSettingsKey::REFORMAT_UI_QML_FILES, true);
    restoreValue(settings, DesignerSettingsKey::IGNORE_DEVICE_PIXEL_RATIO, false);

    settings->endGroup();
    settings->endGroup();
}

void DesignerSettings::storeValue(QSettings *settings, const QByteArray &key, const QVariant &value) const
{
    if (key.isEmpty())
        return;
    settings->setValue(QString::fromLatin1(key), value);
}

void DesignerSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(DesignerSettingsGroupKey::QML_SETTINGS_GROUP));
    settings->beginGroup(QLatin1String(DesignerSettingsGroupKey::QML_DESIGNER_SETTINGS_GROUP));

    QHash<QByteArray, QVariant>::const_iterator i = constBegin();
    while (i != constEnd()) {
        storeValue(settings, i.key(), i.value());
        ++i;
    }

    settings->endGroup();
    settings->endGroup();
}

QVariant DesignerSettings::getValue(const QByteArray &key)
{
    DesignerSettings settings = QmlDesignerPlugin::instance()->settings();
    return settings.value(key);
}

void DesignerSettings::setValue(const QByteArray &key, const QVariant &value)
{
    DesignerSettings settings = QmlDesignerPlugin::instance()->settings();
    settings.insert(key, value);
    QmlDesignerPlugin::instance()->setSettings(settings);
}

} // namespace QmlDesigner
