/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "documentquickfixfactory.h"

#include "clangfixitsrefactoringchanges.h"
#include "clangtoolsdiagnostic.h"
#include "documentclangtoolrunner.h"

#include <texteditor/refactoringchanges.h>
#include <utils/qtcassert.h>

namespace ClangTools {
namespace Internal {

class ClangToolQuickFixOperation : public TextEditor::QuickFixOperation
{
public:
    explicit ClangToolQuickFixOperation(const Diagnostic &diagnostic)
        : m_diagnostic(diagnostic)
    {}

    QString description() const override { return m_diagnostic.description; }
    void perform() override;

private:
    const Diagnostic m_diagnostic;
};

using Range = TextEditor::RefactoringFile::Range;
using DiagnosticRange = QPair<Debugger::DiagnosticLocation, Debugger::DiagnosticLocation>;

static Range toRange(const QTextDocument *doc, DiagnosticRange locations)
{
    Range range;
    range.start = Utils::Text::positionInText(doc, locations.first.line, locations.first.column);
    range.end = Utils::Text::positionInText(doc, locations.second.line, locations.second.column);
    return range;
}

void ClangToolQuickFixOperation::perform()
{
    TextEditor::RefactoringChanges changes;
    QMap<QString, TextEditor::RefactoringFilePtr> refactoringFiles;

    for (const ExplainingStep &step : m_diagnostic.explainingSteps) {
        if (!step.isFixIt)
            continue;
        TextEditor::RefactoringFilePtr &refactoringFile
            = refactoringFiles[step.location.filePath.toString()];
        if (refactoringFile.isNull())
            refactoringFile = changes.file(step.location.filePath);
        Utils::ChangeSet changeSet = refactoringFile->changeSet();
        Range range = toRange(refactoringFile->document(), {step.ranges.first(), step.ranges.last()});
        changeSet.replace(range, step.message);
        refactoringFile->setChangeSet(changeSet);
    }

    for (const TextEditor::RefactoringFilePtr &refactoringFile : qAsConst(refactoringFiles))
        refactoringFile->apply();
}

DocumentQuickFixFactory::DocumentQuickFixFactory(DocumentQuickFixFactory::RunnerCollector runnerCollector)
    : m_runnerCollector(runnerCollector)
{}

void DocumentQuickFixFactory::match(const CppEditor::Internal::CppQuickFixInterface &interface,
                                    QuickFixOperations &result)
{
    QTC_ASSERT(m_runnerCollector, return );
    if (DocumentClangToolRunner *runner = m_runnerCollector(interface.filePath())) {
        const QTextBlock &block = interface.textDocument()->findBlock(interface.position());
        if (!block.isValid())
            return;

        const int lineNumber = block.blockNumber() + 1;

        for (Diagnostic diagnostic : runner->diagnosticsAtLine(lineNumber)) {
            if (diagnostic.hasFixits)
                result << new ClangToolQuickFixOperation(diagnostic);
        }
    }
}

} // namespace Internal
} // namespace ClangTools
