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

#include "clangassistproposalitem.h"

#include "clangassistproposalmodel.h"

#include <texteditor/codeassist/assistproposaliteminterface.h>

#include <algorithm>

namespace ClangCodeModel {
namespace Internal {

const int SORT_LIMIT = 30000;

ClangAssistProposalModel::ClangAssistProposalModel(
        ClangBackEnd::CompletionCorrection neededCorrection)
    : m_neededCorrection(neededCorrection)
{
}

bool ClangAssistProposalModel::containsDuplicates() const
{
    return false;
}

bool ClangAssistProposalModel::isSortable(const QString &/*prefix*/) const
{
    return m_currentItems.size() <= SORT_LIMIT;
}

void ClangAssistProposalModel::sort(const QString &/*prefix*/)
{
    using TextEditor::AssistProposalItemInterface;

    auto currentItemsCompare = [](AssistProposalItemInterface *first,
                                  AssistProposalItemInterface *second) {
        return (first->order() > 0
                && (first->order() < second->order()
                     || (first->order() == second->order() && first->text() < second->text())));
    };

    std::sort(m_currentItems.begin(), m_currentItems.end(), currentItemsCompare);
}

ClangBackEnd::CompletionCorrection ClangAssistProposalModel::neededCorrection() const
{
    return m_neededCorrection;
}

} // namespace Internal
} // namespace ClangCodeModel

