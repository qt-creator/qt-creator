// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "melement.h"

#include "qmt/infrastructure/handle.h"

namespace qmt {

class MObject;

class QMT_EXPORT MRelation : public MElement
{
public:
    MRelation();
    MRelation(const MRelation &rhs);
    ~MRelation() override;

    MRelation &operator=(const MRelation &rhs);

    QString name() const { return m_name; }
    void setName(const QString &name);
    Uid endAUid() const { return m_endAUid; }
    void setEndAUid(const Uid &uid);
    Uid endBUid() const { return m_endBUid; }
    void setEndBUid(const Uid &uid);

    void accept(MVisitor *visitor) override;
    void accept(MConstVisitor *visitor) const override;

private:
    QString m_name;
    Uid m_endAUid;
    Uid m_endBUid;
};

} // namespace qmt
