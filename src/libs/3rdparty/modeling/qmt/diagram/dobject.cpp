/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "dobject.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DObject::DObject()
    : DElement(),
      _model_uid(Uid::getInvalidUid()),
      _depth(0),
      _visual_primary_role(PRIMARY_ROLE_NORMAL),
      _visual_secondary_role(SECONDARY_ROLE_NONE),
      _stereotype_display(STEREOTYPE_SMART),
      _auto_sized(true),
      _visual_emphasized(false)
{
}

DObject::DObject(const DObject &rhs)
    : DElement(rhs),
      _model_uid(rhs._model_uid),
      _context(rhs._context),
      _name(rhs._name),
      _pos(rhs._pos),
      _rect(rhs._rect),
      _depth(rhs._depth),
      _visual_primary_role(rhs._visual_primary_role),
      _visual_secondary_role(rhs._visual_secondary_role),
      _stereotype_display(rhs._stereotype_display),
      _auto_sized(rhs._auto_sized),
      _visual_emphasized(rhs._visual_emphasized)
{
}

DObject::~DObject()
{
}

DObject &DObject::operator =(const DObject &rhs)
{
    if (this != &rhs) {
        DElement::operator=(rhs);
        _model_uid = rhs._model_uid;
        _context = rhs._context;
        _name = rhs._name;
        _pos = rhs._pos;
        _rect = rhs._rect;
        _depth = rhs._depth;
        _visual_primary_role = rhs._visual_primary_role;
        _visual_secondary_role = rhs._visual_secondary_role;
        _stereotype_display = rhs._stereotype_display;
        _auto_sized = rhs._auto_sized;
        _visual_emphasized = rhs._visual_emphasized;
    }
    return *this;
}

void DObject::setModelUid(const Uid &uid)
{
    _model_uid = uid;
}

void DObject::setStereotypes(const QList<QString> &stereotypes)
{
    _stereotypes = stereotypes;
}

void DObject::setContext(const QString &context)
{
    _context = context;
}

void DObject::setName(const QString &name)
{
    _name = name;
}

void DObject::setPos(const QPointF &pos)
{
    _pos = pos;
}

void DObject::setRect(const QRectF &rect)
{
    _rect = rect;
}

void DObject::setDepth(int depth)
{
    _depth = depth;
}

void DObject::setVisualPrimaryRole(DObject::VisualPrimaryRole visual_primary_role)
{
    _visual_primary_role = visual_primary_role;
}

void DObject::setVisualSecondaryRole(DObject::VisualSecondaryRole visual_secondary_role)
{
    _visual_secondary_role = visual_secondary_role;
}

void DObject::setStereotypeDisplay(DObject::StereotypeDisplay stereotype_display)
{
    _stereotype_display = stereotype_display;
}

void DObject::setAutoSize(bool auto_sized)
{
    _auto_sized = auto_sized;
}

void DObject::setVisualEmphasized(bool visual_emphasized)
{
    _visual_emphasized = visual_emphasized;
}

void DObject::accept(DVisitor *visitor)
{
    visitor->visitDObject(this);
}

void DObject::accept(DConstVisitor *visitor) const
{
    visitor->visitDObject(this);
}

}
