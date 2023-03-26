// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "iassistproposalwidget.h"

#include "genericproposalmodel.h"

#include <texteditor/texteditor_global.h>


namespace TextEditor {

class GenericProposalWidgetPrivate;

class TEXTEDITOR_EXPORT GenericProposalWidget : public IAssistProposalWidget
{
    Q_OBJECT
    friend class GenericProposalWidgetPrivate;

public:
    GenericProposalWidget();
    ~GenericProposalWidget() override;

    void setAssistant(CodeAssistant *assistant) override;
    void setReason(AssistReason reason) override;
    void setKind(AssistKind kind) override;
    void setUnderlyingWidget(const QWidget *underlyingWidget) override;
    void setModel(ProposalModelPtr model) override;
    void setDisplayRect(const QRect &rect) override;
    void setIsSynchronized(bool isSync) override;

    void updateModel(GenericProposalModelPtr model, const QString &prefix);

    void showProposal(const QString &prefix) override;
    void filterProposal(const QString &prefix) override;
    void closeProposal() override;

private:
    bool updateAndCheck(const QString &prefix);
    void notifyActivation(int index);
    void abort();
    void updatePositionAndSize();
    void turnOffAutoWidth();
    void turnOnAutoWidth();

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
    bool activateCurrentProposalItem();
    TextEditor::GenericProposalModelPtr model();

private:
    GenericProposalWidgetPrivate *d;
};

} // TextEditor
