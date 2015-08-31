/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef OPENELEMENTVISITOR_H
#define OPENELEMENTVISITOR_H

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

    void visitDElement(const qmt::DElement *element);
    void visitDObject(const qmt::DObject *object);
    void visitDPackage(const qmt::DPackage *package);
    void visitDClass(const qmt::DClass *klass);
    void visitDComponent(const qmt::DComponent *component);
    void visitDDiagram(const qmt::DDiagram *diagram);
    void visitDItem(const qmt::DItem *item);
    void visitDRelation(const qmt::DRelation *relation);
    void visitDInheritance(const qmt::DInheritance *inheritance);
    void visitDDependency(const qmt::DDependency *dependency);
    void visitDAssociation(const qmt::DAssociation *association);
    void visitDAnnotation(const qmt::DAnnotation *annotation);
    void visitDBoundary(const qmt::DBoundary *boundary);

private:
    qmt::ModelController *m_modelController = 0;
    ElementTasks *m_elementTasks = 0;
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

private:
    ElementTasks *m_elementTasks = 0;
};

} // namespace Internal
} // namespace ModelEditor

#endif // OPENELEMENTVISITOR_H
