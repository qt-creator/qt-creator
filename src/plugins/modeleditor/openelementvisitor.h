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
