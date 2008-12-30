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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef CPPCODECOMPLETION_H
#define CPPCODECOMPLETION_H

// C++ front-end
#include <ASTfwd.h>
#include <FullySpecifiedType.h>
#include <cplusplus/Icons.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>

// Qt Creator
#include <texteditor/icompletioncollector.h>

// Qt
#include <QtCore/QObject>
#include <QtCore/QPointer>

namespace Core {
class ICore;
}

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
    CppCodeCompletion(CppModelManager *manager, Core::ICore *core);

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
    void addCompletionItem(CPlusPlus::Symbol *symbol);

    bool completeFunction(CPlusPlus::FullySpecifiedType exprTy,
                          const QList<CPlusPlus::TypeOfExpression::Result> &,
                          const CPlusPlus::LookupContext &context);

    bool completeMember(const QList<CPlusPlus::TypeOfExpression::Result> &,
                        const CPlusPlus::LookupContext &context);

    bool completeScope(const QList<CPlusPlus::TypeOfExpression::Result> &,
                       const CPlusPlus::LookupContext &context);

    void completeNamespace(const QList<CPlusPlus::Symbol *> &candidates,
                           const CPlusPlus::LookupContext &context);

    void completeClass(const QList<CPlusPlus::Symbol *> &candidates,
                       const CPlusPlus::LookupContext &context,
                       bool staticLookup = true);

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

    static int findStartOfName(const TextEditor::ITextEditor *editor);

    QList<TextEditor::CompletionItem> m_completions;

    TextEditor::ITextEditable *m_editor;
    int m_startPosition;     // Position of the cursor from which completion started

    Core::ICore *m_core;
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
