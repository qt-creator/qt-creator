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
    m_core = SnippetsPlugin::core();
    m_snippetsWnd = SnippetsPlugin::snippetsWindow();

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

    int index = 0;
    foreach (SnippetSpec *spec, m_snippetsWnd->snippets()) {
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

void SnippetsCompletion::completions(QList<TextEditor::CompletionItem *> *completions)
{
    const int length = m_editor->position() - m_startPosition;
    if (length >= 2) {
        QString key = m_editor->textAt(m_startPosition, length);
        foreach (TextEditor::CompletionItem* item, m_autoCompletions.values()) {
            if (item->m_key.startsWith(key, Qt::CaseInsensitive)) {
                (*completions) << item;
            }
        }
    }
}

QString SnippetsCompletion::text(TextEditor::CompletionItem *item) const
{
    const SnippetSpec *spec = m_snippetsWnd->snippets().at(item->m_index);
    return spec->name();
}

QString SnippetsCompletion::details(TextEditor::CompletionItem *item) const
{
    const SnippetSpec *spec = m_snippetsWnd->snippets().at(item->m_index);
    return spec->description();
}

QIcon SnippetsCompletion::icon(TextEditor::CompletionItem *) const
{
    return m_fileIcon;
}

void SnippetsCompletion::complete(TextEditor::CompletionItem *item)
{
    SnippetSpec *spec = m_snippetsWnd->snippets().at(item->m_index);

    int length = m_editor->position() - m_startPosition;
    m_editor->setCurPos(m_startPosition);
    m_editor->remove(length);

    m_snippetsWnd->insertSnippet(m_editor, spec);
}

bool SnippetsCompletion::partiallyComplete()
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
