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

#include "diagramscenemodel.h"

#include "qmt/diagram/dvisitor.h"

namespace qmt {

class QMT_EXPORT DiagramSceneModel::CreationVisitor : public DVisitor
{
public:
    explicit CreationVisitor(DiagramSceneModel *diagramSceneModel);

    QGraphicsItem *createdGraphicsItem() const { return m_graphicsItem; }

    void visitDElement(DElement *element) override;
    void visitDObject(DObject *object) override;
    void visitDPackage(DPackage *package) override;
    void visitDClass(DClass *klass) override;
    void visitDComponent(DComponent *component) override;
    void visitDDiagram(DDiagram *diagram) override;
    void visitDItem(DItem *item) override;
    void visitDRelation(DRelation *relation) override;
    void visitDInheritance(DInheritance *inheritance) override;
    void visitDDependency(DDependency *dependency) override;
    void visitDAssociation(DAssociation *association) override;
    void visitDConnection(DConnection *connection) override;
    void visitDAnnotation(DAnnotation *annotation) override;
    void visitDBoundary(DBoundary *boundary) override;
    void visitDSwimlane(DSwimlane *swimlane) override;

private:
    DiagramSceneModel *m_diagramSceneModel = nullptr;
    QGraphicsItem *m_graphicsItem = nullptr;
};

class QMT_EXPORT DiagramSceneModel::UpdateVisitor : public DVisitor
{
public:
    UpdateVisitor(QGraphicsItem *item, DiagramSceneModel *diagramSceneModel,
                  DElement *relatedElement = nullptr);

    void visitDElement(DElement *element) override;
    void visitDObject(DObject *object) override;
    void visitDPackage(DPackage *package) override;
    void visitDClass(DClass *klass) override;
    void visitDComponent(DComponent *component) override;
    void visitDDiagram(DDiagram *diagram) override;
    void visitDItem(DItem *item) override;
    void visitDRelation(DRelation *relation) override;
    void visitDInheritance(DInheritance *inheritance) override;
    void visitDDependency(DDependency *dependency) override;
    void visitDAssociation(DAssociation *association) override;
    void visitDConnection(DConnection *connection) override;
    void visitDAnnotation(DAnnotation *annotation) override;
    void visitDBoundary(DBoundary *boundary) override;
    void visitDSwimlane(DSwimlane *swimlane) override;

private:
    QGraphicsItem *m_graphicsItem = nullptr;
    DiagramSceneModel *m_diagramSceneModel = nullptr;
    DElement *m_relatedElement = nullptr;
};

} // namespace qmt
