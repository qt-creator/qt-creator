/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtCore/QSettings>

using namespace QmlDesigner;

static const char *qmlGroup = "Qml";
static const char *qmlDesignerGroup = "Designer";
static const char *snapToGridKey = "SnapToGrid";
static const char *showBoundingRectanglesKey = "ShowBoundingRectangles";

void DesignerSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(qmlGroup));
    settings->beginGroup(QLatin1String(qmlDesignerGroup));
    snapToGrid = settings->value(QLatin1String(snapToGridKey), false).toBool();
    showBoundingRectangles = settings->value(
            QLatin1String(showBoundingRectanglesKey), false).toBool();
    settings->endGroup();
    settings->endGroup();
}

void DesignerSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(qmlGroup));
    settings->beginGroup(QLatin1String(qmlDesignerGroup));
    settings->setValue(QLatin1String(snapToGridKey), snapToGrid);
    settings->setValue(QLatin1String(showBoundingRectanglesKey),
                       showBoundingRectangles);
    settings->endGroup();
    settings->endGroup();
}

bool DesignerSettings::equals(const DesignerSettings &other) const
{
    return snapToGrid == other.snapToGrid
            && showBoundingRectangles == other.showBoundingRectangles;
}
