/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "designersettings.h"
#include "qmldesignerconstants.h"

#include <QtCore/QSettings>

using namespace QmlDesigner;

DesignerSettings::DesignerSettings()
    : openDesignMode(QmlDesigner::Constants::QML_OPENDESIGNMODE_DEFAULT)
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
    enableContextPane = settings->value(
            QLatin1String(QmlDesigner::Constants::QML_CONTEXTPANE_KEY), QVariant(0)).toBool();

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
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_CONTEXTPANE_KEY), enableContextPane);

    settings->endGroup();
    settings->endGroup();
}

bool DesignerSettings::equals(const DesignerSettings &other) const
{
    return openDesignMode == other.openDesignMode
            && snapMargin == other.snapMargin
            && itemSpacing == other.itemSpacing
            && enableContextPane == other.enableContextPane;

}
