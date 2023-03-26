// Copyright (C) 2017 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "delement.h"

namespace qmt {

class QMT_EXPORT DSwimlane : public DElement
{
public:
    DSwimlane();
    DSwimlane(const DSwimlane &rhs);
    ~DSwimlane() override;

    DSwimlane &operator=(const DSwimlane &rhs);

    Uid modelUid() const override { return Uid::invalidUid(); }
    QString text() const { return m_text; }
    void setText(const QString &text);
    bool isHorizontal() const { return m_horizontal; }
    void setHorizontal(bool horizontal);
    qreal pos() const { return m_pos; }
    void setPos(qreal pos);

    void accept(DVisitor *visitor) override;
    void accept(DConstVisitor *visitor) const override;

private:
    QString m_text;
    bool m_horizontal = false; // false: vertical, true: horizontal
    qreal m_pos = 0.0;
};

} // namespace qmt
