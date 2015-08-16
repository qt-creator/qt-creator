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

#ifndef QMT_MCONSTVISITOR_H
#define QMT_MCONSTVISITOR_H

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

};

}

#endif // QMT_MCONSTVISITOR_H
