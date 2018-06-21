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
