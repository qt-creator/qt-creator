// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "drelation.h"

#include "qmt/model/massociation.h"

namespace qmt {

class QMT_EXPORT DAssociationEnd
{
public:
    DAssociationEnd();
    ~DAssociationEnd();

    QString name() const { return m_name; }
    void setName(const QString &name);
    QString cardinality() const { return m_cardinality; }
    void setCardinality(const QString &cardinality);
    MAssociationEnd::Kind kind() const { return m_kind; }
    void setKind(MAssociationEnd::Kind kind);
    bool isNavigable() const { return m_isNavigable; }
    void setNavigable(bool navigable);

private:
    QString m_name;
    QString m_cardinality;
    MAssociationEnd::Kind m_kind = MAssociationEnd::Association;
    bool m_isNavigable = false;

};

bool operator==(const DAssociationEnd &lhs, const DAssociationEnd &rhs);
bool operator!=(const DAssociationEnd &lhs, const DAssociationEnd &rhs);

class QMT_EXPORT DAssociation : public DRelation
{
public:
    DAssociation();
    ~DAssociation() override;

    DAssociationEnd endA() const { return m_endA; }
    void setEndA(const DAssociationEnd &endA);
    DAssociationEnd endB() const { return m_endB; }
    void setEndB(const DAssociationEnd &endB);
    Uid assoicationClassUid() const { return m_associationClassUid; }
    void setAssociationClassUid(const Uid &uid);

    void accept(DVisitor *visitor) override;
    void accept(DConstVisitor *visitor) const override;

private:
    DAssociationEnd m_endA;
    DAssociationEnd m_endB;
    Uid m_associationClassUid;
};

} // namespace qmt
