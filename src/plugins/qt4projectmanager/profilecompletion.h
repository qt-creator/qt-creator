/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef PROFILECOMPLETION_H
#define PROFILECOMPLETION_H

#include <texteditor/icompletioncollector.h>

namespace Qt4ProjectManager {

namespace Internal {

class ProFileCompletion : public TextEditor::ICompletionCollector
{
    Q_OBJECT
public:
    ProFileCompletion(QObject *parent = 0);

    virtual ~ProFileCompletion();

    virtual QList<TextEditor::CompletionItem> getCompletions();
    virtual bool shouldRestartCompletion();

    virtual TextEditor::ITextEditor *editor() const;
    virtual int startPosition() const;

    virtual bool supportsEditor(TextEditor::ITextEditor *editor) const;
    virtual bool supportsPolicy(TextEditor::CompletionPolicy policy) const;
    virtual bool triggersCompletion(TextEditor::ITextEditor *editor);
    virtual int startCompletion(TextEditor::ITextEditor *editor);
    virtual void completions(QList<TextEditor::CompletionItem> *completions);
    virtual bool typedCharCompletes(const TextEditor::CompletionItem &item, QChar typedChar);
    virtual void complete(const TextEditor::CompletionItem &item, QChar typedChar);
    virtual bool partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems);
    virtual void cleanup();
private:
    int findStartOfName(int pos = -1) const;
    bool isInComment() const;

    TextEditor::ITextEditor *m_editor;
    int m_startPosition;
    const QIcon m_variableIcon;
    const QIcon m_functionIcon;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROFILECOMPLETION_H
