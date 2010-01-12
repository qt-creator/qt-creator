/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QTSCRIPTCODECOMPLETION_H
#define QTSCRIPTCODECOMPLETION_H

#include <texteditor/icompletioncollector.h>

namespace TextEditor {
class ITextEditable;
}

namespace QtScriptEditor {
namespace Internal {

class QtScriptCodeCompletion: public TextEditor::ICompletionCollector
{
    Q_OBJECT

public:
    QtScriptCodeCompletion(QObject *parent = 0);
    virtual ~QtScriptCodeCompletion();

    Qt::CaseSensitivity caseSensitivity() const;
    void setCaseSensitivity(Qt::CaseSensitivity caseSensitivity);

    virtual bool supportsEditor(TextEditor::ITextEditable *editor);
    virtual bool triggersCompletion(TextEditor::ITextEditable *editor);
    virtual int startCompletion(TextEditor::ITextEditable *editor);
    virtual void completions(QList<TextEditor::CompletionItem> *completions);
    virtual void complete(const TextEditor::CompletionItem &item);
    virtual bool partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems);
    virtual void cleanup();

private:
    TextEditor::ITextEditable *m_editor;
    int m_startPosition;
    QList<TextEditor::CompletionItem> m_completions;
    Qt::CaseSensitivity m_caseSensitivity;
};


} // end of namespace Internal
} // end of namespace QtScriptEditor

#endif // QTSCRIPTCODECOMPLETION_H
