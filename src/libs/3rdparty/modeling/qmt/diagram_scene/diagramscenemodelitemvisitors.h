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

#ifndef QMT_DIAGRAMSCENEMODELITEMVISITORS_H
#define QMT_DIAGRAMSCENEMODELITEMVISITORS_H

#include "diagramscenemodel.h"

#include "qmt/diagram/dvisitor.h"

namespace qmt {

class QMT_EXPORT DiagramSceneModel::CreationVisitor : public DVisitor
{
public:
    CreationVisitor(DiagramSceneModel *diagramSceneModel);

    QGraphicsItem *createdGraphicsItem() const { return m_graphicsItem; }

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
    void visitDAnnotation(DAnnotation *annotation) override;
    void visitDBoundary(DBoundary *boundary) override;

private:
    DiagramSceneModel *m_diagramSceneModel;
    QGraphicsItem *m_graphicsItem;
};

class DiagramSceneModel::UpdateVisitor : public DVisitor
{
public:
    UpdateVisitor(QGraphicsItem *item, DiagramSceneModel *diagramSceneModel,
                  DElement *relatedElement = 0);

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
    void visitDAnnotation(DAnnotation *annotation) override;
    void visitDBoundary(DBoundary *boundary) override;

private:
    QGraphicsItem *m_graphicsItem;
    DiagramSceneModel *m_diagramSceneModel;
    DElement *m_relatedElement;
};

} // namespace qmt

#endif // QMT_DIAGRAMSCENEMODELITEMVISITORS_H
