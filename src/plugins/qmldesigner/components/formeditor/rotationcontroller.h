/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
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
