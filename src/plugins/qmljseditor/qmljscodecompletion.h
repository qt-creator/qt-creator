/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QMLJSCODECOMPLETION_H
#define QMLJSCODECOMPLETION_H

#include <qmljs/qmljsdocument.h>
#include <texteditor/icompletioncollector.h>
#include <QtCore/QDateTime>
#include <QtCore/QPointer>

namespace TextEditor {
class ITextEditable;
}

namespace QmlJSEditor {

class ModelManagerInterface;

namespace Internal {

class FunctionArgumentWidget;

class CodeCompletion: public TextEditor::ICompletionCollector
{
    Q_OBJECT

public:
    CodeCompletion(ModelManagerInterface *modelManager, QObject *parent = 0);
    virtual ~CodeCompletion();

    Qt::CaseSensitivity caseSensitivity() const;
    void setCaseSensitivity(Qt::CaseSensitivity caseSensitivity);

    virtual TextEditor::ITextEditable *editor() const;
    virtual int startPosition() const;
    virtual bool shouldRestartCompletion();
    virtual bool supportsEditor(TextEditor::ITextEditable *editor);
    virtual bool triggersCompletion(TextEditor::ITextEditable *editor);
    virtual int startCompletion(TextEditor::ITextEditable *editor);
    virtual void completions(QList<TextEditor::CompletionItem> *completions);
    virtual void complete(const TextEditor::CompletionItem &item);
    virtual bool partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems);
    virtual QList<TextEditor::CompletionItem> getCompletions();

    virtual void cleanup();

private:
    void updateSnippets();

    bool maybeTriggersCompletion(TextEditor::ITextEditable *editor);
    bool isDelimiter(const QChar &ch) const;

    ModelManagerInterface *m_modelManager;
    TextEditor::ITextEditable *m_editor;
    int m_startPosition;
    QList<TextEditor::CompletionItem> m_completions;
    Qt::CaseSensitivity m_caseSensitivity;

    QList<TextEditor::CompletionItem> m_snippets;
    QDateTime m_snippetFileLastModified;
    QPointer<FunctionArgumentWidget> m_functionArgumentWidget;
};


} // end of namespace Internal
} // end of namespace QmlJSEditor

#endif // QMLJSCODECOMPLETION_H
