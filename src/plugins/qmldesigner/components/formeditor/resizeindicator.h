// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "resizecontroller.h"

#include <QHash>
#include <QPair>

namespace QmlDesigner {

class FormEditorItem;
class LayerItem;

class ResizeIndicator
{
public:
    enum Orientation {
        Top = 1,
        Right = 2,
        Bottom = 4,
        Left = 8
    };

    explicit ResizeIndicator(LayerItem *layerItem);
    ~ResizeIndicator();

    void show();
    void hide();
    void clear();

    void setItems(const QList<FormEditorItem*> &itemList);
    void updateItems(const QList<FormEditorItem*> &itemList);

private:
    QHash<FormEditorItem*, ResizeController> m_itemControllerHash;
    LayerItem *m_layerItem;
};

} // namespace QmlDesigner
