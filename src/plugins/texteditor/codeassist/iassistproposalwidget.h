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

#include "assistenums.h"

#include <texteditor/texteditor_global.h>

#include <QFrame>

namespace TextEditor {

class CodeAssistant;
class IAssistProposalModel;
class AssistProposalItemInterface;

class TEXTEDITOR_EXPORT IAssistProposalWidget  : public QFrame
{
    Q_OBJECT

public:
    IAssistProposalWidget();
    virtual ~IAssistProposalWidget();

    virtual void setAssistant(CodeAssistant *assistant) = 0;
    virtual void setReason(AssistReason reason) = 0;
    virtual void setKind(AssistKind kind) = 0;
    virtual void setUnderlyingWidget(const QWidget *underlyingWidget) = 0;
    virtual void setModel(IAssistProposalModel *model) = 0;
    virtual void setDisplayRect(const QRect &rect) = 0;
    virtual void setIsSynchronized(bool isSync) = 0;

    virtual void showProposal(const QString &prefix) = 0;
    virtual void updateProposal(const QString &prefix) = 0;
    virtual void closeProposal() = 0;

signals:
    void prefixExpanded(const QString &newPrefix);
    void proposalItemActivated(AssistProposalItemInterface *proposalItem);
    void explicitlyAborted();
};

} // TextEditor
