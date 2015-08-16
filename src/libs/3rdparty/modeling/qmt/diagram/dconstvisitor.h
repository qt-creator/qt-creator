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

#ifndef QMT_DCONSTVISITOR_H
#define QMT_DCONSTVISITOR_H

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


class DConstVisitor
{
public:
    virtual ~DConstVisitor() { }

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

    virtual void visitDAnnotation(const DAnnotation *annotation) = 0;

    virtual void visitDBoundary(const DBoundary *boundary) = 0;
};

}

#endif // QMT_DCONSTVISITOR_H
