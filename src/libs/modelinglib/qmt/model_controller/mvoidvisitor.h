// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/model/mvisitor.h"
#include "qmt/model/mconstvisitor.h"
#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class QMT_EXPORT MVoidVisitor : public MVisitor
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

class QMT_EXPORT MVoidConstVisitor : public MConstVisitor
{
public:
    void visitMElement(const MElement *element) override;
    void visitMObject(const MObject *object) override;
    void visitMPackage(const MPackage *package) override;
    void visitMClass(const MClass *klass) override;
    void visitMComponent(const MComponent *component) override;
    void visitMDiagram(const MDiagram *diagram) override;
    void visitMCanvasDiagram(const MCanvasDiagram *diagram) override;
    void visitMItem(const MItem *item) override;
    void visitMRelation(const MRelation *relation) override;
    void visitMDependency(const MDependency *dependency) override;
    void visitMInheritance(const MInheritance *inheritance) override;
    void visitMAssociation(const MAssociation *association) override;
    void visitMConnection(const MConnection *connection) override;
};

} // namespace qmt
