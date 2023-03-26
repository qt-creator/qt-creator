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

class DConstVisitor
{
public:
    virtual ~DConstVisitor() {}

    virtual void visitDElement(const DElement *element) = 0;
    virtual void visitDObject(const DObject *object) = 0;
    virtual void visitDPackage(const DPackage *package) = 0;
    virtual void visitDClass(const DClass *klass) = 0;
    virtual void visitDComponent(const DComponent *component) = 0;
    virtual void visitDDiagram(const DDiagram *diagram) = 0;
    virtual void visitDItem(const DItem *item) = 0;
    virtual void visitDRelation(const DRelation *relation) = 0;
    virtual void visitDInheritance(const DInheritance *inheritance) = 0;
    virtual void visitDDependency(const DDependency *dependency) = 0;
    virtual void visitDAssociation(const DAssociation *association) = 0;
    virtual void visitDConnection(const DConnection *connection) = 0;
    virtual void visitDAnnotation(const DAnnotation *annotation) = 0;
    virtual void visitDBoundary(const DBoundary *boundary) = 0;
    virtual void visitDSwimlane(const DSwimlane *swimlane) = 0;
};

} // namespace qmt
