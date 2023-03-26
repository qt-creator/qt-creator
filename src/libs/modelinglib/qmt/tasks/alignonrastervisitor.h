// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/diagram/dvisitor.h"
#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class DiagramController;
class ISceneInspector;
class MDiagram;

class QMT_EXPORT AlignOnRasterVisitor : public DVisitor
{
public:
    AlignOnRasterVisitor();
    ~AlignOnRasterVisitor() override;

    void setDiagramController(DiagramController *diagramController);
    void setSceneInspector(ISceneInspector *sceneInspector);
    void setDiagram(MDiagram *diagram);

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
    DiagramController *m_diagramController = nullptr;
    ISceneInspector *m_sceneInspector = nullptr;
    MDiagram *m_diagram = nullptr;
};

} // namespace qmt
