/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef IASSISTPROPOSALWIDGET_H
#define IASSISTPROPOSALWIDGET_H

#include "assistenums.h"

#include <texteditor/texteditor_global.h>

#include <QFrame>

namespace TextEditor {

class CodeAssistant;
class IAssistProposalModel;
class IAssistProposalItem;

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
    void proposalItemActivated(IAssistProposalItem *proposalItem);
};

} // TextEditor

#endif // IASSISTPROPOSALWIDGET_H
