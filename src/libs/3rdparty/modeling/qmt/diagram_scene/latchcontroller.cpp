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

#include "latchcontroller.h"

#include "diagramsceneconstants.h"
#include "diagramscenemodel.h"
#include "parts/alignlineitem.h"

#include "qmt/diagram/dobject.h"
#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/infrastructure/qmtassert.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>

namespace qmt {

LatchController::LatchController(QObject *parent)
    : QObject(parent),
      _diagram_scene_model(0),
      _horizontal_align_line(new AlignLineItem(AlignLineItem::HORIZONTAL, 0)),
      _vertical_align_line(new AlignLineItem(AlignLineItem::VERTICAL, 0)),
      _found_horizontal_latch(false),
      _horizontal_dist(0.0),
      _found_vertical_latch(false),
      _vertical_dist(0.0)
{
    _horizontal_align_line->setZValue(LATCH_LINES_ZVALUE);
    _horizontal_align_line->setVisible(false);

    _vertical_align_line->setZValue(LATCH_LINES_ZVALUE);
    _vertical_align_line->setVisible(false);
}

LatchController::~LatchController()
{
    if (_vertical_align_line->scene()) {
        _vertical_align_line->scene()->removeItem(_vertical_align_line);
    }
    delete _vertical_align_line;
    if (_horizontal_align_line->scene()) {
        _horizontal_align_line->scene()->removeItem(_horizontal_align_line);
    }
    delete _horizontal_align_line;
}

void LatchController::setDiagramSceneModel(DiagramSceneModel *diagram_scene_model)
{
    _diagram_scene_model = diagram_scene_model;
}

void LatchController::addToGraphicsScene(QGraphicsScene *graphics_scene)
{
    QMT_CHECK(graphics_scene);
    graphics_scene->addItem(_horizontal_align_line);
    graphics_scene->addItem(_vertical_align_line);
}

void LatchController::removeFromGraphicsScene(QGraphicsScene *graphics_scene)
{
    Q_UNUSED(graphics_scene); // avoid warning in release mode

    if (_vertical_align_line->scene()) {
        QMT_CHECK(graphics_scene == _vertical_align_line->scene());
        _vertical_align_line->scene()->removeItem(_vertical_align_line);
    }
    if (_horizontal_align_line->scene()) {
        QMT_CHECK(graphics_scene == _horizontal_align_line->scene());
        _horizontal_align_line->scene()->removeItem(_horizontal_align_line);
    }
}

void LatchController::mousePressEventLatching(QGraphicsSceneMouseEvent *event)
{
    // TODO add keyboard event handler to handle latching also on modified change without move
    mouseMoveEventLatching(event);
}

void LatchController::mouseMoveEventLatching(QGraphicsSceneMouseEvent *event)
{
    _found_horizontal_latch = false;
    _found_vertical_latch = false;

    if (!(event->modifiers() & Qt::ShiftModifier)) {
        _horizontal_align_line->setVisible(false);
        _vertical_align_line->setVisible(false);
        return;
    }

    if (!_diagram_scene_model->getFocusItem()) {
        return;
    }

    ILatchable *palped_latchable = dynamic_cast<ILatchable *>(_diagram_scene_model->getFocusItem());
    if (!palped_latchable) {
        return;
    }

    ILatchable::Action horiz_action = ILatchable::MOVE;
    ILatchable::Action vert_action = ILatchable::MOVE;

    QList<ILatchable::Latch> palped_horizontals = palped_latchable->getHorizontalLatches(horiz_action, true);
    QList<ILatchable::Latch> palped_verticals = palped_latchable->getVerticalLatches(vert_action, true);

    qreal horiz_min_dist = 10.0;
    ILatchable::Latch best_horiz_latch;
    bool found_best_horiz = false;
    qreal vert_min_dist = 10.0;
    ILatchable::Latch best_vert_latch;
    bool found_best_vert = false;

    foreach (QGraphicsItem *item, _diagram_scene_model->getGraphicsItems()) {
        if (item != _diagram_scene_model->getFocusItem() && !_diagram_scene_model->isSelectedItem(item)) {
            if (ILatchable *latchable = dynamic_cast<ILatchable *>(item)) {
                QList<ILatchable::Latch> horizontals = latchable->getHorizontalLatches(horiz_action, false);
                foreach (const ILatchable::Latch &palped_latch, palped_horizontals) {
                    foreach (const ILatchable::Latch &latch, horizontals) {
                        if (palped_latch._latch_type == latch._latch_type) {
                            // calculate distance and minimal distance with sign because this is needed later to move the objects
                            qreal dist = latch._pos - palped_latch._pos;
                            if (qAbs(dist) < qAbs(horiz_min_dist)) {
                                horiz_min_dist = dist;
                                best_horiz_latch = latch;
                                found_best_horiz = true;
                            }
                        }
                    }
                }
                QList<ILatchable::Latch> verticals = latchable->getVerticalLatches(vert_action, false);
                foreach (const ILatchable::Latch &palped_latch, palped_verticals) {
                    foreach (const ILatchable::Latch &latch, verticals) {
                        if (palped_latch._latch_type == latch._latch_type) {
                            // calculate distance and minimal distance with sign because this is needed later to move the objects
                            qreal dist = latch._pos - palped_latch._pos;
                            if (qAbs(dist) < qAbs(vert_min_dist)) {
                                vert_min_dist = dist;
                                best_vert_latch = latch;
                                found_best_vert = true;
                            }
                        }
                    }
                }
            }
        }
    }

    if (found_best_horiz) {
        switch (best_horiz_latch._latch_type) {
        case ILatchable::LEFT:
        case ILatchable::RIGHT:
        case ILatchable::HCENTER:
            _vertical_align_line->setLine(best_horiz_latch._pos, best_horiz_latch._other_pos1, best_horiz_latch._other_pos2);
            _vertical_align_line->setVisible(true);
            _found_horizontal_latch = true;
            _horizontal_latch = best_horiz_latch;
            _horizontal_dist = horiz_min_dist;
            break;
        case ILatchable::NONE:
        case ILatchable::TOP:
        case ILatchable::BOTTOM:
        case ILatchable::VCENTER:
            QMT_CHECK(false);
            break;
        }
    } else {
        _vertical_align_line->setVisible(false);
    }

    if (found_best_vert) {
        switch (best_vert_latch._latch_type) {
        case ILatchable::TOP:
        case ILatchable::BOTTOM:
        case ILatchable::VCENTER:
            _horizontal_align_line->setLine(best_vert_latch._pos, best_vert_latch._other_pos1, best_vert_latch._other_pos2);
            _horizontal_align_line->setVisible(true);
            _found_vertical_latch = true;
            _vertical_latch = best_vert_latch;
            _vertical_dist = vert_min_dist;
            break;
        case ILatchable::NONE:
        case ILatchable::LEFT:
        case ILatchable::RIGHT:
        case ILatchable::HCENTER:
            QMT_CHECK(false);
            break;
        }
    } else {
        _horizontal_align_line->setVisible(false);
    }
}

void LatchController::mouseReleaseEventLatching(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);

