/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMT_DVISITOR_H
#define QMT_DVISITOR_H

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
class DAnnotation;
class DBoundary;

class DVisitor
{
public:
    virtual ~DVisitor() { }

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
    virtual void visitDAnnotation(DAnnotation *annotation) = 0;
    virtual void visitDBoundary(DBoundary *boundary) = 0;
};

} // namespace qmt

#endif // QMT_DVISITOR_H
