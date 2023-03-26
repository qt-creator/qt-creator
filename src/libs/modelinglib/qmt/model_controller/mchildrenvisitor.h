// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/model/mvisitor.h"
#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class QMT_EXPORT MChildrenVisitor : public MVisitor
{
public:
    void visitMElement(MElement *element) override;
    void visitMObject(MObject *object) override;
    void visitMPackage(MPackage *package) override;
    void visitMClass(MClass *klass) override;
    void visitMComponent(MComponent *component) override;
    void visitMDiagram(MDiagram *diagram) override;
    void visitMCanvasDiagram(MCanvasDiagram *diagram) override;
    void visitMItem(MItem *item) override;
    void visitMRelation(MRelation *relation) override;
    void visitMDependency(MDependency *dependency) override;
    void visitMInheritance(MInheritance *inheritance) override;
    void visitMAssociation(MAssociation *association) override;
    void visitMConnection(MConnection *connection) override;
};

} // namespace qmt
