/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
class ResizeHandleItem;
class ResizeControllerData;
class WeakResizeController;

class ResizeController
{
    friend class WeakResizeController;
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
    friend class ResizeController;
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
