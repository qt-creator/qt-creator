// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "layeritem.h"
#include "formeditoritem.h"

#include <QPointer>

namespace QmlDesigner {

class FormEditorItem;
class BindingIndicatorGraphicsItem;

class BindingIndicator
{
public:
    BindingIndicator(LayerItem *layerItem);
    BindingIndicator();
    ~BindingIndicator();

    void show();
    void hide();
    void clear();

    void setItems(const QList<FormEditorItem*> &itemList);
    void updateItems(const QList<FormEditorItem*> &itemList);

private:
    QPointer<LayerItem> m_layerItem;
    FormEditorItem *m_formEditorItem = nullptr;
    QPointer<BindingIndicatorGraphicsItem> m_indicatorTopShape;
    QPointer<BindingIndicatorGraphicsItem> m_indicatorBottomShape;
    QPointer<BindingIndicatorGraphicsItem> m_indicatorLeftShape;
    QPointer<BindingIndicatorGraphicsItem> m_indicatorRightShape;
};

} // namespace QmlDesigner
