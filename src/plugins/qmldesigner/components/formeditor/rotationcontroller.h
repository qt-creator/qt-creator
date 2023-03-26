// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QWeakPointer>
#include <QSharedPointer>

namespace QmlDesigner {

class FormEditorItem;
class LayerItem;
class RotationHandleItem;
class RotationControllerData;
class WeakRotationController;

class RotationController
{
    friend WeakRotationController;

public:
    RotationController();
    RotationController(LayerItem *layerItem, FormEditorItem *formEditorItem);
    RotationController(const RotationController &rotationController);
    RotationController(const WeakRotationController &rotationController);
    ~RotationController();

    RotationController& operator=(const RotationController &other);

    void show();
    void hide();

    void updatePosition();

    bool isValid() const;

    FormEditorItem *formEditorItem() const;

    bool isTopLeftHandle(const RotationHandleItem *handle) const;
    bool isTopRightHandle(const RotationHandleItem *handle) const;
    bool isBottomLeftHandle(const RotationHandleItem *handle) const;
    bool isBottomRightHandle(const RotationHandleItem *handle) const;

    WeakRotationController toWeakRotationController() const;

private:
    RotationController(const QSharedPointer<RotationControllerData> &data);

    QCursor getRotationCursor() const;

private:
    QSharedPointer<RotationControllerData> m_data;
};

class WeakRotationController
{
    friend RotationController;

public:
    WeakRotationController();
    WeakRotationController(const WeakRotationController &rotationController);
    WeakRotationController(const RotationController &rotationController);
    ~WeakRotationController();

    WeakRotationController& operator=(const WeakRotationController &other);

    RotationController toRotationController() const;

private: // variables
    QWeakPointer<RotationControllerData> m_data;
};

} // namespace QmlDesigner
