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
