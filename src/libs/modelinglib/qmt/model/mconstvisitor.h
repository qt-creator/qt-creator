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

class MConstVisitor
{
public:
    virtual ~MConstVisitor() { }

    virtual void visitMElement(const MElement *element) = 0;
    virtual void visitMObject(const MObject *object) = 0;
    virtual void visitMPackage(const MPackage *package) = 0;
    virtual void visitMClass(const MClass *klass) = 0;
    virtual void visitMComponent(const MComponent *component) = 0;
    virtual void visitMDiagram(const MDiagram *diagram) = 0;
    virtual void visitMCanvasDiagram(const MCanvasDiagram *diagram) = 0;
    virtual void visitMItem(const MItem *item) = 0;
    virtual void visitMRelation(const MRelation *relation) = 0;
    virtual void visitMDependency(const MDependency *dependency) = 0;
    virtual void visitMInheritance(const MInheritance *inheritance) = 0;
    virtual void visitMAssociation(const MAssociation *association) = 0;
    virtual void visitMConnection(const MConnection *connection) = 0;
};

} // namespace qmt
