// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "instanceimageprovider.h"

#include <abstractview.h>
#include <QTimer>

static const char INSTANCE_IMAGE_REQUEST_ID[] = "PropertyEditor.InstanceImage";

namespace QmlDesigner {

InstanceImageProvider::InstanceImageProvider()
    : QQuickImageProvider(Pixmap)
    , m_delayTimer(new QTimer(this))
{
    m_delayTimer->setInterval(500);
    m_delayTimer->setSingleShot(true);
    m_delayTimer->callOnTimeout([this] { requestOne(); });
}

/*!
 * \internal
 * \brief If a fresh image for the node is available, it returns it.
 * Otherwise, if the requested node matches the received node, it loads a rescaled image of the
 * most recent received image.
 * But since it's been rescaled, and probably doesn't have a good resolution,
 * it requests one more to get a new image.
 * \return The most recent image received for the node from NodeInstanceView.
 */
QPixmap InstanceImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    using namespace Qt::StringLiterals;
    static const QPixmap defaultImage = QPixmap::fromImage(
        QImage(":/propertyeditor/images/defaultmaterialpreview.png"));

    if (id != "preview")
        return defaultImage;

    if (!m_requestedNode)
        return defaultImage;

    QPixmap result;
    if (dataAvailable(m_requestedNode, requestedSize)) {
        result = m_receivedImage;
    } else {
        result = (m_requestedNode == m_receivedNode) ? getScaledImage(requestedSize) : defaultImage;

        // Here we should request one more image since the dataAvailable was false, and it means
        // that we've temporarily returned a scaled image. But we need another fresh image with
        // the correct size.

        if (hasPendingRequest())
            postponeRequest(requestedSize);
        else
            requestOne(requestedSize);
    }

    if (result.isNull())
        result = defaultImage;

    if (size)
        *size = result.size();

    return result;
}

bool InstanceImageProvider::feedImage(const ModelNode &node,
                                      const QPixmap &pixmap,
                                      const QByteArray &requestId)
{
    if (!requestId.startsWith(INSTANCE_IMAGE_REQUEST_ID))
        return false;

    if (m_pendingRequest == requestId)
        m_pendingRequest.clear();

    m_receivedImage = pixmap;
    m_receivedNode = node;

    return true;
}

void InstanceImageProvider::setModelNode(const ModelNode &node)
{
    m_requestedNode = node;
}

bool InstanceImageProvider::hasPendingRequest() const
{
    return !m_pendingRequest.isEmpty();
}

void InstanceImageProvider::requestOne()
{
    if (!m_requestedNode)
        return;

    static int requestId = 0;
    QByteArray previewRequestId = QByteArray(INSTANCE_IMAGE_REQUEST_ID)
                                  + QByteArray::number(++requestId);
    m_pendingRequest = previewRequestId;

    m_resetRequest = false;

    m_requestedNode.model()->sendCustomNotificationToNodeInstanceView(
        NodePreviewImage{m_requestedNode, {}, m_requestedSize, previewRequestId});
}

void InstanceImageProvider::requestOne(QSize size)
{
    prepareRequest(size);
    requestOne();
}

void InstanceImageProvider::postponeRequest(QSize size)
{
    prepareRequest(size);
    QMetaObject::invokeMethod(m_delayTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
}

void InstanceImageProvider::prepareRequest(QSize size)
{
    m_requestedSize = size;
}

/*!
 * \internal
 * \return true if the node matches the most recent node received from NodeInstanceView, and the
 * size is the same as the received one, and data is not invalidated after the image is received.
 */
bool InstanceImageProvider::dataAvailable(const ModelNode &node, QSize size)
{
    return !m_resetRequest && node == m_receivedNode && size == m_receivedImage.size();
}

void InstanceImageProvider::invalidate()
{
    m_resetRequest = true;
}

QPixmap InstanceImageProvider::getScaledImage(QSize size)
{
    if (size == m_receivedImage.size())
        return m_receivedImage;
    else
        return m_receivedImage.scaled(size, Qt::KeepAspectRatioByExpanding);
}

} // namespace QmlDesigner
