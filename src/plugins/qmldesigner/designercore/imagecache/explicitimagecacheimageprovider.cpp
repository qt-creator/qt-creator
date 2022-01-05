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

#include "explicitimagecacheimageprovider.h"

#include <asynchronousexplicitimagecache.h>

#include <QMetaObject>
#include <QQuickImageResponse>

namespace QmlDesigner {

class ImageRespose : public QQuickImageResponse
{
public:
    ImageRespose(const QImage &defaultImage)
        : m_image(defaultImage)
    {}

    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

    void setImage(const QImage &image)
    {
        m_image = image;

        emit finished();
    }

    void abort() { emit finished(); }

private:
    QImage m_image;
};

QQuickImageResponse *ExplicitImageCacheImageProvider::requestImageResponse(const QString &id,
                                                                           const QSize &)
{
    auto response = std::make_unique<ImageRespose>(m_defaultImage);

    m_cache.requestImage(
        id,
        [response = QPointer<ImageRespose>(response.get())](const QImage &image) {
            QMetaObject::invokeMethod(
                response,
                [response, image] {
                    if (response)
                        response->setImage(image);
                },
                Qt::QueuedConnection);
        },
        [response = QPointer<ImageRespose>(response.get())](ImageCache::AbortReason abortReason) {
            QMetaObject::invokeMethod(
                response,
                [response, abortReason] {
                    switch (abortReason) {
                    case ImageCache::AbortReason::Failed:
                        if (response)
                            response->abort();
                        break;
                    case ImageCache::AbortReason::Abort:
                        response->cancel();
                        break;
                    }
                },
                Qt::QueuedConnection);
        });

    return response.release();
}

} // namespace QmlDesigner
