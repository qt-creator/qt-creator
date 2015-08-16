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

#ifndef QMT_OBJECTVISUALS_H
#define QMT_OBJECTVISUALS_H

#include "qmt/diagram/dobject.h"

#include <QColor>

namespace qmt {

class QMT_EXPORT ObjectVisuals
{
public:
    ObjectVisuals();
    ObjectVisuals(DObject::VisualPrimaryRole visual_primary_role,
                  DObject::VisualSecondaryRole visual_secondary_role,
                  bool emphasized, const QColor &base_color, int depth);
    ~ObjectVisuals();

public:

    DObject::VisualPrimaryRole getVisualPrimaryRole() const { return _visual_primary_role; }

    void setVisualPrimaryRole(DObject::VisualPrimaryRole visual_primary_role);

    DObject::VisualSecondaryRole getVisualSecondaryRole() const { return _visual_secondary_role; }

    void setVisualSecondaryRole(DObject::VisualSecondaryRole visual_secondary_role);

    bool isEmphasized() const { return _emphasized; }

    void setEmphasized(bool emphasized);

    QColor getBaseColor() const { return _base_color; }

    void setBaseColor(const QColor &base_color);

    int getDepth() const { return _depth; }

    void setDepth(int depth);

private:

    DObject::VisualPrimaryRole _visual_primary_role;

    DObject::VisualSecondaryRole _visual_secondary_role;

    bool _emphasized;

    QColor _base_color;

    int _depth;
};

bool operator==(const ObjectVisuals &lhs, const ObjectVisuals &rhs);

uint qHash(const ObjectVisuals &object_visuals);

} // namespace qmt

#endif // QMT_OBJECTVISUALS_H
