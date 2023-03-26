// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
