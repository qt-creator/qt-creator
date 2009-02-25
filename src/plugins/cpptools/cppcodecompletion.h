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

#ifndef CPPCODECOMPLETION_H
#define CPPCODECOMPLETION_H

#include <ASTfwd.h>
#include <FullySpecifiedType.h>
#include <cplusplus/Icons.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>

#include <texteditor/icompletioncollector.h>

#include <QtCore/QObject>
#include <QtCore/QPointer>


namespace TextEditor {
class ITextEditor;
}

namespace CppTools {
namespace Internal {

class CppModelManager;
class FunctionArgumentWidget;

class CppCodeCompletion : public TextEditor::ICompletionCollector
{
    Q_OBJECT
public:
    explicit CppCodeCompletion(CppModelManager *manager);

    bool triggersCompletion(TextEditor::ITextEditable *editor);
    int startCompletion(TextEditor::ITextEditable *editor);
    void completions(QList<TextEditor::CompletionItem> *completions);

    void complete(const TextEditor::CompletionItem &item);
    bool partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems);
    void cleanup();

    QIcon iconForSymbol(CPlusPlus::Symbol *symbol) const;

    Qt::CaseSensitivity caseSensitivity() const;
    void setCaseSensitivity(Qt::CaseSensitivity caseSensitivity);

    bool autoInsertBraces() const;
    void setAutoInsertBraces(bool autoInsertBraces);

    bool isPartialCompletionEnabled() const;
    void setPartialCompletionEnabled(bool partialCompletionEnabled);

private:
    void addKeywords();
    void addMacros(const CPlusPlus::LookupContext &context);
    void addMacros_helper(const CPlusPlus::LookupContext &context,
                          const QString &fileName,
                          QSet<QString> *processed,
                          QSet<QString> *definedMacros);
    void addCompletionItem(CPlusPlus::Symbol *symbol);

    bool completeConstructorOrFunction(CPlusPlus::FullySpecifiedType exprTy,
                                       const QList<CPlusPlus::TypeOfExpression::Result> &);

    bool completeMember(const QList<CPlusPlus::TypeOfExpression::Result> &,
                        const CPlusPlus::LookupContext &context);

    bool completeScope(const QList<CPlusPlus::TypeOfExpression::Result> &,
                       const CPlusPlus::LookupContext &context);

    void completeNamespace(const QList<CPlusPlus::Symbol *> &candidates,
                           const CPlusPlus::LookupContext &context);

    void completeClass(const QList<CPlusPlus::Symbol *> &candidates,
                       const CPlusPlus::LookupContext &context,
                       bool staticLookup = true);

    bool completeConstructors(CPlusPlus::Class *klass);

    bool completeQtMethod(CPlusPlus::FullySpecifiedType exprTy,
                          const QList<CPlusPlus::TypeOfExpression::Result> &,
                          const CPlusPlus::LookupContext &context,
                          bool wantSignals);

    bool completeSignal(CPlusPlus::FullySpecifiedType exprTy,
                        const QList<CPlusPlus::TypeOfExpression::Result> &results,
                        const CPlusPlus::LookupContext &context)
    { return completeQtMethod(exprTy, results, context, true); }

    bool completeSlot(CPlusPlus::FullySpecifiedType exprTy,
                      const QList<CPlusPlus::TypeOfExpression::Result> &results,
                      const CPlusPlus::LookupContext &context)
    { return completeQtMethod(exprTy, results, context, false); }

    int findStartOfName(int pos = -1) const;

    QList<TextEditor::CompletionItem> m_completions;

    TextEditor::ITextEditable *m_editor;
    int m_startPosition;     // Position of the cursor from which completion started

    CppModelManager *m_manager;
    Qt::CaseSensitivity m_caseSensitivity;
    bool m_autoInsertBraces;
    bool m_partialCompletionEnabled;

    bool m_forcedCompletion;

    CPlusPlus::Icons m_icons;
    CPlusPlus::Overview overview;
    CPlusPlus::TypeOfExpression typeOfExpression;

    unsigned m_completionOperator;

    QPointer<FunctionArgumentWidget> m_functionArgumentWidget;
};

} // namespace Internal
} // namespace CppTools

Q_DECLARE_METATYPE(CPlusPlus::Symbol *)

#endif // CPPCODECOMPLETION_H
