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
      m_modelController(0),
      m_stereotypeController(0),
      m_stereotypeDisplay(DObject::StereotypeNone),
      m_stereotypeIconElement(StereotypeIcon::ElementAny),
      m_stereotypeSmartDisplay(DObject::StereotypeDecoration)
{
}

StereotypeDisplayVisitor::~StereotypeDisplayVisitor()
{
}

void StereotypeDisplayVisitor::setModelController(ModelController *modelController)
{
    m_modelController = modelController;
}

void StereotypeDisplayVisitor::setStereotypeController(StereotypeController *stereotypeController)
{
    m_stereotypeController = stereotypeController;
}

StereotypeIcon::Display StereotypeDisplayVisitor::stereotypeIconDisplay() const
{
    switch (m_stereotypeDisplay) {
    case DObject::StereotypeNone:
        return StereotypeIcon::DisplayNone;
    case DObject::StereotypeLabel:
        return StereotypeIcon::DisplayLabel;
    case DObject::StereotypeDecoration:
        return StereotypeIcon::DisplayDecoration;
    case DObject::StereotypeIcon:
        return StereotypeIcon::DisplayIcon;
    case DObject::StereotypeSmart:
        QMT_CHECK(false);
        return StereotypeIcon::DisplaySmart;
    }
    return StereotypeIcon::DisplayLabel;
}

void StereotypeDisplayVisitor::visitDObject(const DObject *object)
{
    DObject::StereotypeDisplay stereotypeDisplay = object->stereotypeDisplay();
    m_stereotypeIconId = m_stereotypeController->findStereotypeIconId(m_stereotypeIconElement, object->stereotypes());

    if (m_stereotypeIconId.isEmpty() && stereotypeDisplay == DObject::StereotypeIcon) {
        stereotypeDisplay = DObject::StereotypeLabel;
    } else if (!m_stereotypeIconId.isEmpty() && stereotypeDisplay == DObject::StereotypeSmart) {
        StereotypeIcon stereotypeIcon = m_stereotypeController->findStereotypeIcon(m_stereotypeIconId);
        StereotypeIcon::Display iconDisplay = stereotypeIcon.display();
        switch (iconDisplay) {
        case StereotypeIcon::DisplayNone:
            stereotypeDisplay = DObject::StereotypeNone;
            break;
        case StereotypeIcon::DisplayLabel:
            stereotypeDisplay = DObject::StereotypeLabel;
            break;
        case StereotypeIcon::DisplayDecoration:
            stereotypeDisplay = DObject::StereotypeDecoration;
            break;
        case StereotypeIcon::DisplayIcon:
            stereotypeDisplay = DObject::StereotypeIcon;
            break;
        case StereotypeIcon::DisplaySmart:
            stereotypeDisplay = m_stereotypeSmartDisplay;
            break;
        }
    }
    if (stereotypeDisplay == DObject::StereotypeSmart)
        stereotypeDisplay = DObject::StereotypeLabel;
    if (stereotypeDisplay == DObject::StereotypeIcon && m_shapeIconId.isEmpty())
        m_shapeIconId = m_stereotypeIconId;
    m_stereotypeDisplay = stereotypeDisplay;
}

void StereotypeDisplayVisitor::visitDPackage(const DPackage *package)
{
    m_stereotypeIconElement = StereotypeIcon::ElementPackage;
    m_stereotypeSmartDisplay = DObject::StereotypeDecoration;
    visitDObject(package);
}

void StereotypeDisplayVisitor::visitDClass(const DClass *klass)
{
    m_stereotypeIconElement = StereotypeIcon::ElementClass;
    MClass *modelKlass = m_modelController->findObject<MClass>(klass->modelUid());
    bool hasMembers = false;
    if (!modelKlass->members().isEmpty() && klass->showAllMembers())
        hasMembers = true;
    m_stereotypeSmartDisplay = hasMembers ? DObject::StereotypeDecoration : DObject::StereotypeIcon;
    visitDObject(klass);
}

void StereotypeDisplayVisitor::visitDComponent(const DComponent *component)
{
    m_stereotypeIconElement = StereotypeIcon::ElementComponent;
    m_stereotypeSmartDisplay = DObject::StereotypeIcon;
    visitDObject(component);
}

void StereotypeDisplayVisitor::visitDDiagram(const DDiagram *diagram)
{
    m_stereotypeIconElement = StereotypeIcon::ElementDiagram;
    m_stereotypeSmartDisplay = DObject::StereotypeDecoration;
    visitDObject(diagram);
}

void StereotypeDisplayVisitor::visitDItem(const DItem *item)
{
    m_stereotypeIconElement = StereotypeIcon::ElementItem;
    m_stereotypeSmartDisplay = DObject::StereotypeIcon;
    visitDObject(item);
    if (m_stereotypeIconId.isEmpty() && !item->shape().isEmpty())
        m_shapeIconId = m_stereotypeController->findStereotypeIconId(StereotypeIcon::ElementItem, QStringList() << item->shape());
    if (m_shapeIconId.isEmpty() && !item->variety().isEmpty())
        m_shapeIconId = m_stereotypeController->findStereotypeIconId(StereotypeIcon::ElementItem, QStringList() << item->variety());
}

} // namespace qmt
