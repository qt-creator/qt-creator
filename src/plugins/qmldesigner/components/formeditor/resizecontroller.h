// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QWeakPointer>
#include <QSharedPointer>

namespace QmlDesigner {

class FormEditorItem;
class LayerItem;
class ResizeHandleItem;
class ResizeControllerData;
class WeakResizeController;

class ResizeController
{
    friend WeakResizeController;

public:
    ResizeController();
    ResizeController(LayerItem *layerItem, FormEditorItem *formEditorItem);
    ResizeController(const ResizeController &resizeController);
    ResizeController(const WeakResizeController &resizeController);
    ~ResizeController();

    ResizeController& operator=(const ResizeController &other);

    void show();
    void hide();

    void updatePosition();

    bool isValid() const;

    FormEditorItem *formEditorItem() const;

    bool isTopLeftHandle(const ResizeHandleItem *handle) const;
    bool isTopRightHandle(const ResizeHandleItem *handle) const;
    bool isBottomLeftHandle(const ResizeHandleItem *handle) const;
    bool isBottomRightHandle(const ResizeHandleItem *handle) const;

    bool isTopHandle(const ResizeHandleItem *handle) const;
    bool isLeftHandle(const ResizeHandleItem *handle) const;
    bool isRightHandle(const ResizeHandleItem *handle) const;
    bool isBottomHandle(const ResizeHandleItem *handle) const;

    WeakResizeController toWeakResizeController() const;

private: // functions
    ResizeController(const QSharedPointer<ResizeControllerData> &data);
private: // variables
    QSharedPointer<ResizeControllerData> m_data;
};

class WeakResizeController
{
    friend ResizeController;

public:
    WeakResizeController();
    WeakResizeController(const WeakResizeController &resizeController);
    WeakResizeController(const ResizeController &resizeController);
    ~WeakResizeController();

    WeakResizeController& operator=(const WeakResizeController &other);

    ResizeController toResizeController() const;

private: // variables
    QWeakPointer<ResizeControllerData> m_data;
};

} // namespace QmlDesigner
