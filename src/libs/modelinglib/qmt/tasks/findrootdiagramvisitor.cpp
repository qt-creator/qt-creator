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

#include "findrootdiagramvisitor.h"

#include "qmt/model/mdiagram.h"

namespace qmt {

FindRootDiagramVisitor::FindRootDiagramVisitor()
    : MVoidVisitor(),
      m_diagram(nullptr)
{
}

FindRootDiagramVisitor::~FindRootDiagramVisitor()
{
}

void FindRootDiagramVisitor::visitMObject(MObject *object)
{
    // first search flat
    foreach(const Handle<MObject> &child, object->children()) {
        if (child.hasTarget()) {
            auto diagram = dynamic_cast<MDiagram *>(child.target());
            if (diagram) {
                m_diagram = diagram;
                return;
            }
        }
    }
    // then search in children
    foreach(const Handle<MObject> &child, object->children()) {
        if (child.hasTarget()) {
            child.target()->accept(this);
            if (m_diagram)
                return;
        }
    }
    visitMElement(object);
}

} // namespace qmt
