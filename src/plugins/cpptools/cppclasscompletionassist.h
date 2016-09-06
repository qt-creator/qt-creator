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
#include <cpptools/cpprefactoringchanges.h>
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

class CppCompletionAssistInterface;
class CppAssistProposalModel;

class CPPTOOLS_EXPORT CppClassCompletionAssistProvider : public CppCompletionAssistProvider
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

class CppClassCompletionAssistProcessor : public CppCompletionAssistProcessor
{
public:
   TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *interface) override;
private:
   bool classCompletion(const QString& filename, int& positionOfProposals);
   TextEditor::IAssistProposal *createContentProposal();
   void addCompletionItemWithRegexp(const QString& text, const QIcon& icon, int order, const QSharedPointer<QRegExp>& qRegExp);
   void executeClassCompletion(CppTools::CppRefactoringFilePtr currentFile, int requestPosition, int& proposalPosition, QStringList& proposals, QString& regExp);
   void getCurrentSymbolForClassCompletion(CppTools::CppRefactoringFilePtr currentFile, int requestPosition, QString& symbol, int& symbolPosition);
   void createRegexpForUpperCase(const QString& expression, QString& regexp) const;
   void orderProposals(const QString& cursorSymbol, const QMap<int, QSet<QString> >& proposalsMap, QStringList& proposals);
   void transformQualifiedClassIntoProposition(const QString& className, const QStringList& vNamespaceElements, const QStringList& vClassAsNamespaceElements, const QStringList& vCallerClassNamespace, const QStringList& vCallerClassAsNamespaceElements, const QMap<QString, QString>& namespaceAliases, QString& proposition, int& callerNamespaceDistance);
   QSharedPointer<QRegExp> m_qRegExp;
};



} // CppTools

