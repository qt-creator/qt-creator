// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "finddiagramvisitor.h"

#include "qmt/model/mobject.h"
#include "qmt/model/mdiagram.h"

namespace qmt {

FindDiagramVisitor::FindDiagramVisitor()
{
}

FindDiagramVisitor::~FindDiagramVisitor()
{
}

void FindDiagramVisitor::visitMObject(const MObject *object)
{
    for (const Handle<MObject> &child : object->children()) {
        if (child.hasTarget()) {
            if (auto diagram = dynamic_cast<MDiagram *>(child.target())) {
                m_diagram = diagram;
                return;
            }
        }
    }
}

void FindDiagramVisitor::visitMDiagram(const MDiagram *diagram)
{
    m_diagram = diagram;
}

} // namespace qmt
