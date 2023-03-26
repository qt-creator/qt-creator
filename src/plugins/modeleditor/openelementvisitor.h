// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/diagram/dconstvisitor.h"
#include "qmt/model/mconstvisitor.h"

namespace qmt { class ModelController; }

namespace ModelEditor {
namespace Internal {

class ElementTasks;

class OpenDiagramElementVisitor :
        public qmt::DConstVisitor
{
public:
    void setModelController(qmt::ModelController *modelController);
    void setElementTasks(ElementTasks *elementTasks);

    void visitDElement(const qmt::DElement *element) override;
    void visitDObject(const qmt::DObject *object) override;
    void visitDPackage(const qmt::DPackage *package) override;
    void visitDClass(const qmt::DClass *klass) override;
    void visitDComponent(const qmt::DComponent *component) override;
    void visitDDiagram(const qmt::DDiagram *diagram) override;
    void visitDItem(const qmt::DItem *item) override;
    void visitDRelation(const qmt::DRelation *relation) override;
    void visitDInheritance(const qmt::DInheritance *inheritance) override;
    void visitDDependency(const qmt::DDependency *dependency) override;
    void visitDAssociation(const qmt::DAssociation *association) override;
    void visitDConnection(const qmt::DConnection *connection) override;
    void visitDAnnotation(const qmt::DAnnotation *annotation) override;
    void visitDBoundary(const qmt::DBoundary *boundary) override;
    void visitDSwimlane(const qmt::DSwimlane *swimlane) override;

private:
    qmt::ModelController *m_modelController = nullptr;
    ElementTasks *m_elementTasks = nullptr;
};

class OpenModelElementVisitor :
        public qmt::MConstVisitor
{
public:
    void setElementTasks(ElementTasks *elementTasks);

    void visitMElement(const qmt::MElement *element) override;
    void visitMObject(const qmt::MObject *object) override;
    void visitMPackage(const qmt::MPackage *package) override;
    void visitMClass(const qmt::MClass *klass) override;
    void visitMComponent(const qmt::MComponent *component) override;
    void visitMDiagram(const qmt::MDiagram *diagram) override;
    void visitMCanvasDiagram(const qmt::MCanvasDiagram *diagram) override;
    void visitMItem(const qmt::MItem *item) override;
    void visitMRelation(const qmt::MRelation *relation) override;
    void visitMDependency(const qmt::MDependency *dependency) override;
    void visitMInheritance(const qmt::MInheritance *inheritance) override;
    void visitMAssociation(const qmt::MAssociation *association) override;
    void visitMConnection(const qmt::MConnection *connection) override;

private:
    ElementTasks *m_elementTasks = nullptr;
};

} // namespace Internal
} // namespace ModelEditor
