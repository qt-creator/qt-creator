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

#include <codecompletion.h>

#include <texteditor/codeassist/assistproposaliteminterface.h>

#include <QString>

namespace ClangCodeModel {
namespace Internal {

class ClangAssistProposalItem final : public TextEditor::AssistProposalItemInterface
{
    friend bool operator<(const ClangAssistProposalItem &first, const ClangAssistProposalItem &second);
public:
    ~ClangAssistProposalItem() Q_DECL_NOEXCEPT {}
    bool prematurelyApplies(const QChar &typedCharacter) const final;
    bool implicitlyApplies() const final;
    void apply(TextEditor::TextDocumentManipulatorInterface &manipulator, int basePosition) const final;

    void setText(const QString &text);
    QString text() const final;
    QIcon icon() const final;
    QString detail() const final;
    bool isSnippet() const final;
    bool isValid() const final;
    quint64 hash() const final;

    void keepCompletionOperator(unsigned compOp);

    bool hasOverloadsWithParameters() const;
    void setHasOverloadsWithParameters(bool hasOverloadsWithParameters);

    void setCodeCompletion(const ClangBackEnd::CodeCompletion &codeCompletion);
    const ClangBackEnd::CodeCompletion &codeCompletion() const;

private:
    ClangBackEnd::CodeCompletion m_codeCompletion;
    QList<ClangBackEnd::CodeCompletion> m_overloads;
    bool m_hasOverloadsWithParameters = false;
    QString m_text;
    unsigned m_completionOperator;
    mutable QChar m_typedCharacter;
};

} // namespace Internal
} // namespace ClangCodeModel
