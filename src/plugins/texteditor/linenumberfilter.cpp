// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "linenumberfilter.h"

#include "texteditortr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <QMetaType>
#include <QPair>
#include <QVariant>

using LineColumn = QPair<int, int>;
Q_DECLARE_METATYPE(LineColumn)

using namespace Core;

namespace TextEditor::Internal {

LineNumberFilter::LineNumberFilter(QObject *parent)
  : ILocatorFilter(parent)
{
    setId("Line in current document");
    setDisplayName(Tr::tr("Line in Current Document"));
    setDescription(Tr::tr("Jumps to the given line in the current document."));
    setDefaultSearchText(Tr::tr("<line>:<column>"));
    setPriority(High);
    setDefaultShortcutString("l");
    setDefaultIncludedByDefault(true);
}

void LineNumberFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    m_hasCurrentEditor = EditorManager::currentEditor() != nullptr;
}

QList<LocatorFilterEntry> LineNumberFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &, const QString &entry)
{
    QList<LocatorFilterEntry> value;
    const QStringList lineAndColumn = entry.split(':');
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
            text = Tr::tr("Line %1, Column %2").arg(line).arg(column);
        else if (line > 0)
            text = Tr::tr("Line %1").arg(line);
        else
            text = Tr::tr("Column %1").arg(column);
        LocatorFilterEntry entry(this, text);
        entry.internalData = QVariant::fromValue(data);
        value.append(entry);
    }
    return value;
}

void LineNumberFilter::accept(const LocatorFilterEntry &selection,
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

} // TextEditor::Internal
