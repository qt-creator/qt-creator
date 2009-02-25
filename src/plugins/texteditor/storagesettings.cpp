/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "storagesettings.h"

#include <QSettings>
#include <QString>

namespace TextEditor {

static const char * const cleanWhitespaceKey = "cleanWhitespace";
static const char * const inEntireDocumentKey = "inEntireDocument";
static const char * const addFinalNewLineKey = "addFinalNewLine";
static const char * const cleanIndentationKey = "cleanIndentation";
static const char * const groupPostfix = "StorageSettings";

StorageSettings::StorageSettings()
    : m_cleanWhitespace(true),
      m_inEntireDocument(false),
      m_addFinalNewLine(true),
      m_cleanIndentation(true)
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
    s->setValue(QLatin1String(cleanIndentationKey), m_cleanIndentation);
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
    m_cleanIndentation = s->value(group + QLatin1String(cleanIndentationKey), m_cleanIndentation).toBool();
}

bool StorageSettings::equals(const StorageSettings &ts) const
{
    return m_addFinalNewLine == ts.m_addFinalNewLine
        && m_cleanWhitespace == ts.m_cleanWhitespace
        && m_inEntireDocument == ts.m_inEntireDocument
        && m_cleanIndentation == ts.m_cleanIndentation;
}

} // namespace TextEditor
