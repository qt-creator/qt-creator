// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QGraphicsRectItem>

namespace QmlDesigner {

class LayerItem;

class ControlElement
{
public:
    ControlElement(LayerItem *layerItem);
    ~ControlElement();

    void hide();
    void setBoundingRect(const QRectF &rect);

private:
    QGraphicsRectItem *m_controlShape;
};

} // namespace QmlDesigner
