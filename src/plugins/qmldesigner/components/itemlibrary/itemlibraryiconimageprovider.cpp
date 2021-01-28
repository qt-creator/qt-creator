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

#include "itemlibraryiconimageprovider.h"

#include <projectexplorer/target.h>
#include <utils/stylehelper.h>

#include <QMetaObject>
#include <QQuickImageResponse>

namespace QmlDesigner {

class ImageRespose : public QQuickImageResponse
{
public:
    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

    void setImage(const QImage &image)
    {
        m_image = image;

        emit finished();
    }

    void abort()
    {
        m_image = QImage{
            Utils::StyleHelper::dpiSpecificImageFile(":/ItemLibrary/images/item-default-icon.png")};

        emit finished();
    }

private:
    QImage m_image;
};


QQuickImageResponse *ItemLibraryIconImageProvider::requestImageResponse(const QString &id,
                                                                        const QSize &)
{
    auto response = std::make_unique<ImageRespose>();

    m_cache.requestSmallImage(
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
