/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "clangautomationutils.h"

#include "../clangcompletionassistinterface.h"
#include "../clangcompletionassistprovider.h"

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QElapsedTimer>

namespace ClangCodeModel {
namespace Internal {

class WaitForAsyncCompletions
{
public:
    enum WaitResult { GotResults, GotInvalidResults, Timeout };

    WaitResult wait(TextEditor::IAssistProcessor *processor,
                    TextEditor::AssistInterface *assistInterface,
                    int timeoutInMs)
    {
        QTC_ASSERT(processor, return Timeout);
        QTC_ASSERT(assistInterface, return Timeout);

        bool gotResults = false;

        processor->setAsyncCompletionAvailableHandler(
                    [this, &gotResults] (TextEditor::IAssistProposal *proposal) {
            QTC_ASSERT(proposal, return);
            proposalModel = proposal->model();
            delete proposal;
            gotResults = true;
        });

        // Are there any immediate results?
        if (TextEditor::IAssistProposal *proposal = processor->perform(assistInterface)) {
            delete processor;
            proposalModel = proposal->model();
            delete proposal;
            QTC_ASSERT(proposalModel, return GotInvalidResults);
            return GotResults;
        }

        // There are not any, so wait for async results.
        QElapsedTimer timer;
        timer.start();
        while (!gotResults) {
            if (timer.elapsed() >= timeoutInMs)
                return Timeout;
            QCoreApplication::processEvents();
        }

        return proposalModel ? GotResults : GotInvalidResults;
    }

public:
    TextEditor::IAssistProposalModel *proposalModel;
};

static const CppTools::ProjectPartHeaderPaths toHeaderPaths(const QStringList &paths)
{
    using namespace CppTools;

    ProjectPartHeaderPaths result;
    foreach (const QString &path, paths)
        result << ProjectPartHeaderPath(path, ProjectPartHeaderPath::IncludePath);
    return result;
}

ProposalModel completionResults(TextEditor::BaseTextEditor *textEditor,
                                const QStringList &includePaths,
                                int timeOutInMs)
{
    using namespace TextEditor;

    TextEditorWidget *textEditorWidget = qobject_cast<TextEditorWidget *>(textEditor->widget());
    QTC_ASSERT(textEditorWidget, return ProposalModel());
    AssistInterface *assistInterface = textEditorWidget->createAssistInterface(
                TextEditor::Completion, TextEditor::ExplicitlyInvoked);
    QTC_ASSERT(assistInterface, return ProposalModel());
    if (!includePaths.isEmpty()) {
        auto clangAssistInterface = static_cast<ClangCompletionAssistInterface *>(assistInterface);
        clangAssistInterface->setHeaderPaths(toHeaderPaths(includePaths));
    }

    CompletionAssistProvider *assistProvider
            = textEditor->textDocument()->completionAssistProvider();
    QTC_ASSERT(qobject_cast<ClangCompletionAssistProvider *>(assistProvider),
               return ProposalModel());
    QTC_ASSERT(assistProvider, return ProposalModel());
    QTC_ASSERT(assistProvider->runType() == IAssistProvider::Asynchronous, return ProposalModel());

    IAssistProcessor *processor = assistProvider->createProcessor();
    QTC_ASSERT(processor, return ProposalModel());

    WaitForAsyncCompletions waitForCompletions;
    const WaitForAsyncCompletions::WaitResult result = waitForCompletions.wait(processor,
                                                                               assistInterface,
                                                                               timeOutInMs);
    QTC_ASSERT(result == WaitForAsyncCompletions::GotResults, return ProposalModel());
    return QSharedPointer<TextEditor::IAssistProposalModel>(waitForCompletions.proposalModel);
}

} // namespace Internal
} // namespace ClangCodeModel
