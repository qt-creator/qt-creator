// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "iassistproposalwidget.h"


namespace TextEditor {

struct FunctionHintProposalWidgetPrivate;

class TEXTEDITOR_EXPORT FunctionHintProposalWidget : public IAssistProposalWidget
{
    Q_OBJECT

public:
    FunctionHintProposalWidget();
    ~FunctionHintProposalWidget() override;

    void setAssistant(CodeAssistant *assistant) override;
    void setKind(AssistKind kind) override;
    void setUnderlyingWidget(const QWidget *underlyingWidget) override;
    void setModel(ProposalModelPtr model) override;
    void setDisplayRect(const QRect &rect) override;
    void setIsSynchronized(bool isSync) override;

    void showProposal(const QString &prefix) override;
    void filterProposal(const QString &prefix) override;
    void closeProposal() override;

    bool proposalIsVisible() const override;

protected:
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    void nextPage();
    void previousPage();
    bool updateAndCheck(const QString &prefix);
    void updateContent();
    void updatePosition();
    void abort();

    int loadSelectedHint() const;
    void storeSelectedHint();

private:
    FunctionHintProposalWidgetPrivate *d;
};

} // TextEditor
