// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "materialeditorimageprovider.h"

#include "materialeditorview.h"

#include <QImage>
#include <QTimer>

namespace QmlDesigner {

MaterialEditorImageProvider::MaterialEditorImageProvider(MaterialEditorView *materialEditorView)
    : QQuickImageProvider(Pixmap)
    , m_delayTimer(new QTimer(this))
{
    m_delayTimer->setInterval(500);
    m_delayTimer->setSingleShot(true);
    m_delayTimer->callOnTimeout([this] {
        if (m_previewPixmap.size() != m_requestedSize)
            emit this->requestPreview(m_requestedSize);
    });

    connect(this,
            &MaterialEditorImageProvider::requestPreview,
            materialEditorView,
            &MaterialEditorView::handlePreviewSizeChanged);
}

void MaterialEditorImageProvider::setPixmap(const QPixmap &pixmap)
{
    m_previewPixmap = pixmap;
}

QPixmap MaterialEditorImageProvider::requestPixmap(const QString &id,
                                                   QSize *size,
                                                   const QSize &requestedSize)
{
    static QPixmap defaultPreview = QPixmap::fromImage(
        QImage(":/materialeditor/images/defaultmaterialpreview.png"));

    QPixmap pixmap{150, 150};

    if (id == "preview") {
        if (!m_previewPixmap.isNull()) {
            pixmap = m_previewPixmap;
            setRequestedSize(requestedSize);
        } else {
            if (requestedSize.isEmpty())
                pixmap = defaultPreview;
            else
                pixmap = defaultPreview.scaled(requestedSize, Qt::KeepAspectRatio);
        }
    } else {
        qWarning() << __FUNCTION__ << "Unsupported image id:" << id;
        pixmap.fill(Qt::red);
    }

    if (size)
        *size = pixmap.size();

    return pixmap;
}

/*!
 * \internal
 * \brief Sets the requested size. If the requested size is not the same as the
 * size of the m_previewPixmap, it will ask \l {MaterialEditorView} to provide
 * an image with the requested size
 * The requests are delayed until the requested size is stable.
 */
void MaterialEditorImageProvider::setRequestedSize(const QSize &requestedSize)
{
    if (requestedSize.isEmpty())
        return;

    m_requestedSize = requestedSize;

    if (m_previewPixmap.size() != requestedSize)
        m_delayTimer->start();
}

} // namespace QmlDesigner
