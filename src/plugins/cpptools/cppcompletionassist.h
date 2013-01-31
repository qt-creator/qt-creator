/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPCOMPLETIONASSIST_H
#define CPPCOMPLETIONASSIST_H

#include "cppcompletionassistprovider.h"

#include <cplusplus/Icons.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/CppDocument.h>
#if QT_VERSION >= 0x050000
// Qt 5 requires the types to be defined for Q_DECLARE_METATYPE
#  include <cplusplus/Symbol.h>
#endif

#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/snippets/snippetassistcollector.h>
#include <texteditor/codeassist/defaultassistinterface.h>
#include <texteditor/codeassist/basicproposalitem.h>

#include <QStringList>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QTextCursor;
QT_END_NAMESPACE

namespace CPlusPlus {
class LookupItem;
class ClassOrNamespace;
class Function;
class LookupContext;
}

namespace CppTools {
namespace Internal {

class CppCompletionAssistInterface;

class CppAssistProposalModel : public TextEditor::BasicProposalItemListModel
{
public:
    CppAssistProposalModel()
        : TextEditor::BasicProposalItemListModel()
        , m_completionOperator(CPlusPlus::T_EOF_SYMBOL)
        , m_replaceDotForArrow(false)
        , m_typeOfExpression(new CPlusPlus::TypeOfExpression)
    {
        m_typeOfExpression->setExpandTemplates(true);
    }

    virtual bool isSortable(const QString &prefix) const;
    virtual TextEditor::IAssistProposalItem *proposalItem(int index) const;

    unsigned m_completionOperator;
    bool m_replaceDotForArrow;
    QSharedPointer<CPlusPlus::TypeOfExpression> m_typeOfExpression;
};

class InternalCompletionAssistProvider : public CppCompletionAssistProvider
{
    Q_OBJECT

public:
    virtual TextEditor::IAssistProcessor *createProcessor() const;
    virtual CppCompletionSupport *completionSupport(TextEditor::ITextEditor *editor);
};

class CppCompletionAssistProcessor : public TextEditor::IAssistProcessor
{
public:
    CppCompletionAssistProcessor();
    virtual ~CppCompletionAssistProcessor();

    virtual TextEditor::IAssistProposal *perform(const TextEditor::IAssistInterface *interface);

private:
    TextEditor::IAssistProposal *createContentProposal();
    TextEditor::IAssistProposal *createHintProposal(QList<CPlusPlus::Function *> symbols) const;
    bool accepts() const;

    int startOfOperator(int pos, unsigned *kind, bool wantFunctionCall) const;
    int findStartOfName(int pos = -1) const;
    int startCompletionHelper();
    bool tryObjCCompletion();
    bool objcKeywordsWanted() const;
    int startCompletionInternal(const QString fileName,
                                unsigned line, unsigned column,
                                const QString &expression,
                                int endOfExpression);

    void completeObjCMsgSend(CPlusPlus::ClassOrNamespace *binding, bool staticClassAccess);
    bool completeInclude(const QTextCursor &cursor);
    void completeInclude(const QString &realPath, const QStringList &suffixes);
    void completePreprocessor();
    bool completeConstructorOrFunction(const QList<CPlusPlus::LookupItem> &results,
                                       int endOfExpression,
                                       bool toolTipOnly);
    bool completeMember(const QList<CPlusPlus::LookupItem> &results);
    bool completeScope(const QList<CPlusPlus::LookupItem> &results);
    void completeNamespace(CPlusPlus::ClassOrNamespace *binding);
    void completeClass(CPlusPlus::ClassOrNamespace *b, bool staticLookup = true);
    bool completeQtMethod(const QList<CPlusPlus::LookupItem> &results, bool wantSignals);
    bool completeSignal(const QList<CPlusPlus::LookupItem> &results)
    { return completeQtMethod(results, true); }
    bool completeSlot(const QList<CPlusPlus::LookupItem> &results)
    { return completeQtMethod(results, false); }
    void globalCompletion(CPlusPlus::Scope *scope);

    void addCompletionItem(const QString &text,
                           const QIcon &icon = QIcon(),
                           int order = 0,
                           const QVariant &data = QVariant());
    void addCompletionItem(CPlusPlus::Symbol *symbol,
                           int order = 0);
    void addSnippets();
    void addKeywords();
    void addMacros(const QString &fileName, const CPlusPlus::Snapshot &snapshot);
    void addMacros_helper(const CPlusPlus::Snapshot &snapshot,
                          const QString &fileName,
                          QSet<QString> *processed,
                          QSet<QString> *definedMacros);

    int m_startPosition;
    bool m_objcEnabled;
    QScopedPointer<const CppCompletionAssistInterface> m_interface;
    QList<TextEditor::BasicProposalItem *> m_completions;
    TextEditor::SnippetAssistCollector m_snippetCollector;
    CPlusPlus::Icons m_icons;
    QStringList preprocessorCompletions;
    QScopedPointer<CppAssistProposalModel> m_model;
    TextEditor::IAssistProposal *m_hintProposal;
};

class CppCompletionAssistInterface : public TextEditor::DefaultAssistInterface
{
public:
    CppCompletionAssistInterface(QTextDocument *textDocument,
                                 int position,
                                 Core::IDocument *document,
                                 TextEditor::AssistReason reason,
                                 const CPlusPlus::Snapshot &snapshot,
                                 const QStringList &includePaths,
                                 const QStringList &frameworkPaths)
        : TextEditor::DefaultAssistInterface(textDocument, position, document, reason)
        , m_snapshot(snapshot)
        , m_includePaths(includePaths)
        , m_frameworkPaths(frameworkPaths)
    {}

    const CPlusPlus::Snapshot &snapshot() const { return m_snapshot; }
    const QStringList &includePaths() const { return m_includePaths; }
    const QStringList &frameworkPaths() const { return m_frameworkPaths; }

private:
    CPlusPlus::Snapshot m_snapshot;
    QStringList m_includePaths;
    QStringList m_frameworkPaths;
};

} // Internal
} // CppTools

Q_DECLARE_METATYPE(CPlusPlus::Symbol *)

#endif // CPPCOMPLETIONASSIST_H
