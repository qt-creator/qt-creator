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
#ifndef GLSLCODECOMPLETION_H
#define GLSLCODECOMPLETION_H

#include <texteditor/icompletioncollector.h>
#include <QtCore/QPointer>

namespace GLSLEditor {
namespace Internal {

class FunctionArgumentWidget;

class CodeCompletion: public TextEditor::ICompletionCollector
{
    Q_OBJECT

public:
    CodeCompletion(QObject *parent = 0);
    virtual ~CodeCompletion();

    /* Returns the current active ITextEditable */
    virtual TextEditor::ITextEditable *editor() const;
    virtual int startPosition() const;

    /*
     * Returns true if this completion collector can be used with the given editor.
     */
    virtual bool supportsEditor(TextEditor::ITextEditable *editor);

    /* This method should return whether the cursor is at a position which could
     * trigger an autocomplete. It will be called each time a character is typed in
     * the text editor.
     */
    virtual bool triggersCompletion(TextEditor::ITextEditable *editor);

    // returns starting position
    virtual int startCompletion(TextEditor::ITextEditable *editor);

    /* This method should add all the completions it wants to show into the list,
     * based on the given cursor position.
     */
    virtual void completions(QList<TextEditor::CompletionItem> *completions);

    /**
     * This method should return true when the given typed character should cause
     * the selected completion item to be completed.
     */
    virtual bool typedCharCompletes(const TextEditor::CompletionItem &item, QChar typedChar);

    /**
     * This method should complete the given completion item.
     *
     * \param typedChar Non-null when completion was triggered by typing a
     *                  character. Possible values depend on typedCharCompletes()
     */
    virtual void complete(const TextEditor::CompletionItem &item, QChar typedChar);

    /* This method gives the completion collector a chance to partially complete
     * based on a set of items. The general use case is to complete the common
     * prefix shared by all possible completion items.
     *
     * Returns whether the completion popup should be closed.
     */
    virtual bool partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems);

    virtual QList<TextEditor::CompletionItem> getCompletions();
    virtual bool shouldRestartCompletion();

    /* Called when it's safe to clean up the completion items.
     */
    virtual void cleanup();

private:
    QList<TextEditor::CompletionItem> m_completions;
    QList<TextEditor::CompletionItem> m_keywordCompletions;
    TextEditor::ITextEditable *m_editor;
    int m_startPosition;
    bool m_restartCompletion;
    QPointer<FunctionArgumentWidget> m_functionArgumentWidget;

    static bool glslCompletionItemLessThan(const TextEditor::CompletionItem &l, const TextEditor::CompletionItem &r);

    int m_keywordVariant;

    QIcon m_keywordIcon;
    QIcon m_varIcon;
    QIcon m_functionIcon;
    QIcon m_typeIcon;
    QIcon m_constIcon;
    QIcon m_attributeIcon;
    QIcon m_uniformIcon;
    QIcon m_varyingIcon;
    QIcon m_otherIcon;
};

} // namespace Internal
} // namespace GLSLEditor

#endif // GLSLCODECOMPLETION_H
