// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/diagram/dvisitor.h"
#include "qmt/diagram/dconstvisitor.h"
#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class QMT_EXPORT DVoidVisitor : public DVisitor
{
public:
    DVoidVisitor();

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
};

class QMT_EXPORT DConstVoidVisitor : public DConstVisitor
{
public:
    DConstVoidVisitor();

    void visitDElement(const DElement *element) override;
    void visitDObject(const DObject *object) override;
    void visitDPackage(const DPackage *package) override;
    void visitDClass(const DClass *klass) override;
    void visitDComponent(const DComponent *component) override;
    void visitDDiagram(const DDiagram *diagram) override;
    void visitDItem(const DItem *item) override;
    void visitDRelation(const DRelation *relation) override;
    void visitDInheritance(const DInheritance *inheritance) override;
    void visitDDependency(const DDependency *dependency) override;
    void visitDAssociation(const DAssociation *association) override;
    void visitDConnection(const DConnection *connection) override;
    void visitDAnnotation(const DAnnotation *annotation) override;
    void visitDBoundary(const DBoundary *boundary) override;
    void visitDSwimlane(const DSwimlane *swimlane) override;
};

} // namespace qmt
