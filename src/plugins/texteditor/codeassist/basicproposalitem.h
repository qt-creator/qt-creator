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

#ifndef BASICPROPOSALITEM_H
#define BASICPROPOSALITEM_H

#include "iassistproposalitem.h"

#include <texteditor/texteditor_global.h>

#include <utils/qtcoverride.h>

#include <QVariant>
#include <QIcon>

namespace TextEditor {

class TEXTEDITOR_EXPORT BasicProposalItem : public IAssistProposalItem
{
public:
    BasicProposalItem();
    ~BasicProposalItem();

    void setIcon(const QIcon &icon);
    const QIcon &icon() const;

    void setText(const QString &text);
    QString text() const QTC_OVERRIDE;

    void setDetail(const QString &detail);
    const QString &detail() const;

    void setData(const QVariant &var);
    const QVariant &data() const;

    int order() const;
    void setOrder(int order);

    bool implicitlyApplies() const QTC_OVERRIDE;
    bool prematurelyApplies(const QChar &c) const QTC_OVERRIDE;
    void apply(BaseTextEditor *editor, int basePosition) const QTC_OVERRIDE;
    virtual void applyContextualContent(BaseTextEditor *editor, int basePosition) const;
    virtual void applySnippet(BaseTextEditor *editor, int basePosition) const;
    virtual void applyQuickFix(BaseTextEditor *editor, int basePosition) const;

private:
    QIcon m_icon;
    QString m_text;
    QString m_detail;
    QVariant m_data;
    int m_order;
};

} // TextEditor

#endif // BASICPROPOSALITEM_H