    if (_found_horizontal_latch) {
        switch (_horizontal_latch._latch_type) {
        case ILatchable::LEFT:
        case ILatchable::RIGHT:
        case ILatchable::HCENTER:
            foreach (QGraphicsItem *item, _diagram_scene_model->getSelectedItems()) {
                DElement *element = _diagram_scene_model->getElement(item);
                if (DObject *selected_object = dynamic_cast<DObject *>(element)) {
                    _diagram_scene_model->getDiagramController()->startUpdateElement(selected_object, _diagram_scene_model->getDiagram(), DiagramController::UPDATE_GEOMETRY);
                    QPointF new_pos = selected_object->getPos();
                    new_pos.setX(new_pos.x() + _horizontal_dist);
                    selected_object->setPos(new_pos);
                    _diagram_scene_model->getDiagramController()->finishUpdateElement(selected_object, _diagram_scene_model->getDiagram(), false);
                }
            }
            break;
        case ILatchable::NONE:
        case ILatchable::TOP:
        case ILatchable::BOTTOM:
        case ILatchable::VCENTER:
            QMT_CHECK(false);
            break;
        }
    }

    if (_found_vertical_latch) {
        switch (_vertical_latch._latch_type) {
        case ILatchable::TOP:
        case ILatchable::BOTTOM:
        case ILatchable::VCENTER:
            foreach (QGraphicsItem *item, _diagram_scene_model->getSelectedItems()) {
                DElement *element = _diagram_scene_model->getElement(item);
                if (DObject *selected_object = dynamic_cast<DObject *>(element)) {
                    _diagram_scene_model->getDiagramController()->startUpdateElement(selected_object, _diagram_scene_model->getDiagram(), DiagramController::UPDATE_GEOMETRY);
                    QPointF new_pos = selected_object->getPos();
                    new_pos.setY(new_pos.y() + _vertical_dist);
                    selected_object->setPos(new_pos);
                    _diagram_scene_model->getDiagramController()->finishUpdateElement(selected_object, _diagram_scene_model->getDiagram(), false);
                }
            }
            break;
        case ILatchable::NONE:
        case ILatchable::LEFT:
        case ILatchable::RIGHT:
        case ILatchable::HCENTER:
            QMT_CHECK(false);
            break;
        }

    }

    _horizontal_align_line->setVisible(false);
    _vertical_align_line->setVisible(false);
}

}
