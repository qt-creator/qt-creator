// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findrootdiagramvisitor.h"

#include "qmt/model/mdiagram.h"

namespace qmt {

FindRootDiagramVisitor::FindRootDiagramVisitor()
{
}

FindRootDiagramVisitor::~FindRootDiagramVisitor()
{
}

void FindRootDiagramVisitor::visitMObject(MObject *object)
{
    // first search flat
    for (const Handle<MObject> &child : object->children()) {
        if (child.hasTarget()) {
            auto diagram = dynamic_cast<MDiagram *>(child.target());
            if (diagram) {
                m_diagram = diagram;
                return;
            }
        }
    }
    // then search in children
    for (const Handle<MObject> &child : object->children()) {
        if (child.hasTarget()) {
            child.target()->accept(this);
            if (m_diagram)
                return;
        }
    }
    visitMElement(object);
}

} // namespace qmt
