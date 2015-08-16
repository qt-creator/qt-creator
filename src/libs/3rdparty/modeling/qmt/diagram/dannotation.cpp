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

#include "dannotation.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DAnnotation::DAnnotation()
    : DElement(),
      _visual_role(ROLE_NORMAL),
      _auto_sized(true)
{
}

DAnnotation::DAnnotation(const DAnnotation &rhs)
    : DElement(rhs),
      _text(rhs._text),
      _pos(rhs._pos),
      _rect(rhs._rect),
      _visual_role(rhs._visual_role),
      _auto_sized(rhs._auto_sized)
{
}

DAnnotation::~DAnnotation()
{
}

DAnnotation &DAnnotation::operator=(const DAnnotation &rhs)
{
    if (this != &rhs) {
        DElement::operator=(rhs);
        _text = rhs._text;
        _pos = rhs._pos;
        _rect = rhs._rect;
        _visual_role = rhs._visual_role;
        _auto_sized = rhs._auto_sized;
    }
    return *this;
}

void DAnnotation::setText(const QString &text)
{
    _text = text;
}

void DAnnotation::setPos(const QPointF &pos)
{
    _pos = pos;
}

void DAnnotation::setRect(const QRectF &rect)
{
    _rect = rect;
}

void DAnnotation::setVisualRole(DAnnotation::VisualRole visual_role)
{
    _visual_role = visual_role;
}

void DAnnotation::setAutoSize(bool auto_sized)
{
    _auto_sized = auto_sized;
}

void DAnnotation::accept(DVisitor *visitor)
{
    visitor->visitDAnnotation(this);
}

void DAnnotation::accept(DConstVisitor *visitor) const
{
    visitor->visitDAnnotation(this);
}

}
