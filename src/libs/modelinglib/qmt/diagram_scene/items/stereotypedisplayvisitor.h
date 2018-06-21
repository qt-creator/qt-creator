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

#include "qmt/diagram_controller/dvoidvisitor.h"

#include "qmt/diagram/dobject.h"
#include "qmt/stereotype/stereotypeicon.h"

namespace qmt {

class ModelController;
class StereotypeController;

class StereotypeDisplayVisitor : public DConstVoidVisitor
{
public:
    StereotypeDisplayVisitor();
    ~StereotypeDisplayVisitor() override;

    void setModelController(ModelController *modelController);
    void setStereotypeController(StereotypeController *stereotypeController);
    StereotypeIcon::Display stereotypeIconDisplay() const;
    QString stereotypeIconId() const { return m_stereotypeIconId; }
    QString shapeIconId() const { return m_shapeIconId; }
    StereotypeIcon shapeIcon() const { return m_shapeIcon; }

    void visitDObject(const DObject *object) override;
    void visitDPackage(const DPackage *package) override;
    void visitDClass(const DClass *klass) override;
    void visitDComponent(const DComponent *component) override;
    void visitDDiagram(const DDiagram *diagram) override;
    void visitDItem(const DItem *item) override;

private:
    void updateShapeIcon();

    ModelController *m_modelController = nullptr;
    StereotypeController *m_stereotypeController = nullptr;
    DObject::StereotypeDisplay m_stereotypeDisplay = DObject::StereotypeNone;
    QString m_stereotypeIconId;
    QString m_shapeIconId;
    StereotypeIcon m_shapeIcon;
    StereotypeIcon::Element m_stereotypeIconElement = StereotypeIcon::ElementAny;
    DObject::StereotypeDisplay m_stereotypeSmartDisplay = DObject::StereotypeDecoration;
};

} // namespace qmt
