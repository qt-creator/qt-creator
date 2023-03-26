// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "drelation.h"

#include "qmt/model/mconnection.h"

namespace qmt {

class QMT_EXPORT DConnectionEnd
{
public:
    DConnectionEnd();
    ~DConnectionEnd();

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

bool operator==(const DConnectionEnd &lhs, const DConnectionEnd &rhs);
bool operator!=(const DConnectionEnd &lhs, const DConnectionEnd &rhs);

class QMT_EXPORT DConnection : public DRelation
{
public:
    DConnection();
    ~DConnection() override;

    QString customRelationId() const { return m_customRelationId; }
    void setCustomRelationId(const QString &customRelationId);
    DConnectionEnd endA() const { return m_endA; }
    void setEndA(const DConnectionEnd &endA);
    DConnectionEnd endB() const { return m_endB; }
    void setEndB(const DConnectionEnd &endB);

    void accept(DVisitor *visitor) override;
    void accept(DConstVisitor *visitor) const override;

private:
    QString m_customRelationId;
    DConnectionEnd m_endA;
    DConnectionEnd m_endB;
};

} // namespace qmt
