/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "storagesettings.h"

#include <QSettings>
#include <QString>

namespace TextEditor {

static const char * const cleanWhitespaceKey = "cleanWhitespace";
static const char * const inEntireDocumentKey = "inEntireDocument";
static const char * const addFinalNewLineKey = "addFinalNewLine";
static const char * const groupPostfix = "StorageSettings";

StorageSettings::StorageSettings()
    : m_cleanWhitespace(true),
      m_inEntireDocument(false),
      m_addFinalNewLine(true)
{
}

void StorageSettings::toSettings(const QString &category, QSettings *s) const
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);
    s->beginGroup(group);
    s->setValue(QLatin1String(cleanWhitespaceKey), m_cleanWhitespace);
    s->setValue(QLatin1String(inEntireDocumentKey), m_inEntireDocument);
    s->setValue(QLatin1String(addFinalNewLineKey), m_addFinalNewLine);
    s->endGroup();
}

void StorageSettings::fromSettings(const QString &category, const QSettings *s)
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);
    group += QLatin1Char('/');
    m_cleanWhitespace = s->value(group + QLatin1String(cleanWhitespaceKey), m_cleanWhitespace).toBool();
    m_inEntireDocument = s->value(group + QLatin1String(inEntireDocumentKey), m_inEntireDocument).toBool();
    m_addFinalNewLine = s->value(group + QLatin1String(addFinalNewLineKey), m_addFinalNewLine).toBool();
}

bool StorageSettings::equals(const StorageSettings &ts) const
{
    return m_addFinalNewLine == ts.m_addFinalNewLine
        && m_cleanWhitespace == ts.m_cleanWhitespace
        && m_inEntireDocument == ts.m_inEntireDocument;
}

} // namespace TextEditor
