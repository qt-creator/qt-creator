/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef SNIPPETSCOMPLETION_H
#define SNIPPETSCOMPLETION_H

#include <texteditor/icompletioncollector.h>

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QDir>
#include <QtGui/QIcon>

namespace Core {
class ICore;
}

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

    void complete(TextEditor::CompletionItem *item);
    bool partiallyComplete();
    void cleanup();

private slots:
    void updateCompletions();

private:
    static int findStartOfName(const TextEditor::ITextEditor *editor);

    TextEditor::ITextEditable *m_editor;
    int m_startPosition;                            // Position of the cursor from which completion started

    SnippetsWindow *m_snippetsWnd;
    Core::ICore *m_core;

    QMultiMap<QString, TextEditor::CompletionItem *> m_autoCompletions;

    static const QIcon m_fileIcon;
};

} // namespace Internal
} // namespace Snippets

#endif // SNIPPETSCOMPLETION_H

