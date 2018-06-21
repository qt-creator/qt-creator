/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include <QPlainTextEdit>
#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class MClassMember;

class QMT_EXPORT ClassMembersEdit : public QPlainTextEdit
{
    Q_OBJECT
    class Cursor;
    class ClassMembersEditPrivate;

public:
    explicit ClassMembersEdit(QWidget *parent = nullptr);
    ~ClassMembersEdit() override;

signals:
    void statusChanged(bool valid);
    void membersChanged(QList<MClassMember> &);

public:
    QList<MClassMember> members() const;
    void setMembers(const QList<MClassMember> &members);

    void reparse();

private:
    void onTextChanged();

    QString build(const QList<MClassMember> &members);
    QList<MClassMember> parse(const QString &text, bool *ok);

    ClassMembersEditPrivate *d;
};

} // namespace qmt
