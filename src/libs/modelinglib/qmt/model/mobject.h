// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "melement.h"
#include "qmt/infrastructure/handles.h"

#include <QString>

namespace qmt {

class MRelation;

class QMT_EXPORT MObject : public MElement
{
public:
    MObject();
    MObject(const MObject &rhs);
    ~MObject() override;

    MObject &operator=(const MObject &rhs);

    QString name() const { return m_name; }
    void setName(const QString &name);

    const Handles<MObject> &children() const { return m_children; }
    void setChildren(const Handles<MObject> &children);
    void addChild(const Uid &uid);
    void addChild(MObject *child);
    void insertChild(int beforeIndex, const Uid &uid);
    void insertChild(int beforeIndex, MObject *child);
    void removeChild(const Uid &uid);
    void removeChild(MObject *child);
    void decontrolChild(const Uid &uid);
    void decontrolChild(MObject *child);

    const Handles<MRelation> &relations() const { return m_relations; }
    void setRelations(const Handles<MRelation> &relations);
    void addRelation(const Uid &uid);
    void addRelation(MRelation *relation);
    void insertRelation(int beforeIndex, MRelation *relation);
    void removeRelation(MRelation *relation);
    void decontrolRelation(MRelation *relation);

    void accept(MVisitor *visitor) override;
    void accept(MConstVisitor *visitor) const override;

private:
    QString m_name;
    Handles<MObject> m_children;
    Handles<MRelation> m_relations;
};

} // namespace qmt
