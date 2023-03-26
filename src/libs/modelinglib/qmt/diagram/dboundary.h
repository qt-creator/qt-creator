// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "delement.h"

#include <QPointF>
#include <QRectF>

namespace qmt {

class QMT_EXPORT DBoundary : public DElement
{
public:
    DBoundary();
    DBoundary(const DBoundary &rhs);
    ~DBoundary() override;

    DBoundary &operator=(const DBoundary &rhs);

    Uid modelUid() const override { return Uid::invalidUid(); }
    QString text() const { return m_text; }
    void setText(const QString &text);
    QPointF pos() const { return m_pos; }
    void setPos(const QPointF &pos);
    QRectF rect() const { return m_rect; }
    void setRect(const QRectF &rect);

    void accept(DVisitor *visitor) override;
    void accept(DConstVisitor *visitor) const override;

private:
    QString m_text;
    QPointF m_pos;
    QRectF m_rect;
};

} // namespace qmt
