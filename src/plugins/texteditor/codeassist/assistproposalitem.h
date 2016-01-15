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

#ifndef IASSISTPROPOSALITEM_H
#define IASSISTPROPOSALITEM_H

#include <texteditor/texteditor_global.h>


#include <QIcon>
#include <QString>
#include <QVariant>

namespace TextEditor {

class TextEditorWidget;

class TEXTEDITOR_EXPORT AssistProposalItem
{
public:
    AssistProposalItem();
    virtual ~AssistProposalItem();

    virtual QString text() const;
    virtual bool implicitlyApplies() const;
    virtual bool prematurelyApplies(const QChar &c) const;
    virtual void apply(TextEditorWidget *editorWidget, int basePosition) const;

    void setIcon(const QIcon &icon);
    const QIcon &icon() const;

    void setText(const QString &text);

    void setDetail(const QString &detail);
    const QString &detail() const;

    void setData(const QVariant &var);
    const QVariant &data() const;

    int order() const;
    void setOrder(int order);

    virtual void applyContextualContent(TextEditorWidget *editorWidget, int basePosition) const;
    virtual void applySnippet(TextEditorWidget *editorWidget, int basePosition) const;
    virtual void applyQuickFix(TextEditorWidget *editorWidget, int basePosition) const;

private:
    QIcon m_icon;
    QString m_text;
    QString m_detail;
    QVariant m_data;
    int m_order;
};

} // namespace TextEditor

#endif // BASICPROPOSALITEM_H
