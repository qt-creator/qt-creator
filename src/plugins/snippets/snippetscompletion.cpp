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

#include "snippetscompletion.h"
#include "snippetswindow.h"
#include "snippetspec.h"
#include "snippetsplugin.h"

#include <texteditor/itexteditable.h>

#include <QtCore/QDebug>
#include <QtCore/QMap>
#include <QtGui/QAction>
#include <QtGui/QKeyEvent>

using namespace Snippets::Internal;

const QIcon SnippetsCompletion::m_fileIcon = QIcon(":/snippets/images/file.png");

SnippetsCompletion::SnippetsCompletion(QObject *parent)
    : ICompletionCollector(parent)
{
    m_snippetsWindow = SnippetsPlugin::snippetsWindow();

    updateCompletions();
}

SnippetsCompletion::~SnippetsCompletion()
{
    qDeleteAll(m_autoCompletions.values());
    m_autoCompletions.clear();
}

void SnippetsCompletion::updateCompletions()
{
    qDeleteAll(m_autoCompletions.values());
    m_autoCompletions.clear();
#if 0
    int index = 0;
    foreach (SnippetSpec *spec, m_snippetsWindow->snippets()) {
        if (!spec->completionShortcut().isEmpty()) {
            TextEditor::CompletionItem *item = new TextEditor::CompletionItem;
            item->m_key = spec->name();
            item->m_collector = this;
            item->m_index = index;
            item->m_relevance = 0;
            m_autoCompletions.insert(spec->completionShortcut(), item);
            ++index;
        }
    }
#endif
}

bool SnippetsCompletion::triggersCompletion(TextEditor::ITextEditable *editor)
{
    QString currentWord = editor->textAt(editor->position() - 3, 3);
    currentWord = currentWord.trimmed();
    return currentWord.length() == 2 && m_autoCompletions.contains(currentWord) &&
            !editor->characterAt(editor->position() - 1).isSpace();
}

int SnippetsCompletion::startCompletion(TextEditor::ITextEditable *editor)
{
    m_editor = editor;
    m_startPosition = findStartOfName(m_editor);
    return m_startPosition;
}

#if 0
void SnippetsCompletion::completions(const QList<TextEditor::CompletionItem *> &completions)
{
    const int length = m_editor->position() - m_startPosition;
    if (length >= 2) {
        QString key = m_editor->textAt(m_startPosition, length);
        foreach (TextEditor::CompletionItem* item, m_autoCompletions.values()) {
            if (item->m_key.startsWith(key, Qt::CaseInsensitive))
                completions->append(item);
        }
    }
}
#endif

QString SnippetsCompletion::text(TextEditor::CompletionItem *item) const
{
#if 0
    const SnippetSpec *spec = m_snippetsWindow->snippets().at(item->m_index);
    return spec->name();
#endif
    return QString();
}

QString SnippetsCompletion::details(TextEditor::CompletionItem *item) const
{
#if 0
    const SnippetSpec *spec = m_snippetsWindow->snippets().at(item->m_index);
    return spec->description();
#endif
    return QString();
}

QIcon SnippetsCompletion::icon(TextEditor::CompletionItem *) const
{
    return m_fileIcon;
}

void SnippetsCompletion::complete(const TextEditor::CompletionItem &item)
{
#if 0
    SnippetSpec *spec = m_snippetsWindow->snippets().at(item->m_index);

    int length = m_editor->position() - m_startPosition;
    m_editor->setCurPos(m_startPosition);
    m_editor->remove(length);

    m_snippetsWindow->insertSnippet(m_editor, spec);
#endif
}

bool SnippetsCompletion::partiallyComplete(const myns::QList<TextEditor::CompletionItem>&)
{
    return false;
}

void SnippetsCompletion::cleanup()
{
}

int SnippetsCompletion::findStartOfName(const TextEditor::ITextEditor *editor)
{
    int pos = editor->position() - 1;
    QChar chr = editor->characterAt(pos);

    // Skip to the start of a name
    while (!chr.isSpace() && !chr.isNull())
        chr = editor->characterAt(--pos);

    return pos + 1;
}
