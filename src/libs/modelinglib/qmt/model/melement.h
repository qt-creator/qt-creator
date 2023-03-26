// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/uid.h"

#include <QList>
#include <QString>

namespace qmt {

class MElement;
class MObject;
class MVisitor;
class MConstVisitor;

class MExpansion
{
public:
    virtual ~MExpansion() { }

    virtual MExpansion *clone(const MElement &rhs) const = 0;
    virtual void assign(MElement *lhs, const MElement &rhs);
    virtual void destroy(MElement *element);
};

class QMT_EXPORT MElement
{
    friend class MExpansion;

public:
    enum Flag {
        ReverseEngineered = 0x01
    };

    Q_DECLARE_FLAGS(Flags, Flag)

    MElement();
    MElement(const MElement &rhs);
    virtual ~MElement();

    MElement &operator=(const MElement &rhs);

    Uid uid() const { return m_uid; }
    void setUid(const Uid &uid);
    void renewUid();
    MObject *owner() const { return m_owner; }
    void setOwner(MObject *owner);
    MExpansion *expansion() const { return m_expansion; }
    void setExpansion(MExpansion *expansion);
    QList<QString> stereotypes() const { return m_stereotypes; }
    void setStereotypes(const QList<QString> &stereotypes);
    Flags flags() const { return m_flags; }
    void setFlags(const Flags &flags);

    virtual void accept(MVisitor *visitor);
    virtual void accept(MConstVisitor *visitor) const;

private:
    Uid m_uid;
    MObject *m_owner = nullptr;
    MExpansion *m_expansion = nullptr;
    QList<QString> m_stereotypes;
    Flags m_flags;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MElement::Flags)

} // namespace qmt
