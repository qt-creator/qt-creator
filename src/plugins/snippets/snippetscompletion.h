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

#ifndef SNIPPETSCOMPLETION_H
#define SNIPPETSCOMPLETION_H

#include <texteditor/icompletioncollector.h>

#include <QtCore/QDir>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtGui/QIcon>

namespace TextEditor {
class ITextEditable;
class ITextEditor;
}

namespace Snippets {
namespace Internal {

class SnippetsWindow;
class SnippetSpec;

class SnippetsCompletion : public TextEditor::ICompletionCollector
{
    Q_OBJECT
public:
    SnippetsCompletion(QObject *parent);
    ~SnippetsCompletion();

    // ICompletionCollector
    bool triggersCompletion(TextEditor::ITextEditable *editor);
    int startCompletion(TextEditor::ITextEditable *editor);
    void completions(QList<TextEditor::CompletionItem *> *completions);

    QString text(TextEditor::CompletionItem *item) const;
    QString details(TextEditor::CompletionItem *item) const;
    QIcon icon(TextEditor::CompletionItem *item) const;

    void complete(const TextEditor::CompletionItem &item);
    bool partiallyComplete(const QList<TextEditor::CompletionItem> &);
    void cleanup();

    void completions(QList<TextEditor::CompletionItem>*);

private slots:
    void updateCompletions();

private:
    static int findStartOfName(const TextEditor::ITextEditor *editor);

    TextEditor::ITextEditable *m_editor;
    int m_startPosition;  // Position of the cursor from which completion started

    SnippetsWindow *m_snippetsWindow;

    QMultiMap<QString, TextEditor::CompletionItem *> m_autoCompletions;

    static const QIcon m_fileIcon;
};

} // namespace Internal
} // namespace Snippets

#endif // SNIPPETSCOMPLETION_H

