// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mobject.h"
#include "mclassmember.h"

namespace qmt {

class MInheritance;

class QMT_EXPORT MClass : public MObject
{
public:
    MClass();
    MClass(const MClass &rhs);
    ~MClass() override;

    MClass &operator=(const MClass &rhs);

    QString umlNamespace() const { return m_umlNamespace; }
    void setUmlNamespace(const QString &umlNamespace);
    QList<QString> templateParameters() const { return m_templateParameters; }
    void setTemplateParameters(const QList<QString> &templateParameters);
    QList<MClassMember> members() const { return m_members; }
    void setMembers(const QList<MClassMember> &members);

    void addMember(const MClassMember &member);
    void insertMember(int beforeIndex, const MClassMember &member);
    void removeMember(const Uid &uid);
    void removeMember(const MClassMember &member);

    void accept(MVisitor *visitor) override;
    void accept(MConstVisitor *visitor) const override;

private:
    QString m_umlNamespace;
    QList<QString> m_templateParameters;
    QList<MClassMember> m_members;
};

} // namespace qmt
