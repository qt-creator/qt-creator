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
