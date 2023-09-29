// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designersettings.h"

#include <utils/qtcsettings.h>

using namespace Utils;

namespace QmlDesigner {

namespace DesignerSettingsGroupKey {
    const char QML_SETTINGS_GROUP[] = "QML";
    const char QML_DESIGNER_SETTINGS_GROUP[] = "Designer";
}

DesignerSettings::DesignerSettings(QtcSettings *settings) :
    m_settings(settings)
{
    fromSettings(settings);
}

void DesignerSettings::insert(const QByteArray &key, const QVariant &value)
{
    QMutexLocker locker(&m_mutex);
    m_cache.insert(key, value);
    toSettings(m_settings);
}

void DesignerSettings::insert(const QHash<QByteArray, QVariant> &settingsHash)
{
    QMutexLocker locker(&m_mutex);
    m_cache.insert(settingsHash);
    toSettings(m_settings);
}

QVariant DesignerSettings::value(const QByteArray &key, const QVariant &defaultValue) const
{
    QMutexLocker locker(&m_mutex);
    return m_cache.value(key, defaultValue);
}

void DesignerSettings::restoreValue(QtcSettings *settings, const QByteArray &key, const QVariant &defaultValue)
{
    m_cache.insert(key, settings->value(key, defaultValue));
}

void DesignerSettings::fromSettings(QtcSettings *settings)
{
    settings->beginGroup(DesignerSettingsGroupKey::QML_SETTINGS_GROUP);
    settings->beginGroup(DesignerSettingsGroupKey::QML_DESIGNER_SETTINGS_GROUP);

    restoreValue(settings, DesignerSettingsKey::ITEMSPACING, 6);
    restoreValue(settings, DesignerSettingsKey::CONTAINERPADDING, 8);
    restoreValue(settings, DesignerSettingsKey::CANVASWIDTH, 40000);
    restoreValue(settings, DesignerSettingsKey::CANVASHEIGHT, 40000);
    restoreValue(settings, DesignerSettingsKey::ROOT_ELEMENT_INIT_WIDTH, 640);
    restoreValue(settings, DesignerSettingsKey::ROOT_ELEMENT_INIT_HEIGHT, 480);
    restoreValue(settings, DesignerSettingsKey::WARNING_FOR_FEATURES_IN_DESIGNER, true);
    restoreValue(settings, DesignerSettingsKey::WARNING_FOR_QML_FILES_INSTEAD_OF_UIQML_FILES, true);
    restoreValue(settings, DesignerSettingsKey::WARNING_FOR_DESIGNER_FEATURES_IN_EDITOR, false);
    restoreValue(settings, DesignerSettingsKey::SHOW_DEBUGVIEW, false);
    restoreValue(settings, DesignerSettingsKey::ENABLE_DEBUGVIEW, false);
    restoreValue(settings, DesignerSettingsKey::ALWAYS_SAVE_IN_CRUMBLEBAR, false);
    restoreValue(settings, DesignerSettingsKey::USE_DEFAULT_PUPPET, true);
    restoreValue(settings, DesignerSettingsKey::TYPE_OF_QSTR_FUNCTION, 0);
    restoreValue(settings, DesignerSettingsKey::PUPPET_DEFAULT_DIRECTORY);
    restoreValue(settings, DesignerSettingsKey::PUPPET_TOPLEVEL_BUILD_DIRECTORY);
    restoreValue(settings, DesignerSettingsKey::CONTROLS_STYLE);
    restoreValue(settings, DesignerSettingsKey::SHOW_PROPERTYEDITOR_WARNINGS, false);
    restoreValue(settings, DesignerSettingsKey::ENABLE_MODEL_EXCEPTION_OUTPUT, false);
    restoreValue(settings, DesignerSettingsKey::PUPPET_KILL_TIMEOUT, 30000); // this has no ui at the moment
    restoreValue(settings, DesignerSettingsKey::DEBUG_PUPPET, QString());
    restoreValue(settings, DesignerSettingsKey::FORWARD_PUPPET_OUTPUT, QString());
    restoreValue(settings, DesignerSettingsKey::REFORMAT_UI_QML_FILES, true);
    restoreValue(settings, DesignerSettingsKey::IGNORE_DEVICE_PIXEL_RATIO, false);
    restoreValue(settings, DesignerSettingsKey::NAVIGATOR_SHOW_ONLY_VISIBLE_ITEMS, true);
    restoreValue(settings, DesignerSettingsKey::NAVIGATOR_REVERSE_ITEM_ORDER, false);
    restoreValue(settings, DesignerSettingsKey::ENABLE_TIMELINEVIEW, true);
    restoreValue(settings, DesignerSettingsKey::COLOR_PALETTE_RECENT, QStringList());
    restoreValue(settings, DesignerSettingsKey::COLOR_PALETTE_FAVORITE, QStringList());
    restoreValue(settings, DesignerSettingsKey::ALWAYS_DESIGN_MODE, true);
    restoreValue(settings, DesignerSettingsKey::DISABLE_ITEM_LIBRARY_UPDATE_TIMER, false);
    restoreValue(settings, DesignerSettingsKey::ASK_BEFORE_DELETING_ASSET, true);
    restoreValue(settings, DesignerSettingsKey::EDIT3DVIEW_BACKGROUND_COLOR,
                 QStringList{"#222222", "#999999"});
    restoreValue(settings, DesignerSettingsKey::EDIT3DVIEW_GRID_COLOR, "#cccccc");
    restoreValue(settings, DesignerSettingsKey::EDIT3DVIEW_SNAP_ABSOLUTE, true);
    restoreValue(settings, DesignerSettingsKey::EDIT3DVIEW_SNAP_ENABLED, false);
    restoreValue(settings, DesignerSettingsKey::EDIT3DVIEW_SNAP_POSITION, true);
    restoreValue(settings, DesignerSettingsKey::EDIT3DVIEW_SNAP_POSITION_INTERVAL, 50.);
    restoreValue(settings, DesignerSettingsKey::EDIT3DVIEW_SNAP_ROTATION, true);
    restoreValue(settings, DesignerSettingsKey::EDIT3DVIEW_SNAP_ROTATION_INTERVAL, 5.);
    restoreValue(settings, DesignerSettingsKey::EDIT3DVIEW_SNAP_SCALE, true);
    restoreValue(settings, DesignerSettingsKey::EDIT3DVIEW_SNAP_SCALE_INTERVAL, 10.);
    restoreValue(settings, DesignerSettingsKey::SMOOTH_RENDERING, false);
    restoreValue(settings, DesignerSettingsKey::SHOW_DEBUG_SETTINGS, false);
    restoreValue(settings, DesignerSettingsKey::EDITOR_ZOOM_FACTOR, 1.0);
    restoreValue(settings, DesignerSettingsKey::ACTIONS_MERGE_TEMPLATE_ENABLED, false);
    restoreValue(settings, DesignerSettingsKey::DOWNLOADABLE_BUNDLES_URL,
                 "https://cdn.qt.io/designstudio/bundles");
    restoreValue(settings, DesignerSettingsKey::CONTENT_LIBRARY_NEW_FLAG_EXPIRATION_DAYS, 3);

    settings->endGroup();
    settings->endGroup();
}

void DesignerSettings::storeValue(QtcSettings *settings, const QByteArray &key, const QVariant &value) const
{
    if (key.isEmpty())
        return;
    settings->setValue(key, value);
}

void DesignerSettings::toSettings(QtcSettings *settings) const
{
    settings->beginGroup(DesignerSettingsGroupKey::QML_SETTINGS_GROUP);
    settings->beginGroup(DesignerSettingsGroupKey::QML_DESIGNER_SETTINGS_GROUP);

    QHash<QByteArray, QVariant>::const_iterator i = m_cache.constBegin();
    while (i != m_cache.constEnd()) {
        storeValue(settings, i.key(), i.value());
        ++i;
    }

    settings->endGroup();
    settings->endGroup();
}

} // namespace QmlDesigner
