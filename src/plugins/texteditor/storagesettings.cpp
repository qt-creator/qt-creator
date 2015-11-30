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

#include "storagesettings.h"

#include <utils/settingsutils.h>

#include <QSettings>
#include <QString>

namespace TextEditor {

static const char cleanWhitespaceKey[] = "cleanWhitespace";
static const char inEntireDocumentKey[] = "inEntireDocument";
static const char addFinalNewLineKey[] = "addFinalNewLine";
static const char cleanIndentationKey[] = "cleanIndentation";
static const char groupPostfix[] = "StorageSettings";

StorageSettings::StorageSettings()
    : m_cleanWhitespace(true),
      m_inEntireDocument(false),
      m_addFinalNewLine(true),
      m_cleanIndentation(true)
{
}

void StorageSettings::toSettings(const QString &category, QSettings *s) const
{
    Utils::toSettings(QLatin1String(groupPostfix), category, s, this);
}

void StorageSettings::fromSettings(const QString &category, const QSettings *s)
{
    *this = StorageSettings();
    Utils::fromSettings(QLatin1String(groupPostfix), category, s, this);
}

void StorageSettings::toMap(const QString &prefix, QVariantMap *map) const
{
    map->insert(prefix + QLatin1String(cleanWhitespaceKey), m_cleanWhitespace);
    map->insert(prefix + QLatin1String(inEntireDocumentKey), m_inEntireDocument);
    map->insert(prefix + QLatin1String(addFinalNewLineKey), m_addFinalNewLine);
    map->insert(prefix + QLatin1String(cleanIndentationKey), m_cleanIndentation);
}

void StorageSettings::fromMap(const QString &prefix, const QVariantMap &map)
{
    m_cleanWhitespace =
        map.value(prefix + QLatin1String(cleanWhitespaceKey), m_cleanWhitespace).toBool();
    m_inEntireDocument =
        map.value(prefix + QLatin1String(inEntireDocumentKey), m_inEntireDocument).toBool();
    m_addFinalNewLine =
        map.value(prefix + QLatin1String(addFinalNewLineKey), m_addFinalNewLine).toBool();
    m_cleanIndentation =
        map.value(prefix + QLatin1String(cleanIndentationKey), m_cleanIndentation).toBool();
}

bool StorageSettings::equals(const StorageSettings &ts) const
{
    return m_addFinalNewLine == ts.m_addFinalNewLine
        && m_cleanWhitespace == ts.m_cleanWhitespace
        && m_inEntireDocument == ts.m_inEntireDocument
        && m_cleanIndentation == ts.m_cleanIndentation;
}

} // namespace TextEditor
