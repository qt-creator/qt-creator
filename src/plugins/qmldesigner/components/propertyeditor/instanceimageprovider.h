// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <model.h>
#include <modelnode.h>

#include <QCache>
#include <QQuickImageProvider>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace QmlDesigner {

class InstanceImageProvider : public QQuickImageProvider
{
    Q_OBJECT

public:
    explicit InstanceImageProvider();

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;

    bool feedImage(const ModelNode &node, const QPixmap &pixmap, const QByteArray &requestId);
    void setModelNode(const ModelNode &node);

    bool hasPendingRequest() const;
    void invalidate();

protected:
    void requestOne();
    void requestOne(QSize size);
    void postponeRequest(QSize size);
    void prepareRequest(QSize size);
    bool dataAvailable(const ModelNode &node, QSize size);

    QPixmap getScaledImage(QSize size);

private:
    QByteArray m_pendingRequest;
    bool m_resetRequest = false;

    ModelNode m_requestedNode;
    ModelNode m_receivedNode;
    QSize m_requestedSize;

    QPixmap m_receivedImage;

    QTimer *m_delayTimer = nullptr;
};

} // namespace QmlDesigner
