/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "productlistmodel.h"

#include "marketplaceplugin.h"

#include <utils/algorithm.h>
#include <utils/executeondestruction.h>
#include <utils/networkaccessmanager.h>
#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmapCache>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>

namespace Marketplace {
namespace Internal {

static const QNetworkRequest constructRequest(const QString &collection)
{
    QString url("https://marketplace.qt.io");
    if (collection.isEmpty())
        url.append("/collections.json");
    else
        url.append("/collections/").append(collection).append("/products.json");

    return QNetworkRequest(url);
}

static const QString plainTextFromHtml(const QString &original)
{
    QString plainText(original);
    QRegularExpression breakReturn("<\\s*br/?\\s*>", QRegularExpression::CaseInsensitiveOption);

    plainText.replace(breakReturn, "\n");                       // "translate" <br/> into newline
    plainText.remove(QRegularExpression("<[^>]*>"));            // remove all tags
    plainText = plainText.trimmed();
    plainText.replace(QRegularExpression("\n{3,}"), "\n\n");    // consolidate some newlines

    // FIXME the description text is usually too long and needs to get elided sensibly
    return (plainText.length() > 157) ? plainText.left(157).append("...") : plainText;
}

ProductListModel::ProductListModel(QObject *parent)
    : Core::ListModel(parent)
{
}

void ProductListModel::updateCollections()
{
    emit toggleProgressIndicator(true);
    QNetworkReply *reply = Utils::NetworkAccessManager::instance()->get(constructRequest({}));
    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() { onFetchCollectionsFinished(reply); });
}

QPixmap ProductListModel::fetchPixmapAndUpdatePixmapCache(const QString &url) const
{
    const_cast<ProductListModel *>(this)->queueImageForDownload(url);
    return QPixmap();
}

void ProductListModel::onFetchCollectionsFinished(QNetworkReply *reply)
{
    QTC_ASSERT(reply, return);
    Utils::ExecuteOnDestruction replyDeleter([reply]() { reply->deleteLater(); });

    if (reply->error() == QNetworkReply::NoError) {
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isNull())
            return;

        const QJsonArray collections = doc.object().value("collections").toArray();
        for (int i = 0, end = collections.size(); i < end; ++i) {
            const QJsonObject obj = collections.at(i).toObject();
            const auto handle = obj.value("handle").toString();
            const int productsCount = obj.value("products_count").toInt();

            if (productsCount > 0 && handle != "all-products" && handle != "qt-education-1")
                m_pendingCollections.append(handle);
        }
        if (!m_pendingCollections.isEmpty())
            fetchCollectionsContents();
    } else {
        QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        if (status.isValid() && status.toInt() == 430)
            QTimer::singleShot(30000, this, &ProductListModel::updateCollections);
        else
            emit errorOccurred(reply->error(), reply->errorString());
    }
}

void ProductListModel::onFetchSingleCollectionFinished(QNetworkReply *reply)
{
    emit toggleProgressIndicator(false);

    QTC_ASSERT(reply, return);
    Utils::ExecuteOnDestruction replyDeleter([reply]() { reply->deleteLater(); });

    QList<Core::ListItem *> productsForCollection;
    if (reply->error() == QNetworkReply::NoError) {
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isNull())
            return;

        const QJsonArray products = doc.object().value("products").toArray();
        for (int i = 0, end = products.size(); i < end; ++i) {
            const QJsonObject obj = products.at(i).toObject();
            const QString handle = obj.value("handle").toString();

            bool foundItem = Utils::findOrDefault(m_items, [handle](const Core::ListItem *it) {
                return static_cast<const ProductItem *>(it)->handle == handle;
            });
            if (foundItem)
                continue;

            ProductItem *product = new ProductItem;
            product->name = obj.value("title").toString();
            product->description = plainTextFromHtml(obj.value("body_html").toString());

            product->handle = handle;
            for (auto val : obj.value("tags").toArray())
                product->tags.append(val.toString());

            const auto images = obj.value("images").toArray();
            if (!images.isEmpty()) {
                auto imgObj = images.first().toObject();
                const QJsonValue imageSrc = imgObj.value("src");
                if (imageSrc.isString())
                    product->imageUrl = imageSrc.toString();
            }

            productsForCollection.append(product);
        }

        if (!productsForCollection.isEmpty()) {
            beginInsertRows(QModelIndex(), m_items.size(), m_items.size() + productsForCollection.size());
            m_items.append(productsForCollection);
            endInsertRows();
        }
    } else {
        // bad.. but we still might be able to fetch another collection
        qWarning() << "Failed to fetch collection:" << reply->errorString() << reply->error();
    }

    if (!m_pendingCollections.isEmpty()) // more collections? go ahead..
        fetchCollectionsContents();
    else if (m_items.isEmpty())
        emit errorOccurred(0, "Failed to fetch any collection.");
}

void ProductListModel::fetchCollectionsContents()
{
    QTC_ASSERT(!m_pendingCollections.isEmpty(), return);
    const QString collection = m_pendingCollections.dequeue();

    QNetworkReply *reply
            = Utils::NetworkAccessManager::instance()->get(constructRequest(collection));
    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() { onFetchSingleCollectionFinished(reply); });
}

void ProductListModel::queueImageForDownload(const QString &url)
{
    m_pendingImages.insert(url);
    if (!m_isDownloadingImage)
        fetchNextImage();
}

void ProductListModel::fetchNextImage()
{
    if (m_pendingImages.isEmpty()) {
        m_isDownloadingImage = false;
        return;
    }

    const auto it = m_pendingImages.begin();
    const QString nextUrl = *it;
    m_pendingImages.erase(it);

    if (QPixmapCache::find(nextUrl, nullptr)) { // this image is already cached
        updateModelIndexesForUrl(nextUrl);  // it might have been added while downloading
        fetchNextImage();
        return;
    }

    m_isDownloadingImage = true;
    QNetworkReply *reply = Utils::NetworkAccessManager::instance()->get(QNetworkRequest(nextUrl));
    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() { onImageDownloadFinished(reply); });
}

void ProductListModel::onImageDownloadFinished(QNetworkReply *reply)
{
    QTC_ASSERT(reply, return);
    Utils::ExecuteOnDestruction replyDeleter([reply]() { reply->deleteLater(); });

    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray data = reply->readAll();
        QPixmap pixmap;
        if (pixmap.loadFromData(data)) {
            const QString url = reply->request().url().toString();
            QPixmapCache::insert(url, pixmap.scaled(ProductListModel::defaultImageSize,
                                                    Qt::KeepAspectRatio, Qt::SmoothTransformation));
            updateModelIndexesForUrl(url);
        }
    } // handle error not needed - it's okay'ish to have no images as long as the rest works

    fetchNextImage();
}

void ProductListModel::updateModelIndexesForUrl(const QString &url)
{
    for (int row = 0, end = m_items.size(); row < end; ++row) {
        if (m_items.at(row)->imageUrl == url)
            emit dataChanged(index(row), index(row), {ItemImageRole, Qt::DisplayRole});
    }
}

} // namespace Internal
} // namespace Marketplace
