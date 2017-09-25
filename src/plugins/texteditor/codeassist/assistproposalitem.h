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

#include <texteditor/texteditor_global.h>

#include "assistproposaliteminterface.h"

#include <QIcon>
#include <QString>
#include <QVariant>

namespace TextEditor {


class TEXTEDITOR_EXPORT AssistProposalItem : public AssistProposalItemInterface
{
public:
    ~AssistProposalItem() Q_DECL_NOEXCEPT {}
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
