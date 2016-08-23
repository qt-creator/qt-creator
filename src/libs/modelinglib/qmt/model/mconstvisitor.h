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
