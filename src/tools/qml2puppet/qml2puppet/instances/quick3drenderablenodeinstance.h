// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
