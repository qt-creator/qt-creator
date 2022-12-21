// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "assistenums.h"
#include "iassistproposalmodel.h"

#include <texteditor/texteditor_global.h>

#include <QFrame>

namespace Utils { class Id; }

namespace TextEditor {

class AssistInterface;
class AssistProposalItemInterface;
class CodeAssistant;

class TEXTEDITOR_EXPORT IAssistProposalWidget  : public QFrame
{
    Q_OBJECT

public:
    IAssistProposalWidget();
    ~IAssistProposalWidget() override;

    virtual void setAssistant(CodeAssistant *assistant) = 0;
    virtual void setReason(AssistReason reason) { m_reason = reason; }
    virtual void setKind(AssistKind kind) = 0;
    virtual void setUnderlyingWidget(const QWidget *underlyingWidget) = 0;
    virtual void setModel(ProposalModelPtr model) = 0;
    virtual void setDisplayRect(const QRect &rect) = 0;
    virtual void setIsSynchronized(bool isSync) = 0;

    virtual void showProposal(const QString &prefix) = 0;
    virtual void filterProposal(const QString &prefix) = 0;
    virtual void updateProposal(std::unique_ptr<AssistInterface> &&interface);
    virtual void closeProposal() = 0;

    virtual bool proposalIsVisible() const { return isVisible(); }

    int basePosition() const;
    void setBasePosition(int basePosition);

    void setFragile(bool fragile) { m_isFragile = fragile; }
    bool isFragile() const { return m_isFragile; }

    AssistReason reason() const { return m_reason; }

signals:
    void prefixExpanded(const QString &newPrefix);
    void proposalItemActivated(AssistProposalItemInterface *proposalItem);
    void explicitlyAborted();

protected:
    int m_basePosition = -1;
    bool m_isFragile = false;
    AssistReason m_reason = IdleEditor;
};

} // TextEditor
