/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "designersettings.h"
#include "qmldesignerconstants.h"

#include <QtCore/QSettings>

using namespace QmlDesigner;

DesignerSettings::DesignerSettings()
    : openDesignMode(QmlDesigner::Constants::QML_OPENDESIGNMODE_DEFAULT),
    itemSpacing(0),
    snapMargin(0),
    canvasWidth(10000),
    canvasHeight(10000)
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
    settings->endGroup();
    settings->endGroup();
}

bool DesignerSettings::equals(const DesignerSettings &other) const
{
    return openDesignMode == other.openDesignMode
            && snapMargin == other.snapMargin
            && canvasWidth == other.canvasWidth
            && canvasHeight == other.canvasHeight;
}
