// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/qmt_global.h"
#include "qmt/model/mclassmember.h"

#include <QList>
#include <QPlainTextEdit>

namespace qmt {

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
