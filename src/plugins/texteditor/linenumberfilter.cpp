// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "linenumberfilter.h"

#include "texteditortr.h"

#include <coreplugin/editormanager/editormanager.h>

using namespace Core;
using namespace Utils;

namespace TextEditor::Internal {

LineNumberFilter::LineNumberFilter()
{
    setId("Line in current document");
    setDisplayName(Tr::tr("Line in Current Document"));
    setDescription(Tr::tr("Jumps to the given line in the current document."));
    setDefaultSearchText(Tr::tr("<line>:<column>"));
    setPriority(High);
    setDefaultShortcutString("l");
    setDefaultIncludedByDefault(true);
}

LocatorMatcherTasks LineNumberFilter::matchers()
{
    using namespace Tasking;

    TreeStorage<LocatorStorage> storage;

    const auto onSetup = [storage] {
        const QStringList lineAndColumn = storage->input().split(':');
        int sectionCount = lineAndColumn.size();
        int line = 0;
        int column = 0;
        bool ok = false;
        if (sectionCount > 0)
            line = lineAndColumn.at(0).toInt(&ok);
        if (ok && sectionCount > 1)
            column = lineAndColumn.at(1).toInt(&ok);
        if (!ok)
            return;
        if (EditorManager::currentEditor() && (line > 0 || column > 0)) {
            QString text;
            if (line > 0 && column > 0)
                text = Tr::tr("Line %1, Column %2").arg(line).arg(column);
            else if (line > 0)
                text = Tr::tr("Line %1").arg(line);
            else
                text = Tr::tr("Column %1").arg(column);
            LocatorFilterEntry entry;
            entry.displayName = text;
            entry.acceptor = [line, targetColumn = column - 1] {
                IEditor *editor = EditorManager::currentEditor();
                if (!editor)
                    return AcceptResult();
                EditorManager::addCurrentPositionToNavigationHistory();
                editor->gotoLine(line < 1 ? editor->currentLine() : line, targetColumn);
                EditorManager::activateEditor(editor);
                return AcceptResult();
            };
            storage->reportOutput({entry});
        }
    };
    return {{Sync(onSetup), storage}};
}

} // namespace TextEditor::Internal
