// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignersettings_global.h"

#include <utils/aspects.h>

#include <QHash>
#include <QVariant>
#include <QByteArray>
#include <QMutex>

namespace Utils { class QtcSettings; }

namespace QmlDesigner {

class QMLDESIGNERSETTINGS_EXPORT DesignerSettings final : public Utils::AspectContainer
{
public:
    DesignerSettings();
    ~DesignerSettings() final;

    Utils::IntegerAspect itemSpacing{this};
    Utils::IntegerAspect containerPadding{this};
    Utils::IntegerAspect canvasWidth{this};
    Utils::IntegerAspect canvasHeight{this};
    Utils::IntegerAspect rootElementInitWidth{this};
    Utils::IntegerAspect rootElementInitHeight{this};
    Utils::BoolAspect warningForFeaturesInDesigner{this};
    Utils::BoolAspect warningForQmlFilesInsteadOfUiQmlFiles{this};
    Utils::BoolAspect warningForDesignerFeaturesInEditor{this};
    Utils::BoolAspect showDebugView{this};
    Utils::BoolAspect enableDebugView{this};
    Utils::StringListAspect edit3DViewBackgroundColor{this};
    Utils::StringAspect edit3DViewGridLineColor{this};
    Utils::BoolAspect edit3DViewSnapAbsolute{this};
    Utils::BoolAspect edit3DViewSnapEnabled{this};
    Utils::BoolAspect edit3DViewSnapPosition{this};
    Utils::DoubleAspect edit3DViewSnapPositionInterval{this};
    Utils::BoolAspect edit3DViewSnapRotation{this};
    Utils::DoubleAspect edit3DViewSnapRotationInterval{this};
    Utils::BoolAspect edit3DViewSnapScale{this};
    Utils::DoubleAspect edit3DViewSnapScaleInterval{this};
    Utils::BoolAspect particleMode{this};
    Utils::BoolAspect alwaysSaveInCrumbleBar{this};
    Utils::StringAspect controlsStyle{this};
    Utils::SelectionAspect controls2Style{this};
    Utils::SelectionAspect typeOfQsTrFunction{this};
    Utils::BoolAspect showPropertyEditorWarnings{this};
    Utils::BoolAspect enableModelExceptionOutput{this};
    Utils::IntegerAspect puppetKillTimeout{this};
    Utils::SelectionAspect debugPuppet{this};
    Utils::SelectionAspect forwardPuppetOutput{this};
    Utils::BoolAspect navigatorShowReferenceNodes{this};
    Utils::BoolAspect navigatorShowOnlyVisibleItems{this};
    Utils::BoolAspect navigatorReverseItemOrder{this};
    Utils::BoolAspect navigatorColorizeIcons{this};
    Utils::BoolAspect reformatUiQmlFiles{this};
    Utils::BoolAspect ignoreDevicePixelRatio{this};
    Utils::BoolAspect showDebugSettings{this};
    Utils::BoolAspect enableTimelineView{this};
    Utils::BoolAspect enableDockWidgetContentMinSize{this};
    Utils::StringListAspect colorPaletteRecent{this};
    Utils::StringListAspect colorPaletteFavorite{this};
    Utils::BoolAspect alwaysDesignMode{this};
    Utils::BoolAspect disableItemLibraryUpdateTimer{this};
    Utils::BoolAspect askBeforeDeletingAsset{this};
    Utils::BoolAspect askBeforeDeletingContentLibFile{this};
    Utils::BoolAspect smoothRendering{this};
    Utils::DoubleAspect editorZoomFactor{this};
    Utils::BoolAspect actionsMergeTemplateEnabled{this};
    Utils::StringAspect downloadableBundlesLocation{this};
    Utils::IntegerAspect contentLibraryNewFlagExpirationInDays{this};
    Utils::StringAspect groqApiKey{this};

private:
    void apply() override;

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
