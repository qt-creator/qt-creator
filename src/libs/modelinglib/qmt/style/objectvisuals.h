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

#include "qmt/diagram/dobject.h"

#include <QColor>

namespace qmt {

class QMT_EXPORT ObjectVisuals
{
public:
    ObjectVisuals();
    ObjectVisuals(DObject::VisualPrimaryRole visualPrimaryRole,
                  DObject::VisualSecondaryRole visualSecondaryRole,
                  bool emphasized, const QColor &baseColor, int depth);
    ~ObjectVisuals();

    DObject::VisualPrimaryRole visualPrimaryRole() const { return m_visualPrimaryRole; }
    void setVisualPrimaryRole(DObject::VisualPrimaryRole visualPrimaryRole);
    DObject::VisualSecondaryRole visualSecondaryRole() const { return m_visualSecondaryRole; }
    void setVisualSecondaryRole(DObject::VisualSecondaryRole visualSecondaryRole);
    bool isEmphasized() const { return m_isEmphasized; }
    void setEmphasized(bool emphasized);
    QColor baseColor() const { return m_baseColor; }
    void setBaseColor(const QColor &baseColor);
    int depth() const { return m_depth; }
    void setDepth(int depth);

private:
    DObject::VisualPrimaryRole m_visualPrimaryRole;
    DObject::VisualSecondaryRole m_visualSecondaryRole;
    bool m_isEmphasized;
    QColor m_baseColor;
    int m_depth;
};

bool operator==(const ObjectVisuals &lhs, const ObjectVisuals &rhs);
uint qHash(const ObjectVisuals &objectVisuals);

} // namespace qmt
