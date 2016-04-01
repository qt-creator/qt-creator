/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "builtineditordocumentparser.h"
#include "cppcompletionassistprocessor.h"
#include "cppcompletionassistprovider.h"
#include "cppmodelmanager.h"
#include "cppworkingcopy.h"

#include <cplusplus/Icons.h>
#include <cplusplus/Symbol.h>
#include <cplusplus/TypeOfExpression.h>

#include <texteditor/texteditor.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/snippets/snippetassistcollector.h>


#include <QStringList>
#include <QVariant>

namespace CPlusPlus {
class LookupItem;
class ClassOrNamespace;
class Function;
class LookupContext;
} // namespace CPlusPlus

namespace CppTools {
namespace Internal {

class CppCompletionAssistInterface;

class CppAssistProposalModel : public TextEditor::GenericProposalModel
{
public:
    CppAssistProposalModel()
        : TextEditor::GenericProposalModel()
        , m_completionOperator(CPlusPlus::T_EOF_SYMBOL)
        , m_replaceDotForArrow(false)
        , m_typeOfExpression(new CPlusPlus::TypeOfExpression)
    {
        m_typeOfExpression->setExpandTemplates(true);
    }

    bool isSortable(const QString &prefix) const override;
    TextEditor::AssistProposalItemInterface *proposalItem(int index) const override;

    unsigned m_completionOperator;
    bool m_replaceDotForArrow;
    QSharedPointer<CPlusPlus::TypeOfExpression> m_typeOfExpression;
};

class InternalCompletionAssistProvider : public CppCompletionAssistProvider
{
    Q_OBJECT

public:
    TextEditor::IAssistProcessor *createProcessor() const override;

    TextEditor::AssistInterface *createAssistInterface(
            const QString &filePath,
            const TextEditor::TextEditorWidget *textEditorWidget,
            const CPlusPlus::LanguageFeatures &languageFeatures,
            int position,
            TextEditor::AssistReason reason) const override;
};

class InternalCppCompletionAssistProcessor : public CppCompletionAssistProcessor
{
public:
    InternalCppCompletionAssistProcessor();
    ~InternalCppCompletionAssistProcessor();

    TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *interface) override;

private:
    TextEditor::IAssistProposal *createContentProposal();
    TextEditor::IAssistProposal *createHintProposal(QList<CPlusPlus::Function *> symbols) const;
    bool accepts() const;

    int startOfOperator(int positionInDocument, unsigned *kind, bool wantFunctionCall) const;
    int findStartOfName(int pos = -1) const;
    int startCompletionHelper();
    bool tryObjCCompletion();
    bool objcKeywordsWanted() const;
    int startCompletionInternal(const QString &fileName,
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
    void addClassMembersToCompletion(CPlusPlus::Scope *scope, bool staticLookup);
    enum CompleteQtMethodMode {
        CompleteQt4Signals,
        CompleteQt4Slots,
        CompleteQt5Signals,
        CompleteQt5Slots,
    };
    bool completeQtMethod(const QList<CPlusPlus::LookupItem> &results, CompleteQtMethodMode type);
    bool completeQtMethodClassName(const QList<CPlusPlus::LookupItem> &results,
                                   CPlusPlus::Scope *cursorScope);
    bool globalCompletion(CPlusPlus::Scope *scope);

    void addCompletionItem(const QString &text,
                           const QIcon &icon = QIcon(),
                           int order = 0,
                           const QVariant &data = QVariant());
    void addCompletionItem(CPlusPlus::Symbol *symbol,
                           int order = 0);
    void addKeywords();
    void addMacros(const QString &fileName, const CPlusPlus::Snapshot &snapshot);
    void addMacros_helper(const CPlusPlus::Snapshot &snapshot,
                          const QString &fileName,
                          QSet<QString> *processed,
                          QSet<QString> *definedMacros);

    enum {
        CompleteQt5SignalOrSlotClassNameTrigger = CPlusPlus::T_LAST_TOKEN + 1,
        CompleteQt5SignalTrigger,
        CompleteQt5SlotTrigger
    };

    QScopedPointer<const CppCompletionAssistInterface> m_interface;
    QScopedPointer<CppAssistProposalModel> m_model;
};

class CppCompletionAssistInterface : public TextEditor::AssistInterface
{
public:
    CppCompletionAssistInterface(const QString &filePath,
                                 const TextEditor::TextEditorWidget *textEditorWidget,
                                 BuiltinEditorDocumentParser::Ptr parser,
                                 const CPlusPlus::LanguageFeatures &languageFeatures,
                                 int position,
                                 TextEditor::AssistReason reason,
                                 const WorkingCopy &workingCopy)
        : TextEditor::AssistInterface(textEditorWidget->document(), position, filePath, reason)
        , m_parser(parser)
        , m_gotCppSpecifics(false)
        , m_workingCopy(workingCopy)
        , m_languageFeatures(languageFeatures)
    {}

    CppCompletionAssistInterface(const QString &filePath,
                                 QTextDocument *textDocument,
                                 int position,
                                 TextEditor::AssistReason reason,
                                 const CPlusPlus::Snapshot &snapshot,
                                 const ProjectPartHeaderPaths &headerPaths,
                                 const CPlusPlus::LanguageFeatures &features)
        : TextEditor::AssistInterface(textDocument, position, filePath, reason)
        , m_gotCppSpecifics(true)
        , m_snapshot(snapshot)
        , m_headerPaths(headerPaths)
        , m_languageFeatures(features)
    {}

    const CPlusPlus::Snapshot &snapshot() const { getCppSpecifics(); return m_snapshot; }
    const ProjectPartHeaderPaths &headerPaths() const
    { getCppSpecifics(); return m_headerPaths; }
    CPlusPlus::LanguageFeatures languageFeatures() const
    { getCppSpecifics(); return m_languageFeatures; }

private:
    void getCppSpecifics() const;

    BuiltinEditorDocumentParser::Ptr m_parser;
    mutable bool m_gotCppSpecifics;
    WorkingCopy m_workingCopy;
    mutable CPlusPlus::Snapshot m_snapshot;
    mutable ProjectPartHeaderPaths m_headerPaths;
    mutable CPlusPlus::LanguageFeatures m_languageFeatures;
};

} // Internal
} // CppTools

Q_DECLARE_METATYPE(CPlusPlus::Symbol *)
