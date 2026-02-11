// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignersettings_global.h"

#include <QHash>
#include <QVariant>
#include <QByteArray>
#include <QMutex>

namespace Utils { class QtcSettings; }

namespace QmlDesigner {

namespace DesignerSettingsKey {
inline constexpr char ItemSpacing[] = "ItemSpacing";
inline constexpr char ContainerPadding[] = "ContainerPadding";
inline constexpr char CanvasWidth[] = "CanvasWidth";
inline constexpr char CanvasHeight[] = "CanvasHeight";
inline constexpr char RootElementInitWidth[] = "RootElementInitWidth";
inline constexpr char RootElementInitHeight[] = "RootElementInitHeight";
inline constexpr char WarnAboutQtQuickFeaturesInDesigner[] = "WarnAboutQtQuickFeaturesInDesigner";
inline constexpr char WarnAboutQmlFilesInsteadOfUiQmlFiles[] = "WarnAboutQmlFilesInsteadOfUiQmlFiles";
inline constexpr char WarnAboutQtQuickDesignerFeaturesInCodeEditor[] = "WarnAboutQtQuickDesignerFeaturesInCodeEditor";
inline constexpr char ShowQtQuickDesignerDebugView[] = "ShowQtQuickDesignerDebugView";
inline constexpr char EnableQtQuickDesignerDebugView[] = "EnableQtQuickDesignerDebugView";
inline constexpr char Edit3DViewBackgroundColor[] = "Edit3DViewBackgroundColor";
inline constexpr char Edit3DViewGridLineColor[] = "Edit3DViewGridLineColor";
inline constexpr char Edit3DViewSnapAbsolute[] = "Edit3DViewSnapAbsolute";
inline constexpr char Edit3DViewSnapEnabled[] = "Edit3DViewSnapEnabled";
inline constexpr char Edit3DViewSnapPosition[] = "Edit3DViewSnapPosition";
inline constexpr char Edit3DViewSnapPositionInterval[] = "Edit3DViewSnapPositionInterval";
inline constexpr char Edit3DViewSnapRotation[] = "Edit3DViewSnapRotation";
inline constexpr char Edit3DViewSnapRotationInterval[] = "Edit3DViewSnapRotationInterval";
inline constexpr char Edit3DViewSnapScale[] = "Edit3DViewSnapScale";
inline constexpr char Edit3DViewSnapScaleInterval[] = "Edit3DViewSnapScaleInterval";
inline constexpr char AlwaysSaveInCrumbleBar[] = "AlwaysSaveInCrumbleBar";
inline constexpr char ControlsStyle[] = "ControlsStyle";
inline constexpr char TypeOfQsTrFunction[] = "TypeOfQsTrFunction";
inline constexpr char ShowPropertyEditorWarnings[] = "ShowPropertyEditorWarnings";
inline constexpr char WarnException[] = "WarnException";
inline constexpr char PuppetKillTimeout[] = "PuppetKillTimeout";
inline constexpr char DebugPuppet[] = "DebugPuppet";
inline constexpr char ForwardPuppetOutput[] = "ForwardPuppetOutput";
inline constexpr char NavigatorShowReferenceNodes[] = "NavigatorShowReferenceNodes";
inline constexpr char NavigatorShowOnlyVisibleItems[] = "NavigatorShowOnlyVisibleItems";
inline constexpr char NavigatorReverseItemOrder[] = "NavigatorReverseItemOrder";
inline constexpr char NavigatorColorizeIcons[] = "NavigatorColorizeIcons";
inline constexpr char ReformatUiQmlFiles[] = "ReformatUiQmlFiles"; /* These settings are not exposed in ui. */
inline constexpr char IgnoreDevicePixelRatio[]
    = "IgnoreDevicePixelRaio"; /* The settings can be used to turn off the feature, if there are serious issues */
inline constexpr char ShowDebugSettings[] = "ShowDebugSettings";
inline constexpr char EnableTimelineView[] = "EnableTimelineView";
inline constexpr char EnableDockWidgetContentMinSize[] = "EnableDockWidgetContentMinSize";
inline constexpr char ColorPaletteRecent[] = "ColorPaletteRecent";
inline constexpr char ColorPaletteFavorite[] = "ColorPaletteFavorite";
inline constexpr char AlwaysDesignMode[] = "AlwaysDesignMode";
inline constexpr char DisableItemLibraryUpdateTimer[] = "DisableItemLibraryUpdateTimer";
inline constexpr char AskBeforeDeletingAsset[] = "AskBeforeDeletingAsset";
inline constexpr char AskBeforeDeletingContentLibFile[] = "AskBeforeDeletingContentLibFile";
inline constexpr char SmoothRendering[] = "SmoothRendering";
inline constexpr char EditorZoomFactor[] = "EditorZoomFactor";
inline constexpr char ActionsMergeTemplateEnabled[] = "ActionsMergeTemplateEnabled";
inline constexpr char DownloadableBundlesLocation[] = "DownloadableBundlesLocation";
inline constexpr char ContentLibraryNewFlagExpirationInDays[] = "ContentLibraryNewFlagExpirationInDays";
inline constexpr char GroqApiKey[] = "GroqApiKey";
}

class QMLDESIGNERSETTINGS_EXPORT DesignerSettings
{
public:
    DesignerSettings();

    void insert(const QByteArray &key, const QVariant &value);
    void insert(const QHash<QByteArray, QVariant> &settingsHash);
    QVariant value(const QByteArray &key, const QVariant &defaultValue = {}) const;

private:
    void fromSettings(Utils::QtcSettings *);
    void toSettings(Utils::QtcSettings *) const;

    void restoreValue(Utils::QtcSettings *settings, const QByteArray &key,
        const QVariant &defaultValue = QVariant());
    void storeValue(Utils::QtcSettings *settings, const QByteArray &key, const QVariant &value) const;

    Utils::QtcSettings *m_settings;
    QHash<QByteArray, QVariant> m_cache;
    mutable QMutex m_mutex;
};

QMLDESIGNERSETTINGS_EXPORT DesignerSettings &designerSettings();

} // namespace QmlDesigner
