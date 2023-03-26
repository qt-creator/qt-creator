// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsSimpleTextItem>

namespace qmt {

class ContextLabelItem : public QGraphicsSimpleTextItem
{
public:
    explicit ContextLabelItem(QGraphicsItem *parent = nullptr);
    ~ContextLabelItem() override;

    void setMaxWidth(double maxWidth);
    void resetMaxWidth();
    void setContext(const QString &context);
    double height() const;

private:
    void update();

    double m_maxWidth = 0.0;
    QString m_context;
};

} // namespace qmt
