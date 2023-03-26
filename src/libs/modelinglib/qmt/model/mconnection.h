// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
