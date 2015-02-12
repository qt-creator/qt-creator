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

#include "marginsettings.h"

#include <QSettings>
#include <QString>
#include <QVariantMap>

static const char showWrapColumnKey[] = "ShowMargin";
static const char wrapColumnKey[] = "MarginColumn";
static const char groupPostfix[] = "MarginSettings";

using namespace TextEditor;

MarginSettings::MarginSettings()
    : m_showMargin(false)
    , m_marginColumn(80)
{
}

void MarginSettings::toSettings(const QString &category, QSettings *s) const
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);
    s->beginGroup(group);
    s->setValue(QLatin1String(showWrapColumnKey), m_showMargin);
    s->setValue(QLatin1String(wrapColumnKey), m_marginColumn);
    s->endGroup();
}

void MarginSettings::fromSettings(const QString &category, const QSettings *s)
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);
    group += QLatin1Char('/');

    *this = MarginSettings(); // Assign defaults

    m_showMargin = s->value(group + QLatin1String(showWrapColumnKey), m_showMargin).toBool();
    m_marginColumn = s->value(group + QLatin1String(wrapColumnKey), m_marginColumn).toInt();
}

void MarginSettings::toMap(const QString &prefix, QVariantMap *map) const
{
    map->insert(prefix + QLatin1String(showWrapColumnKey), m_showMargin);
    map->insert(prefix + QLatin1String(wrapColumnKey), m_marginColumn);
}

void MarginSettings::fromMap(const QString &prefix, const QVariantMap &map)
{
    m_showMargin = map.value(prefix + QLatin1String(showWrapColumnKey), m_showMargin).toBool();
    m_marginColumn = map.value(prefix + QLatin1String(wrapColumnKey), m_marginColumn).toInt();
}

bool MarginSettings::equals(const MarginSettings &other) const
{
    return m_showMargin == other.m_showMargin
        && m_marginColumn == other.m_marginColumn
        ;
}
