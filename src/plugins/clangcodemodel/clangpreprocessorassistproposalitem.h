// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/codeassist/assistproposaliteminterface.h>

#include <QIcon>
#include <QString>

namespace ClangCodeModel {
namespace Internal {

class ClangPreprocessorAssistProposalItem final : public TextEditor::AssistProposalItemInterface
{
public:
    ~ClangPreprocessorAssistProposalItem() noexcept override = default;
    bool prematurelyApplies(const QChar &typedChar) const final;
    bool implicitlyApplies() const final;
    void apply(TextEditor::TextDocumentManipulatorInterface &manipulator,
               int basePosition) const final;

    void setText(const QString &text);
    QString text() const final;

    void setIcon(const QIcon &icon);
    QIcon icon() const final;

    void setDetail(const QString &detail);
    QString detail() const final;

    Qt::TextFormat detailFormat() const final;

    bool isSnippet() const final;
    bool isValid() const final;

    quint64 hash() const final;

    void setCompletionOperator(uint completionOperator);

private:
    bool isInclude() const;

private:
    QString m_text;
    QString m_detail;
    QIcon m_icon;
    uint m_completionOperator;
    mutable QChar m_typedCharacter;
};

} // namespace Internal
} // namespace ClangCodeModel
