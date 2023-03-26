// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QPointF>

QT_BEGIN_NAMESPACE
class QGraphicsLineItem;
QT_END_NAMESPACE

namespace QmlDesigner {

class LayerItem;
class FormEditorItem;

class ScaleManipulator
{
public:
    ScaleManipulator(LayerItem *layerItem, FormEditorItem *formEditorItem);
    virtual ~ScaleManipulator();

    void begin(const QPointF& beginPoint);
    void update(const QPointF& updatePoint);
    void reparentTo(FormEditorItem *newParent);
    void end(const QPointF& updatePoint);

private:
  LayerItem *m_layerItem;
  FormEditorItem *m_formEditorItem;
};

} // namespace QmlDesigner
