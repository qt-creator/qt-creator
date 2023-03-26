// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mobject.h"

namespace qmt {

class QMT_EXPORT MItem : public MObject
{
public:
    MItem();
    ~MItem() override;

    QString variety() const { return m_variety; }
    void setVariety(const QString &variety);
    bool isVarietyEditable() const { return m_isVarietyEditable; }
    void setVarietyEditable(bool varietyEditable);
    bool isShapeEditable() const { return m_isShapeEditable; }
    void setShapeEditable(bool shapeEditable);

    void accept(MVisitor *visitor) override;
    void accept(MConstVisitor *visitor) const override;

private:
    QString m_variety;
    bool m_isVarietyEditable = true;
    bool m_isShapeEditable = false;
};

} // namespace qmt
