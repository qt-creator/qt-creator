/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "designersettings.h"
#include "qmldesignerconstants.h"

#include <QSettings>

using namespace QmlDesigner;

DesignerSettings::DesignerSettings()
    : itemSpacing(0),
    containerPadding(0),
    canvasWidth(10000),
    canvasHeight(10000),
    warningsInDesigner(true),
    designerWarningsInEditor(false),
    showDebugView(false),
    enableDebugView(false),
    alwaysSaveInCrumbleBar(false),
    useOnlyFallbackPuppet(true)
{}

void DesignerSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(QmlDesigner::Constants::QML_SETTINGS_GROUP));
    settings->beginGroup(QLatin1String(QmlDesigner::Constants::QML_DESIGNER_SETTINGS_GROUP));
    itemSpacing = settings->value(
            QLatin1String(QmlDesigner::Constants::QML_ITEMSPACING_KEY), QVariant(6)).toInt();
    containerPadding = settings->value(
            QLatin1String(QmlDesigner::Constants::QML_CONTAINERPADDING_KEY), QVariant(8)).toInt();
    canvasWidth = settings->value(QLatin1String(QmlDesigner::Constants::QML_CANVASWIDTH_KEY), QVariant(10000)).toInt();
    canvasHeight = settings->value(QLatin1String(QmlDesigner::Constants::QML_CANVASHEIGHT_KEY), QVariant(10000)).toInt();
    warningsInDesigner = settings->value(
            QLatin1String(QmlDesigner::Constants::QML_WARNIN_FOR_FEATURES_IN_DESIGNER_KEY), QVariant(true)).toBool();
    designerWarningsInEditor = settings->value(
            QLatin1String(QmlDesigner::Constants::QML_WARNIN_FOR_DESIGNER_FEATURES_IN_EDITOR_KEY), QVariant(false)).toBool();
    showDebugView = settings->value(
            QLatin1String(QmlDesigner::Constants::QML_SHOW_DEBUGVIEW), QVariant(false)).toBool();
    enableDebugView = settings->value(
            QLatin1String(QmlDesigner::Constants::QML_ENABLE_DEBUGVIEW), QVariant(false)).toBool();
    alwaysSaveInCrumbleBar = settings->value(
                QLatin1String(QmlDesigner::Constants::QML_ALWAYS_SAFE_IN_CRUMBLEBAR), QVariant(false)).toBool();
    useOnlyFallbackPuppet = settings->value(
                QLatin1String(QmlDesigner::Constants::QML_USE_ONLY_FALLBACK_PUPPET), QVariant(true)).toBool();
    puppetFallbackDirectory = settings->value(
                QLatin1String(QmlDesigner::Constants::QML_PUPPET_FALLBACK_DIRECTORY)).toString();
    puppetToplevelBuildDirectory = settings->value(
                QLatin1String(QmlDesigner::Constants::QML_PUPPET_TOPLEVEL_BUILD_DIRECTORY)).toString();
    controlsStyle = settings->value(
                QLatin1String(QmlDesigner::Constants::QML_CONTROLS_STYLE)).toString();


    settings->endGroup();
    settings->endGroup();
}

void DesignerSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(QmlDesigner::Constants::QML_SETTINGS_GROUP));
    settings->beginGroup(QLatin1String(QmlDesigner::Constants::QML_DESIGNER_SETTINGS_GROUP));
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_ITEMSPACING_KEY), itemSpacing);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_CONTAINERPADDING_KEY), containerPadding);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_CANVASWIDTH_KEY), canvasWidth);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_CANVASHEIGHT_KEY), canvasHeight);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_WARNIN_FOR_FEATURES_IN_DESIGNER_KEY), warningsInDesigner);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_WARNIN_FOR_DESIGNER_FEATURES_IN_EDITOR_KEY), designerWarningsInEditor);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_SHOW_DEBUGVIEW), showDebugView);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_ENABLE_DEBUGVIEW), enableDebugView);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_ALWAYS_SAFE_IN_CRUMBLEBAR), alwaysSaveInCrumbleBar);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_USE_ONLY_FALLBACK_PUPPET), useOnlyFallbackPuppet);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_PUPPET_FALLBACK_DIRECTORY), puppetFallbackDirectory);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_PUPPET_TOPLEVEL_BUILD_DIRECTORY), puppetToplevelBuildDirectory);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_CONTROLS_STYLE), controlsStyle);

    settings->endGroup();
    settings->endGroup();
}

bool DesignerSettings::equals(const DesignerSettings &other) const
{
    return containerPadding == other.containerPadding
            && canvasWidth == other.canvasWidth
            && canvasHeight == other.canvasHeight
            && warningsInDesigner == other.warningsInDesigner
            && designerWarningsInEditor == other.designerWarningsInEditor
            && showDebugView == other.showDebugView
            && enableDebugView == other.enableDebugView
            && alwaysSaveInCrumbleBar == other.alwaysSaveInCrumbleBar
            && useOnlyFallbackPuppet == other.useOnlyFallbackPuppet
            && puppetFallbackDirectory == other.puppetFallbackDirectory
            && puppetToplevelBuildDirectory == other.puppetToplevelBuildDirectory
            && controlsStyle == other.controlsStyle;
}
