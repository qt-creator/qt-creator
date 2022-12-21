// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor_global.h>

#include "assistproposaliteminterface.h"

#include <QIcon>
#include <QString>
#include <QVariant>

namespace TextEditor {

class TEXTEDITOR_EXPORT AssistProposalItem : public AssistProposalItemInterface
{
public:
    ~AssistProposalItem() noexcept override = default;
    QString text() const override;
    bool implicitlyApplies() const override;
    bool prematurelyApplies(const QChar &c) const override;
    void apply(TextDocumentManipulatorInterface &manipulator, int basePosition) const override;

    void setIcon(const QIcon &icon);
    QIcon icon() const final;

    void setText(const QString &text);

    void setDetail(const QString &detail);
    QString detail() const final;

    void setData(const QVariant &var);
    const QVariant &data() const;

    bool isSnippet() const final;
    bool isValid() const final;
    quint64 hash() const override;

    virtual void applyContextualContent(TextDocumentManipulatorInterface &manipulator, int basePosition) const;
    virtual void applySnippet(TextDocumentManipulatorInterface &manipulator, int basePosition) const;
    virtual void applyQuickFix(TextDocumentManipulatorInterface &manipulator, int basePosition) const;

private:
    QIcon m_icon;
    QString m_text;
    QString m_detail;
    QVariant m_data;
};

} // namespace TextEditor
