// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace qmt {

class MElement;

class MObject;
class MPackage;
class MClass;
class MComponent;
class MDiagram;
class MCanvasDiagram;
class MItem;

class MRelation;
class MDependency;
class MInheritance;
class MAssociation;
class MConnection;

class MVisitor
{
public:
    virtual ~MVisitor() { }

    virtual void visitMElement(MElement *element) = 0;
    virtual void visitMObject(MObject *object) = 0;
    virtual void visitMPackage(MPackage *package) = 0;
    virtual void visitMClass(MClass *klass) = 0;
    virtual void visitMComponent(MComponent *component) = 0;
    virtual void visitMDiagram(MDiagram *diagram) = 0;
    virtual void visitMCanvasDiagram(MCanvasDiagram *diagram) = 0;
    virtual void visitMItem(MItem *item) = 0;
    virtual void visitMRelation(MRelation *relation) = 0;
    virtual void visitMDependency(MDependency *dependency) = 0;
    virtual void visitMInheritance(MInheritance *inheritance) = 0;
    virtual void visitMAssociation(MAssociation *association) = 0;
    virtual void visitMConnection(MConnection *connection) = 0;
};

} // namespace qmt
