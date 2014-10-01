/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FUNCTIONHINTPROPOSALWIDGET_H
#define FUNCTIONHINTPROPOSALWIDGET_H

#include "iassistproposalwidget.h"

#include <utils/qtcoverride.h>

namespace TextEditor {

struct FunctionHintProposalWidgetPrivate;

class TEXTEDITOR_EXPORT FunctionHintProposalWidget : public IAssistProposalWidget
{
    Q_OBJECT

public:
    FunctionHintProposalWidget();
    ~FunctionHintProposalWidget();

    void setAssistant(CodeAssistant *assistant) QTC_OVERRIDE;
    void setReason(AssistReason reason) QTC_OVERRIDE;
    void setKind(AssistKind kind) QTC_OVERRIDE;
    void setUnderlyingWidget(const QWidget *underlyingWidget) QTC_OVERRIDE;
    void setModel(IAssistProposalModel *model) QTC_OVERRIDE;
    void setDisplayRect(const QRect &rect) QTC_OVERRIDE;
    void setIsSynchronized(bool isSync) QTC_OVERRIDE;

    void showProposal(const QString &prefix) QTC_OVERRIDE;
    void updateProposal(const QString &prefix) QTC_OVERRIDE;
    void closeProposal() QTC_OVERRIDE;

protected:
    bool eventFilter(QObject *o, QEvent *e) QTC_OVERRIDE;

private slots:
    void nextPage();
    void previousPage();

private:
    bool updateAndCheck(const QString &prefix);
    void updateContent();
    void updatePosition();
    void abort();

private:
    FunctionHintProposalWidgetPrivate *d;
};

} // TextEditor

#endif // FUNCTIONHINTPROPOSALWIDGET_H
