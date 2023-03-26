// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
