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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef CPPCOMPLETIONASSIST_H
#define CPPCOMPLETIONASSIST_H

#include <cplusplus/Icons.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/CppDocument.h>

#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/snippets/snippetassistcollector.h>
#include <texteditor/codeassist/defaultassistinterface.h>

#include <QtCore/QStringList>
#include <QtCore/QVariant>

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
class CppAssistProposalModel;

class CppCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
public:
    virtual bool supportsEditor(const QString &editorId) const;
    virtual int activationCharSequenceLength() const;
    virtual bool isActivationCharSequence(const QString &sequence) const;
    virtual TextEditor::IAssistProcessor *createProcessor() const;
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
    void addCompletionItem(CPlusPlus::Symbol *symbol);
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
    const CppCompletionAssistProvider *m_provider;
    CPlusPlus::Icons m_icons;
    QStringList preprocessorCompletions;
    QScopedPointer<CppAssistProposalModel> m_model;
    TextEditor::IAssistProposal *m_hintProposal;
};

class CppCompletionAssistInterface : public TextEditor::DefaultAssistInterface
{
public:
    CppCompletionAssistInterface(QTextDocument *document,
                                 int position,
                                 Core::IFile *file,
                                 TextEditor::AssistReason reason,
                                 const CPlusPlus::Snapshot &snapshot,
                                 const QStringList &includePaths,
                                 const QStringList &frameworkPaths)
        : TextEditor::DefaultAssistInterface(document, position, file, reason)
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
