// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/model/mconstvisitor.h"
#include "qmt/infrastructure/qmt_global.h"

#include <qglobal.h>

namespace qmt {

class QMT_EXPORT MCloneVisitor : public MConstVisitor
{
public:
    MCloneVisitor();

    MElement *cloned() const { return m_cloned; }

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

private:
    MElement *m_cloned = nullptr;
};

class QMT_EXPORT MCloneDeepVisitor : public MConstVisitor
{
public:
    MCloneDeepVisitor();

    MElement *cloned() const { return m_cloned; }

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

private:
    MElement *m_cloned = nullptr;
};

} // namespace qmt
