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

#include "alignonrastervisitor.h"

#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram/dannotation.h"
#include "qmt/diagram/dassociation.h"
#include "qmt/diagram/dboundary.h"
#include "qmt/diagram/dclass.h"
#include "qmt/diagram/dcomponent.h"
#include "qmt/diagram/ddependency.h"
#include "qmt/diagram/ddiagram.h"
#include "qmt/diagram/ditem.h"
#include "qmt/diagram/delement.h"
#include "qmt/diagram/dinheritance.h"
#include "qmt/diagram/dobject.h"
#include "qmt/diagram/dpackage.h"
#include "qmt/diagram/drelation.h"
#include "qmt/diagram_scene/capabilities/moveable.h"
#include "qmt/diagram_scene/capabilities/resizable.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/tasks/isceneinspector.h"


namespace qmt {

AlignOnRasterVisitor::AlignOnRasterVisitor()
    : m_diagramController(0),
      m_sceneInspector(0),
      m_diagram(0)
{
}

AlignOnRasterVisitor::~AlignOnRasterVisitor()
{
}

void AlignOnRasterVisitor::setDiagramController(DiagramController *diagramController)
{
    m_diagramController = diagramController;
}

void AlignOnRasterVisitor::setSceneInspector(ISceneInspector *sceneInspector)
{
    m_sceneInspector = sceneInspector;
}

void AlignOnRasterVisitor::setDiagram(MDiagram *diagram)
{
    m_diagram = diagram;
}

void AlignOnRasterVisitor::visitDElement(DElement *element)
{
    Q_UNUSED(element);

    QMT_CHECK(false);
}

void AlignOnRasterVisitor::visitDObject(DObject *object)
{
    IResizable *resizable = m_sceneInspector->getResizable(object, m_diagram);
    if (resizable) {
        resizable->alignItemSizeToRaster(IResizable::SIDE_RIGHT_OR_BOTTOM, IResizable::SIDE_RIGHT_OR_BOTTOM, 2 * RASTER_WIDTH, 2 * RASTER_HEIGHT);
    }
    IMoveable *moveable = m_sceneInspector->getMoveable(object, m_diagram);
    if (moveable) {
        moveable->alignItemPositionToRaster(RASTER_WIDTH, RASTER_HEIGHT);
    }
}

void AlignOnRasterVisitor::visitDPackage(DPackage *package)
{
    visitDObject(package);
}

void AlignOnRasterVisitor::visitDClass(DClass *klass)
{
    visitDObject(klass);
}

void AlignOnRasterVisitor::visitDComponent(DComponent *component)
{
    visitDObject(component);
}

void AlignOnRasterVisitor::visitDDiagram(DDiagram *diagram)
{
    visitDObject(diagram);
}

void AlignOnRasterVisitor::visitDItem(DItem *item)
{
    visitDObject(item);
}

void AlignOnRasterVisitor::visitDRelation(DRelation *relation)
{
    Q_UNUSED(relation);
}

void AlignOnRasterVisitor::visitDInheritance(DInheritance *inheritance)
{
    visitDRelation(inheritance);
}

void AlignOnRasterVisitor::visitDDependency(DDependency *dependency)
{
    visitDRelation(dependency);
}

void AlignOnRasterVisitor::visitDAssociation(DAssociation *association)
{
    visitDRelation(association);
}

void AlignOnRasterVisitor::visitDAnnotation(DAnnotation *annotation)
{
    IMoveable *moveable = m_sceneInspector->getMoveable(annotation, m_diagram);
    if (moveable) {
        moveable->alignItemPositionToRaster(RASTER_WIDTH, RASTER_HEIGHT);
    }
}

void AlignOnRasterVisitor::visitDBoundary(DBoundary *boundary)
{
    IResizable *resizable = m_sceneInspector->getResizable(boundary, m_diagram);
    if (resizable) {
        resizable->alignItemSizeToRaster(IResizable::SIDE_RIGHT_OR_BOTTOM, IResizable::SIDE_RIGHT_OR_BOTTOM, 2 * RASTER_WIDTH, 2 * RASTER_HEIGHT);
    }
    IMoveable *moveable = m_sceneInspector->getMoveable(boundary, m_diagram);
    if (moveable) {
        moveable->alignItemPositionToRaster(RASTER_WIDTH, RASTER_HEIGHT);
    }
}

}
