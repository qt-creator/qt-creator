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
***************************************************************************/
#include "displaysettings.h"

#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>

static const char * const displayLineNumbersKey = "DisplayLineNumbers";
static const char * const textWrappingKey = "TextWrapping";
static const char * const showWrapColumnKey = "ShowWrapColumn";
static const char * const wrapColumnKey = "WrapColumn";
static const char * const visualizeWhitespaceKey = "VisualizeWhitespace";
static const char * const displayFoldingMarkersKey = "DisplayFoldingMarkers";
static const char * const highlightCurrentLineKey = "HighlightCurrentLineKey";
static const char * const groupPostfix = "DisplaySettings";

namespace TextEditor {

DisplaySettings::DisplaySettings() :
    m_displayLineNumbers(true),
    m_textWrapping(false),
    m_showWrapColumn(false),
    m_wrapColumn(80),
    m_visualizeWhitespace(false),
    m_displayFoldingMarkers(true),
    m_highlightCurrentLine(true)
{
}

void DisplaySettings::toSettings(const QString &category, QSettings *s) const
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);
    s->beginGroup(group);
    s->setValue(QLatin1String(displayLineNumbersKey), m_displayLineNumbers);
    s->setValue(QLatin1String(textWrappingKey), m_textWrapping);
    s->setValue(QLatin1String(showWrapColumnKey), m_showWrapColumn);
    s->setValue(QLatin1String(wrapColumnKey), m_wrapColumn);
    s->setValue(QLatin1String(visualizeWhitespaceKey), m_visualizeWhitespace);
    s->setValue(QLatin1String(displayFoldingMarkersKey), m_displayFoldingMarkers);
    s->setValue(QLatin1String(highlightCurrentLineKey), m_highlightCurrentLine);
    s->endGroup();
}

void DisplaySettings::fromSettings(const QString &category, const QSettings *s)
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);
    group += QLatin1Char('/');

    *this = DisplaySettings(); // Assign defaults

    m_displayLineNumbers = s->value(group + QLatin1String(displayLineNumbersKey), m_displayLineNumbers).toBool();
    m_textWrapping = s->value(group + QLatin1String(textWrappingKey), m_textWrapping).toBool();
    m_showWrapColumn = s->value(group + QLatin1String(showWrapColumnKey), m_showWrapColumn).toBool();
    m_wrapColumn = s->value(group + QLatin1String(wrapColumnKey), m_wrapColumn).toInt();
    m_visualizeWhitespace = s->value(group + QLatin1String(visualizeWhitespaceKey), m_visualizeWhitespace).toBool();
    m_displayFoldingMarkers = s->value(group + QLatin1String(displayFoldingMarkersKey), m_displayFoldingMarkers).toBool();
    m_highlightCurrentLine = s->value(group + QLatin1String(highlightCurrentLineKey), m_highlightCurrentLine).toBool();
}

bool DisplaySettings::equals(const DisplaySettings &ds) const
{
    return m_displayLineNumbers == ds.m_displayLineNumbers
        && m_textWrapping == ds.m_textWrapping
        && m_showWrapColumn == ds.m_showWrapColumn
        && m_wrapColumn == ds.m_wrapColumn
        && m_visualizeWhitespace == ds.m_visualizeWhitespace
        && m_displayFoldingMarkers == ds.m_displayFoldingMarkers
        && m_highlightCurrentLine == ds.m_highlightCurrentLine
        ;
}

} // namespace TextEditor
