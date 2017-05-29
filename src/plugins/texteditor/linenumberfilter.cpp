/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "linenumberfilter.h"
#include "texteditor.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <QMetaType>
#include <QPair>
#include <QVariant>

typedef QPair<int,int> LineColumn;
Q_DECLARE_METATYPE(LineColumn)

using namespace Core;
using namespace TextEditor;
using namespace TextEditor::Internal;

LineNumberFilter::LineNumberFilter(QObject *parent)
  : ILocatorFilter(parent)
{
    setId("Line in current document");
    setDisplayName(tr("Line in Current Document"));
    setPriority(High);
    setShortcutString(QString(QLatin1Char('l')));
    setIncludedByDefault(true);
}

void LineNumberFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    m_hasCurrentEditor = EditorManager::currentEditor() != 0;
}

QList<LocatorFilterEntry> LineNumberFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &, const QString &entry)
{
    QList<LocatorFilterEntry> value;
    const QVector<QStringRef> lineAndColumn = entry.splitRef(QLatin1Char(':'));
    int sectionCount = lineAndColumn.size();
    int line = 0;
    int column = 0;
    bool ok = false;
    if (sectionCount > 0)
        line = lineAndColumn.at(0).toInt(&ok);
    if (ok && sectionCount > 1)
        column = lineAndColumn.at(1).toInt(&ok);
    if (!ok)
        return value;
    if (m_hasCurrentEditor && (line > 0 || column > 0)) {
        LineColumn data;
        data.first = line;
        data.second = column - 1;  // column API is 0-based
        QString text;
        if (line > 0 && column > 0)
            text = tr("Line %1, Column %2").arg(line).arg(column);
        else if (line > 0)
            text = tr("Line %1").arg(line);
        else
            text = tr("Column %1").arg(column);
        value.append(LocatorFilterEntry(this, text, QVariant::fromValue(data)));
    }
    return value;
}

void LineNumberFilter::accept(LocatorFilterEntry selection,
                              QString *newText, int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    IEditor *editor = EditorManager::currentEditor();
    if (editor) {
        EditorManager::addCurrentPositionToNavigationHistory();
        LineColumn data = selection.internalData.value<LineColumn>();
        if (data.first < 1)  // jump to column in same line
            data.first = editor->currentLine();
        editor->gotoLine(data.first, data.second);
        EditorManager::activateEditor(editor);
    }
}
