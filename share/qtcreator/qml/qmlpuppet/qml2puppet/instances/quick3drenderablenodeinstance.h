/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <QtGlobal>

#include "objectnodeinstance.h"

namespace QmlDesigner {
namespace Internal {

class Quick3DRenderableNodeInstance : public ObjectNodeInstance
{
public:
    ~Quick3DRenderableNodeInstance() override;
    void initialize(const ObjectNodeInstance::Pointer &objectNodeInstance,
                    InstanceContainer::NodeFlags flags) override;

    QImage renderImage() const override;
    QImage renderPreviewImage(const QSize &previewImageSize) const override;

    bool isRenderable() const override;
    bool hasContent() const override;
    QRectF boundingRect() const override;
    QRectF contentItemBoundingBox() const override;
    QPointF position() const override;
    QSizeF size() const override;

    QList<ServerNodeInstance> stateInstances() const override;

    QQuickItem *contentItem() const override;
    void setPropertyVariant(const PropertyName &name, const QVariant &value) override;

protected:
    explicit Quick3DRenderableNodeInstance(QObject *node);
    Qt5NodeInstanceServer *qt5NodeInstanceServer() const;
    virtual void invokeDummyViewCreate() const;

    QQuickItem *m_dummyRootView = nullptr;
};

} // namespace Internal
} // namespace QmlDesigner
