// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QGraphicsRectItem>

namespace QmlDesigner {

class LayerItem;
class ScaleIndicator;

class ScaleItem : public QGraphicsRectItem
{
public:
    ScaleItem(LayerItem *layerItem, ScaleIndicator *indicator);

    ScaleIndicator* indicator() const;

private:
    ScaleIndicator* m_indicator;
};

} // namespace QmlDesigner
