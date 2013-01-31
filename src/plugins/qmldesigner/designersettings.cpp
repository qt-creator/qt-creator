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

#include "designersettings.h"
#include "qmldesignerconstants.h"

#include <QSettings>

using namespace QmlDesigner;

DesignerSettings::DesignerSettings()
    : openDesignMode(QmlDesigner::Constants::QML_OPENDESIGNMODE_DEFAULT),
    itemSpacing(0),
    snapMargin(0),
    canvasWidth(10000),
    canvasHeight(10000),
    warningsInDesigner(true),
    designerWarningsInEditor(false)
{}

void DesignerSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(QmlDesigner::Constants::QML_SETTINGS_GROUP));
    settings->beginGroup(QLatin1String(QmlDesigner::Constants::QML_DESIGNER_SETTINGS_GROUP));
    openDesignMode = settings->value(
            QLatin1String(QmlDesigner::Constants::QML_OPENDESIGNMODE_SETTINGS_KEY),
            bool(QmlDesigner::Constants::QML_OPENDESIGNMODE_DEFAULT)).toBool();
    itemSpacing = settings->value(
            QLatin1String(QmlDesigner::Constants::QML_ITEMSPACING_KEY), QVariant(0)).toInt();
    snapMargin = settings->value(
            QLatin1String(QmlDesigner::Constants::QML_SNAPMARGIN_KEY), QVariant(0)).toInt();
    canvasWidth = settings->value(QLatin1String(QmlDesigner::Constants::QML_CANVASWIDTH_KEY), QVariant(10000)).toInt();
    canvasHeight = settings->value(QLatin1String(QmlDesigner::Constants::QML_CANVASHEIGHT_KEY), QVariant(10000)).toInt();
    warningsInDesigner = settings->value(
            QLatin1String(QmlDesigner::Constants::QML_WARNIN_FOR_FEATURES_IN_DESIGNER_KEY), QVariant(true)).toBool();
    designerWarningsInEditor = settings->value(
            QLatin1String(QmlDesigner::Constants::QML_WARNIN_FOR_DESIGNER_FEATURES_IN_EDITOR_KEY), QVariant(false)).toBool();

    settings->endGroup();
    settings->endGroup();
}

void DesignerSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(QmlDesigner::Constants::QML_SETTINGS_GROUP));
    settings->beginGroup(QLatin1String(QmlDesigner::Constants::QML_DESIGNER_SETTINGS_GROUP));
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_OPENDESIGNMODE_SETTINGS_KEY), openDesignMode);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_ITEMSPACING_KEY), itemSpacing);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_SNAPMARGIN_KEY), snapMargin);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_CANVASWIDTH_KEY), canvasWidth);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_CANVASHEIGHT_KEY), canvasHeight);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_WARNIN_FOR_FEATURES_IN_DESIGNER_KEY), warningsInDesigner);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_WARNIN_FOR_DESIGNER_FEATURES_IN_EDITOR_KEY), designerWarningsInEditor);

    settings->endGroup();
    settings->endGroup();
}

bool DesignerSettings::equals(const DesignerSettings &other) const
{
    return openDesignMode == other.openDesignMode
            && snapMargin == other.snapMargin
            && canvasWidth == other.canvasWidth
            && canvasHeight == other.canvasHeight
            && warningsInDesigner == other.warningsInDesigner
            && designerWarningsInEditor == other.designerWarningsInEditor;
}
