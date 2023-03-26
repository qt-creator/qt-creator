// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dobject.h"

namespace qmt {

class QMT_EXPORT DItem : public DObject
{
public:
    DItem();
    ~DItem() override;

    QString variety() const { return m_variety; }
    void setVariety(const QString &variety);
    QString shape() const { return m_shape; }
    void setShape(const QString &shape);
    bool isShapeEditable() const { return m_isShapeEditable; }
    void setShapeEditable(bool shapeEditable);

    void accept(DVisitor *visitor) override;
    void accept(DConstVisitor *visitor) const override;

private:
    QString m_variety;
    QString m_shape;
    bool m_isShapeEditable = true;
};

} // namespace qmt
