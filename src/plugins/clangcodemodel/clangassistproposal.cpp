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

#include "clangassistproposal.h"

#include <texteditor/texteditor.h>

namespace ClangCodeModel {
namespace Internal {

ClangAssistProposal::ClangAssistProposal(int cursorPos, TextEditor::GenericProposalModel *model)
    : GenericProposal(cursorPos, model)
{
}

bool ClangAssistProposal::isCorrective(TextEditor::TextEditorWidget *editorWidget) const
{
    auto clangAssistProposalModel = static_cast<ClangAssistProposalModel*>(model());

    return clangAssistProposalModel->neededCorrection()
                == ClangBackEnd::CompletionCorrection::DotToArrowCorrection
        && editorWidget->textAt(basePosition() - 1, 1) == ".";
}

void ClangAssistProposal::makeCorrection(TextEditor::TextEditorWidget *editorWidget)
{
    const int oldPosition = editorWidget->position();
    editorWidget->setCursorPosition(basePosition() - 1);
    editorWidget->replace(1, QLatin1String("->"));
    editorWidget->setCursorPosition(oldPosition + 1);
    moveBasePosition(1);
}

} // namespace Internal
} // namespace ClangCodeModel

