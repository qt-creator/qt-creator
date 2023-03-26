// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "layeritem.h"
#include "formeditoritem.h"

#include <QPointer>

namespace QmlDesigner {

class FormEditorItem;
class AnchorIndicatorGraphicsItem;

class AnchorIndicator
{
public:
    AnchorIndicator(LayerItem *layerItem);
    AnchorIndicator();
    ~AnchorIndicator();

    void show();
    void hide();

    void clear();

    void setItems(const QList<FormEditorItem*> &itemList);
    void updateItems(const QList<FormEditorItem*> &itemList);


private:
    QPointer<LayerItem> m_layerItem;
    FormEditorItem *m_formEditorItem = nullptr;
    QPointer<AnchorIndicatorGraphicsItem> m_indicatorTopShape;
    QPointer<AnchorIndicatorGraphicsItem> m_indicatorBottomShape;
    QPointer<AnchorIndicatorGraphicsItem> m_indicatorLeftShape;
    QPointer<AnchorIndicatorGraphicsItem> m_indicatorRightShape;
};

} // namespace QmlDesigner
