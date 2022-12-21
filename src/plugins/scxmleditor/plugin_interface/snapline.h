// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsLineItem>
#include <QTimer>

namespace ScxmlEditor {

namespace PluginInterface {

class SnapLine : public QObject, public QGraphicsLineItem
{
    Q_OBJECT

public:
    SnapLine(QGraphicsItem *parent = nullptr);

    void show(qreal x1, qreal y1, qreal x2, qreal y2);
    void hideLine();

private:
    QTimer m_visibilityTimer;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
