/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef CPPEDITOR_INTERNAL_CLANGCOMPLETION_H
#define CPPEDITOR_INTERNAL_CLANGCOMPLETION_H

#include "clangcompleter.h"

#include <cplusplus/Icons.h>

#include <cpptools/cppcompletionassistprovider.h>
#include <cpptools/cppmodelmanagerinterface.h>

#include <texteditor/codeassist/basicproposalitem.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/defaultassistinterface.h>
#include <texteditor/codeassist/iassistprocessor.h>

#include <QStringList>
#include <QTextCursor>

namespace ClangCodeModel {

namespace Internal {
class ClangAssistProposalModel;

class ClangCompletionAssistProvider : public CppTools::CppCompletionAssistProvider
{
public:
    ClangCompletionAssistProvider();

    virtual TextEditor::IAssistProcessor *createProcessor() const;
    virtual TextEditor::IAssistInterface *createAssistInterface(
            const QString &filePath,
            QTextDocument *document, bool isObjCEnabled, int position,
            TextEditor::AssistReason reason) const;

private:
    ClangCodeModel::ClangCompleter::Ptr m_clangCompletionWrapper;
};

} // namespace Internal

class CLANG_EXPORT ClangCompletionAssistInterface: public TextEditor::DefaultAssistInterface
{
public:
    ClangCompletionAssistInterface(ClangCodeModel::ClangCompleter::Ptr clangWrapper,
                                   QTextDocument *document,
                                   int position,
                                   const QString &fileName,
                                   TextEditor::AssistReason reason,
                                   const QStringList &options,
                                   const QList<CppTools::ProjectPart::HeaderPath> &headerPaths,
                                   const Internal::PchInfo::Ptr &pchInfo);

    ClangCodeModel::ClangCompleter::Ptr clangWrapper() const
    { return m_clangWrapper; }

    const ClangCodeModel::Internal::UnsavedFiles &unsavedFiles() const
    { return m_unsavedFiles; }

    bool objcEnabled() const;

    const QStringList &options() const
    { return m_options; }

    const QList<CppTools::ProjectPart::HeaderPath> &headerPaths() const
    { return m_headerPaths; }

private:
    ClangCodeModel::ClangCompleter::Ptr m_clangWrapper;
    ClangCodeModel::Internal::UnsavedFiles m_unsavedFiles;
    QStringList m_options;
    QList<CppTools::ProjectPart::HeaderPath> m_headerPaths;
    Internal::PchInfo::Ptr m_savedPchPointer;
};

class CLANG_EXPORT ClangCompletionAssistProcessor : public TextEditor::IAssistProcessor
{
    Q_DECLARE_TR_FUNCTIONS(ClangCodeModel::Internal::ClangCompletionAssistProcessor)

public:
    ClangCompletionAssistProcessor();
    virtual ~ClangCompletionAssistProcessor();

    virtual TextEditor::IAssistProposal *perform(const TextEditor::IAssistInterface *interface);

private:
    int startCompletionHelper();
    int startOfOperator(int pos, unsigned *kind, bool wantFunctionCall) const;
    int findStartOfName(int pos = -1) const;
    bool accepts() const;
    TextEditor::IAssistProposal *createContentProposal();

    int startCompletionInternal(const QString fileName,
                                unsigned line, unsigned column,
                                int endOfExpression);

    bool completeInclude(const QTextCursor &cursor);
    void completeIncludePath(const QString &realPath, const QStringList &suffixes);
    void completePreprocessor();
    void addCompletionItem(const QString &text,
                           const QIcon &icon = QIcon(),
                           int order = 0,
                           const QVariant &data = QVariant());

private:
    int m_startPosition;
    QScopedPointer<const ClangCompletionAssistInterface> m_interface;
    QList<TextEditor::BasicProposalItem *> m_completions;
    CPlusPlus::Icons m_icons;
    QStringList m_preprocessorCompletions;
    QScopedPointer<Internal::ClangAssistProposalModel> m_model;
    TextEditor::IAssistProposal *m_hintProposal;
};

} // namespace Clang

#endif // CPPEDITOR_INTERNAL_CLANGCOMPLETION_H
