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

#include "stereotypedisplayvisitor.h"

#include "qmt/stereotype/stereotypecontroller.h"
#include "qmt/diagram/dpackage.h"
#include "qmt/diagram/dclass.h"
#include "qmt/diagram/dcomponent.h"
#include "qmt/diagram/ddiagram.h"
#include "qmt/diagram/ditem.h"

#include "qmt/model_controller/modelcontroller.h"
#include "qmt/model/mclass.h"

namespace qmt {

StereotypeDisplayVisitor::StereotypeDisplayVisitor()
    : DConstVoidVisitor(),
      _model_controller(0),
      _stereotype_controller(0),
      _stereotype_display(DObject::STEREOTYPE_NONE),
      _stereotype_icon_element(StereotypeIcon::ELEMENT_ANY),
      _stereotype_smart_display(DObject::STEREOTYPE_DECORATION)
{
}

StereotypeDisplayVisitor::~StereotypeDisplayVisitor()
{

}

void StereotypeDisplayVisitor::setModelController(ModelController *model_controller)
{
    _model_controller = model_controller;
}

void StereotypeDisplayVisitor::setStereotypeController(StereotypeController *stereotype_controller)
{
    _stereotype_controller = stereotype_controller;
}

StereotypeIcon::Display StereotypeDisplayVisitor::getStereotypeIconDisplay() const
{
    switch (_stereotype_display) {
    case DObject::STEREOTYPE_NONE:
        return StereotypeIcon::DISPLAY_NONE;
    case DObject::STEREOTYPE_LABEL:
        return StereotypeIcon::DISPLAY_LABEL;
    case DObject::STEREOTYPE_DECORATION:
        return StereotypeIcon::DISPLAY_DECORATION;
    case DObject::STEREOTYPE_ICON:
        return StereotypeIcon::DISPLAY_ICON;
    case DObject::STEREOTYPE_SMART:
        QMT_CHECK(false);
        return StereotypeIcon::DISPLAY_SMART;
    }
    return StereotypeIcon::DISPLAY_LABEL;
}

void StereotypeDisplayVisitor::visitDObject(const DObject *object)
{
    DObject::StereotypeDisplay stereotype_display = object->getStereotypeDisplay();
    _stereotype_icon_id = _stereotype_controller->findStereotypeIconId(_stereotype_icon_element, object->getStereotypes());

    if (_stereotype_icon_id.isEmpty() && stereotype_display == DObject::STEREOTYPE_ICON) {
        stereotype_display = DObject::STEREOTYPE_LABEL;
    } else if (!_stereotype_icon_id.isEmpty() && stereotype_display == DObject::STEREOTYPE_SMART) {
        StereotypeIcon stereotype_icon = _stereotype_controller->findStereotypeIcon(_stereotype_icon_id);
        StereotypeIcon::Display icon_display = stereotype_icon.getDisplay();
        switch (icon_display) {
        case StereotypeIcon::DISPLAY_NONE:
            stereotype_display = DObject::STEREOTYPE_NONE;
            break;
        case StereotypeIcon::DISPLAY_LABEL:
            stereotype_display = DObject::STEREOTYPE_LABEL;
            break;
        case StereotypeIcon::DISPLAY_DECORATION:
            stereotype_display = DObject::STEREOTYPE_DECORATION;
            break;
        case StereotypeIcon::DISPLAY_ICON:
            stereotype_display = DObject::STEREOTYPE_ICON;
            break;
        case StereotypeIcon::DISPLAY_SMART:
            stereotype_display = _stereotype_smart_display;
            break;
        }
    }
    if (stereotype_display == DObject::STEREOTYPE_SMART) {
        stereotype_display = DObject::STEREOTYPE_LABEL;
    }
    if (stereotype_display == DObject::STEREOTYPE_ICON && _shape_icon_id.isEmpty()) {
        _shape_icon_id = _stereotype_icon_id;
    }
    _stereotype_display = stereotype_display;
}

void StereotypeDisplayVisitor::visitDPackage(const DPackage *package)
{
    _stereotype_icon_element = StereotypeIcon::ELEMENT_PACKAGE;
    _stereotype_smart_display = DObject::STEREOTYPE_DECORATION;
    visitDObject(package);
}

void StereotypeDisplayVisitor::visitDClass(const DClass *klass)
{
    _stereotype_icon_element = StereotypeIcon::ELEMENT_CLASS;
    MClass *model_klass = _model_controller->findObject<MClass>(klass->getModelUid());
    bool has_members = false;
    if (!model_klass->getMembers().isEmpty() && klass->getShowAllMembers()) {
        has_members = true;
    }
    _stereotype_smart_display = has_members ? DObject::STEREOTYPE_DECORATION : DObject::STEREOTYPE_ICON;
    visitDObject(klass);
}

void StereotypeDisplayVisitor::visitDComponent(const DComponent *component)
{
    _stereotype_icon_element = StereotypeIcon::ELEMENT_COMPONENT;
    _stereotype_smart_display = DObject::STEREOTYPE_ICON;
    visitDObject(component);
}

void StereotypeDisplayVisitor::visitDDiagram(const DDiagram *diagram)
{
    _stereotype_icon_element = StereotypeIcon::ELEMENT_DIAGRAM;
    _stereotype_smart_display = DObject::STEREOTYPE_DECORATION;
    visitDObject(diagram);
}

void StereotypeDisplayVisitor::visitDItem(const DItem *item)
{
    _stereotype_icon_element = StereotypeIcon::ELEMENT_ITEM;
    _stereotype_smart_display = DObject::STEREOTYPE_ICON;
    visitDObject(item);
    if (_stereotype_icon_id.isEmpty() && !item->getShape().isEmpty()) {
        _shape_icon_id = _stereotype_controller->findStereotypeIconId(StereotypeIcon::ELEMENT_ITEM, QStringList() << item->getShape());
    }
    if (_shape_icon_id.isEmpty() && !item->getVariety().isEmpty()) {
        _shape_icon_id = _stereotype_controller->findStereotypeIconId(StereotypeIcon::ELEMENT_ITEM, QStringList() << item->getVariety());
    }
}

} // namespace qmt
