/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLJSCODECOMPLETION_H
#define QMLJSCODECOMPLETION_H

#include <qmljs/qmljsdocument.h>
#include <texteditor/icompletioncollector.h>
#include <texteditor/snippets/snippetcollector.h>
#include <QtCore/QDateTime>
#include <QtCore/QPointer>

namespace TextEditor {
class ITextEditable;
}

namespace QmlJS {
    class ModelManagerInterface;

    namespace Interpreter {
        class Value;
    }
}

namespace QmlJSEditor {

namespace Internal {

class FunctionArgumentWidget;

class CodeCompletion: public TextEditor::ICompletionCollector
{
    Q_OBJECT

public:
    explicit CodeCompletion(QmlJS::ModelManagerInterface *modelManager, QObject *parent = 0);
    virtual ~CodeCompletion();

    virtual TextEditor::ITextEditable *editor() const;
    virtual int startPosition() const;
    virtual bool shouldRestartCompletion();
    virtual bool supportsEditor(TextEditor::ITextEditable *editor);
    virtual bool triggersCompletion(TextEditor::ITextEditable *editor);
    virtual int startCompletion(TextEditor::ITextEditable *editor);
    virtual void completions(QList<TextEditor::CompletionItem> *completions);
    virtual bool typedCharCompletes(const TextEditor::CompletionItem &item, QChar typedChar);
    virtual void complete(const TextEditor::CompletionItem &item, QChar typedChar);
    virtual bool partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems);
    virtual QList<TextEditor::CompletionItem> getCompletions();
    virtual void sortCompletion(QList<TextEditor::CompletionItem> &completionItems);

    virtual void cleanup();

private:

    bool maybeTriggersCompletion(TextEditor::ITextEditable *editor);
    bool isDelimiter(QChar ch) const;

    bool completeUrl(const QString &relativeBasePath, const QString &urlString);
    bool completeFileName(const QString &relativeBasePath, const QString &fileName,
                          const QStringList &patterns = QStringList());

    void addCompletions(const QHash<QString, const QmlJS::Interpreter::Value *> &newCompletions,
                        const QIcon &icon, int relevance);
    void addCompletions(const QStringList &newCompletions,
                        const QIcon &icon, int relevance);
    void addCompletionsPropertyLhs(
            const QHash<QString, const QmlJS::Interpreter::Value *> &newCompletions,
            const QIcon &icon, int relevance, bool afterOn);

    QmlJS::ModelManagerInterface *m_modelManager;
    TextEditor::ITextEditable *m_editor;
    int m_startPosition;
    bool m_restartCompletion;
    TextEditor::SnippetCollector m_snippetProvider;
    QList<TextEditor::CompletionItem> m_completions;
    QPointer<FunctionArgumentWidget> m_functionArgumentWidget;
};


} // end of namespace Internal
} // end of namespace QmlJSEditor

#endif // QMLJSCODECOMPLETION_H
