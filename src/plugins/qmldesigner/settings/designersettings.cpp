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

DesignerSettings::DesignerSettings()
    : m_settings(&userSettings())
{
    fromSettings(m_settings);
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

    restoreValue(settings, DesignerSettingsKey::ItemSpacing, 6);
    restoreValue(settings, DesignerSettingsKey::ContainerPadding, 8);
    restoreValue(settings, DesignerSettingsKey::CanvasWidth, 40000);
    restoreValue(settings, DesignerSettingsKey::CanvasHeight, 40000);
    restoreValue(settings, DesignerSettingsKey::RootElementInitWidth, 640);
    restoreValue(settings, DesignerSettingsKey::RootElementInitHeight, 480);
    restoreValue(settings, DesignerSettingsKey::WarnAboutQtQuickFeaturesInDesigner, true);
    restoreValue(settings, DesignerSettingsKey::WarnAboutQmlFilesInsteadOfUiQmlFiles, true);
    restoreValue(settings, DesignerSettingsKey::WarnAboutQtQuickDesignerFeaturesInCodeEditor, false);
    restoreValue(settings, DesignerSettingsKey::ShowQtQuickDesignerDebugView, false);
    restoreValue(settings, DesignerSettingsKey::EnableQtQuickDesignerDebugView, false);
    restoreValue(settings, DesignerSettingsKey::AlwaysSaveInCrumbleBar, false);
    restoreValue(settings, DesignerSettingsKey::TypeOfQsTrFunction, 0);
    restoreValue(settings, DesignerSettingsKey::ControlsStyle);
    restoreValue(settings, DesignerSettingsKey::ShowPropertyEditorWarnings, false);
    restoreValue(settings, DesignerSettingsKey::WarnException, false);
    restoreValue(settings, DesignerSettingsKey::PuppetKillTimeout, 30000); // this has no ui at the moment
    restoreValue(settings, DesignerSettingsKey::DebugPuppet, QString());
    restoreValue(settings, DesignerSettingsKey::ForwardPuppetOutput, QString());
    restoreValue(settings, DesignerSettingsKey::ReformatUiQmlFiles, true);
    restoreValue(settings, DesignerSettingsKey::IgnoreDevicePixelRatio, false);
    restoreValue(settings, DesignerSettingsKey::NavigatorShowReferenceNodes, true);
    restoreValue(settings, DesignerSettingsKey::NavigatorShowOnlyVisibleItems, true);
    restoreValue(settings, DesignerSettingsKey::NavigatorReverseItemOrder, false);
    restoreValue(settings, DesignerSettingsKey::NavigatorColorizeIcons, false);
    restoreValue(settings, DesignerSettingsKey::EnableTimelineView, true);
    restoreValue(settings, DesignerSettingsKey::EnableDockWidgetContentMinSize, true);
    restoreValue(settings, DesignerSettingsKey::ColorPaletteRecent, QStringList());
    restoreValue(settings, DesignerSettingsKey::ColorPaletteFavorite, QStringList());
    restoreValue(settings, DesignerSettingsKey::AlwaysDesignMode, true);
    restoreValue(settings, DesignerSettingsKey::DisableItemLibraryUpdateTimer, false);
    restoreValue(settings, DesignerSettingsKey::AskBeforeDeletingAsset, true);
    restoreValue(settings, DesignerSettingsKey::AskBeforeDeletingContentLibFile, true);
    restoreValue(settings, DesignerSettingsKey::Edit3DViewBackgroundColor,
                 QStringList{"#222222", "#999999"});
    restoreValue(settings, DesignerSettingsKey::Edit3DViewGridLineColor, "#cccccc");
    restoreValue(settings, DesignerSettingsKey::Edit3DViewSnapAbsolute, true);
    restoreValue(settings, DesignerSettingsKey::Edit3DViewSnapEnabled, false);
    restoreValue(settings, DesignerSettingsKey::Edit3DViewSnapPosition, true);
    restoreValue(settings, DesignerSettingsKey::Edit3DViewSnapPositionInterval, 50.);
    restoreValue(settings, DesignerSettingsKey::Edit3DViewSnapRotation, true);
    restoreValue(settings, DesignerSettingsKey::Edit3DViewSnapRotationInterval, 5.);
    restoreValue(settings, DesignerSettingsKey::Edit3DViewSnapScale, true);
    restoreValue(settings, DesignerSettingsKey::Edit3DViewSnapScaleInterval, 10.);
    restoreValue(settings, DesignerSettingsKey::SmoothRendering, true);
    restoreValue(settings, DesignerSettingsKey::ShowDebugSettings, false);
    restoreValue(settings, DesignerSettingsKey::EditorZoomFactor, 1.0);
    restoreValue(settings, DesignerSettingsKey::ActionsMergeTemplateEnabled, false);
    restoreValue(settings, DesignerSettingsKey::DownloadableBundlesLocation,
                 "https://cdn.qt.io/designstudio/bundles");
    restoreValue(settings, DesignerSettingsKey::ContentLibraryNewFlagExpirationInDays, 3);
    restoreValue(settings, DesignerSettingsKey::GroqApiKey, "");

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

DesignerSettings &designerSettings()
{
    static DesignerSettings theDesignerSettings;
    return theDesignerSettings;
}

} // namespace QmlDesigner
