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

#include "mrelation.h"
#include "qmt/infrastructure/handle.h"

#include <QString>

namespace qmt {

class QMT_EXPORT MConnectionEnd
{
public:
    MConnectionEnd();
    MConnectionEnd(const MConnectionEnd &rhs);
    ~MConnectionEnd();

    MConnectionEnd &operator=(const MConnectionEnd &rhs);

    QString name() const { return m_name; }
    void setName(const QString &name);
    QString cardinality() const { return m_cardinality; }
    void setCardinality(const QString &cardinality);
    bool isNavigable() const { return m_isNavigable; }
    void setNavigable(bool navigable);

private:
    QString m_name;
    QString m_cardinality;
    bool m_isNavigable = false;
};

bool operator==(const MConnectionEnd &lhs, const MConnectionEnd &rhs);
inline bool operator!=(const MConnectionEnd &lhs, const MConnectionEnd &rhs)
{
    return !(lhs == rhs);
}

class QMT_EXPORT MConnection : public MRelation
{
public:
    MConnection();
    ~MConnection() override;

    QString customRelationId() const { return m_customRelationId; }
    void setCustomRelationId(const QString &customRelationId);
    MConnectionEnd endA() const { return m_endA; }
    void setEndA(const MConnectionEnd &end);
    MConnectionEnd endB() const { return m_endB; }
    void setEndB(const MConnectionEnd &end);

    void accept(MVisitor *visitor) override;
    void accept(MConstVisitor *visitor) const override;

private:
    QString m_customRelationId;
    MConnectionEnd m_endA;
    MConnectionEnd m_endB;
};

} // namespace qmt
