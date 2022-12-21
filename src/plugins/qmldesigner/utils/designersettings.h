// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignerutils_global.h"

#include <QHash>
#include <QVariant>
#include <QByteArray>
#include <QMutex>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace DesignerSettingsKey {
const char ITEMSPACING[] = "ItemSpacing";
const char CONTAINERPADDING[] = "ContainerPadding";
const char CANVASWIDTH[] = "CanvasWidth";
const char CANVASHEIGHT[] = "CanvasHeight";
const char ROOT_ELEMENT_INIT_WIDTH[] = "RootElementInitWidth";
const char ROOT_ELEMENT_INIT_HEIGHT[] = "RootElementInitHeight";
const char WARNING_FOR_FEATURES_IN_DESIGNER[] = "WarnAboutQtQuickFeaturesInDesigner";
const char WARNING_FOR_QML_FILES_INSTEAD_OF_UIQML_FILES[] = "WarnAboutQmlFilesInsteadOfUiQmlFiles";
const char WARNING_FOR_DESIGNER_FEATURES_IN_EDITOR[] = "WarnAboutQtQuickDesignerFeaturesInCodeEditor";
const char SHOW_DEBUGVIEW[] = "ShowQtQuickDesignerDebugView";
const char ENABLE_DEBUGVIEW[] = "EnableQtQuickDesignerDebugView";
const char EDIT3DVIEW_BACKGROUND_COLOR[] = "Edit3DViewBackgroundColor";
const char EDIT3DVIEW_GRID_COLOR[] = "Edit3DViewGridLineColor";
const char ALWAYS_SAVE_IN_CRUMBLEBAR[] = "AlwaysSaveInCrumbleBar";
const char USE_DEFAULT_PUPPET[] = "UseDefaultQml2Puppet";
const char PUPPET_TOPLEVEL_BUILD_DIRECTORY[] = "PuppetToplevelBuildDirectory";
const char PUPPET_DEFAULT_DIRECTORY[] = "PuppetDefaultDirectory";
const char CONTROLS_STYLE[] = "ControlsStyle";
const char TYPE_OF_QSTR_FUNCTION[] = "TypeOfQsTrFunction";
const char SHOW_PROPERTYEDITOR_WARNINGS[] = "ShowPropertyEditorWarnings";
const char ENABLE_MODEL_EXCEPTION_OUTPUT[] = "WarnException";
const char PUPPET_KILL_TIMEOUT[] = "PuppetKillTimeout";
const char DEBUG_PUPPET[] = "DebugPuppet";
const char FORWARD_PUPPET_OUTPUT[] = "ForwardPuppetOutput";
const char NAVIGATOR_SHOW_ONLY_VISIBLE_ITEMS[] = "NavigatorShowOnlyVisibleItems";
const char NAVIGATOR_REVERSE_ITEM_ORDER[] = "NavigatorReverseItemOrder";
const char REFORMAT_UI_QML_FILES[] = "ReformatUiQmlFiles";        /* These settings are not exposed in ui. */
const char IGNORE_DEVICE_PIXEL_RATIO[] = "IgnoreDevicePixelRaio"; /* The settings can be used to turn off the feature, if there are serious issues */
const char SHOW_DEBUG_SETTINGS[] = "ShowDebugSettings";
const char ENABLE_TIMELINEVIEW[] = "EnableTimelineView";
const char COLOR_PALETTE_RECENT[] = "ColorPaletteRecent";
const char COLOR_PALETTE_FAVORITE[] = "ColorPaletteFavorite";
const char ALWAYS_DESIGN_MODE[] = "AlwaysDesignMode";
const char DISABLE_ITEM_LIBRARY_UPDATE_TIMER[] = "DisableItemLibraryUpdateTimer";
const char ASK_BEFORE_DELETING_ASSET[] = "AskBeforeDeletingAsset";
const char SMOOTH_RENDERING[] = "SmoothRendering";
const char OLD_STATES_EDITOR[] = "ForceOldStatesEditor";
const char EDITOR_ZOOM_FACTOR[] = "EditorZoomFactor";
}

class QMLDESIGNERUTILS_EXPORT DesignerSettings
{
public:
    DesignerSettings(QSettings *settings);

    void insert(const QByteArray &key, const QVariant &value);
    void insert(const QHash<QByteArray, QVariant> &settingsHash);
    QVariant value(const QByteArray &key, const QVariant &defaultValue = {}) const;

private:
    void fromSettings(QSettings *);
    void toSettings(QSettings *) const;

    void restoreValue(QSettings *settings, const QByteArray &key,
        const QVariant &defaultValue = QVariant());
    void storeValue(QSettings *settings, const QByteArray &key, const QVariant &value) const;

    QSettings *m_settings;
    QHash<QByteArray, QVariant> m_cache;
    mutable QMutex m_mutex;
};

} // namespace QmlDesigner
