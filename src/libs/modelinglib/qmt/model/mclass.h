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
