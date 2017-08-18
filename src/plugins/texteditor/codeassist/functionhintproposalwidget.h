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

#include "iassistproposalwidget.h"


namespace TextEditor {

struct FunctionHintProposalWidgetPrivate;

class TEXTEDITOR_EXPORT FunctionHintProposalWidget : public IAssistProposalWidget
{
    Q_OBJECT

public:
    FunctionHintProposalWidget();
    ~FunctionHintProposalWidget();

    void setAssistant(CodeAssistant *assistant) override;
    void setReason(AssistReason reason) override;
    void setKind(AssistKind kind) override;
    void setUnderlyingWidget(const QWidget *underlyingWidget) override;
    void setModel(IAssistProposalModel *model) override;
    void setDisplayRect(const QRect &rect) override;
    void setIsSynchronized(bool isSync) override;

    void showProposal(const QString &prefix) override;
    void updateProposal(const QString &prefix) override;
    void closeProposal() override;

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
