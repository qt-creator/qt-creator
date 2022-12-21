// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QToolButton>

QT_FORWARD_DECLARE_CLASS(QMouseEvent)

namespace ScxmlEditor {

namespace Common {

class DragShapeButton : public QToolButton
{
public:
    explicit DragShapeButton(QWidget *parent = nullptr);

    void setShapeInfo(int groupIndex, int shapeIndex);

protected:
    void mousePressEvent(QMouseEvent *e) override;

private:
    int m_groupIndex = 0;
    int m_shapeIndex = 0;
};

} // namespace Common
} // namespace ScxmlEditor
