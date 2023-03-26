// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace qmt {

class DElement;
class DObject;
class DPackage;
class DClass;
class DComponent;
class DDiagram;
class DItem;
class DRelation;
class DInheritance;
class DDependency;
class DAssociation;
class DConnection;
class DAnnotation;
class DBoundary;
class DSwimlane;

class DVisitor
{
public:
    virtual ~DVisitor() {}

    virtual void visitDElement(DElement *element) = 0;
    virtual void visitDObject(DObject *object) = 0;
    virtual void visitDPackage(DPackage *package) = 0;
    virtual void visitDClass(DClass *klass) = 0;
    virtual void visitDComponent(DComponent *component) = 0;
    virtual void visitDDiagram(DDiagram *diagram) = 0;
    virtual void visitDItem(DItem *item) = 0;
    virtual void visitDRelation(DRelation *relation) = 0;
    virtual void visitDInheritance(DInheritance *inheritance) = 0;
    virtual void visitDDependency(DDependency *dependency) = 0;
    virtual void visitDAssociation(DAssociation *association) = 0;
    virtual void visitDConnection(DConnection *connection) = 0;
    virtual void visitDAnnotation(DAnnotation *annotation) = 0;
    virtual void visitDBoundary(DBoundary *boundary) = 0;
    virtual void visitDSwimlane(DSwimlane *swimlane) = 0;
};

} // namespace qmt
