// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/model/mconstvisitor.h"
#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class DElement;

class QMT_EXPORT DUpdateVisitor : public MConstVisitor
{
public:
    DUpdateVisitor(DElement *target, const MDiagram *diagram, bool checkNeedsUpdate = false);

    bool isUpdateNeeded() const { return m_isUpdateNeeded; }
    void setCheckNeedsUpdate(bool checkNeedsUpdate);

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
    bool isUpdating(bool valueChanged);

    DElement *m_target = nullptr;
    const MDiagram *m_diagram = nullptr;
    bool m_checkNeedsUpdate = false;
    bool m_isUpdateNeeded = false;
};

} // namespace qmt
