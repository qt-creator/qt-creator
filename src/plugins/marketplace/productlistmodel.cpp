// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "productlistmodel.h"

#include <utils/algorithm.h>
#include <utils/networkaccessmanager.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QDesktopServices>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmapCache>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

using namespace Core;

namespace Marketplace {
namespace Internal {

class ProductItemDelegate : public Core::ListItemDelegate
{
public:
    void clickAction(const Core::ListItem *item) const override
    {
        QTC_ASSERT(item, return);
        auto productItem = static_cast<const ProductItem *>(item);
        const QUrl url(QString("https://marketplace.qt.io/products/").append(productItem->handle));
        QDesktopServices::openUrl(url);
    }
};

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
    static const QRegularExpression breakReturn("<\\s*br/?\\s*>",
                                                QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression allTags("<[^>]*>");
    static const QRegularExpression moreThan2NewLines("\n{3,}");
    plainText.replace(breakReturn, "\n"); // "translate" <br/> into newline
    plainText.remove(allTags);            // remove all tags
    plainText = plainText.trimmed();
    plainText.replace(moreThan2NewLines, "\n\n"); // consolidate some newlines

    // FIXME the description text is usually too long and needs to get elided sensibly
    return (plainText.length() > 157) ? plainText.left(157).append("...") : plainText;
}

static int priority(const QString &collection)
{
    if (collection == "featured")
        return 10;
    if (collection == "from-qt-partners")
        return 20;
    return 50;
}

SectionedProducts::SectionedProducts(QWidget *parent)
    : SectionedGridView(parent)
    , m_productDelegate(new ProductItemDelegate)
{
    setItemDelegate(m_productDelegate);
    setPixmapFunction([this](const QString &url) -> QPixmap {
        queueImageForDownload(url);
        return {};
    });
    connect(m_productDelegate,
            &ProductItemDelegate::tagClicked,
            this,
            &SectionedProducts::onTagClicked);
}

SectionedProducts::~SectionedProducts()
{
    delete m_productDelegate;
}

void SectionedProducts::updateCollections()
{
    emit toggleProgressIndicator(true);
    QNetworkReply *reply = Utils::NetworkAccessManager::instance()->get(constructRequest({}));
    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() { onFetchCollectionsFinished(reply); });
}

void SectionedProducts::onFetchCollectionsFinished(QNetworkReply *reply)
{
    QTC_ASSERT(reply, return);
    const QScopeGuard cleanup([reply] { reply->deleteLater(); });

    if (reply->error() == QNetworkReply::NoError) {
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isNull())
            return;

        const QJsonArray collections = doc.object().value("collections").toArray();
        for (int i = 0, end = collections.size(); i < end; ++i) {
            const QJsonObject obj = collections.at(i).toObject();
            const auto handle = obj.value("handle").toString();
            const int productsCount = obj.value("products_count").toInt();

            if (productsCount > 0 && handle != "all-products" && handle != "qt-education-1") {
                m_collectionTitles.insert(handle, obj.value("title").toString());
                m_pendingCollections.append(handle);
            }
        }
        if (!m_pendingCollections.isEmpty())
            fetchCollectionsContents();
    } else {
        QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        if (status.isValid() && status.toInt() == 430)
            QTimer::singleShot(30000, this, &SectionedProducts::updateCollections);
        else
            emit errorOccurred(reply->error(), reply->errorString());
    }
}

void SectionedProducts::onFetchSingleCollectionFinished(QNetworkReply *reply)
{
    emit toggleProgressIndicator(false);

    QTC_ASSERT(reply, return);
    const QScopeGuard cleanup([reply] { reply->deleteLater(); });

    QList<Core::ListItem *> productsForCollection;
    if (reply->error() == QNetworkReply::NoError) {
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isNull())
            return;

        QString collectionHandle = reply->url().path();
        if (QTC_GUARD(collectionHandle.endsWith("/products.json"))) {
            collectionHandle.chop(14);
            collectionHandle = collectionHandle.mid(collectionHandle.lastIndexOf('/') + 1);
        }

        const QList<Core::ListItem *> presentItems = items();
        const QJsonArray products = doc.object().value("products").toArray();
        for (int i = 0, end = products.size(); i < end; ++i) {
            const QJsonObject obj = products.at(i).toObject();
            const QString handle = obj.value("handle").toString();

            bool foundItem = Utils::findOrDefault(presentItems, [handle](const Core::ListItem *it) {
                return static_cast<const ProductItem *>(it)->handle == handle;
            });
            if (foundItem)
                continue;

            ProductItem *product = new ProductItem;
            product->name = obj.value("title").toString();
            product->description = plainTextFromHtml(obj.value("body_html").toString());

            product->handle = handle;
            const QJsonArray tags = obj.value("tags").toArray();
            for (const auto &val : tags)
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
            Section section{m_collectionTitles.value(collectionHandle), priority(collectionHandle)};
            addNewSection(section, productsForCollection);
        }
    } else {
        // bad.. but we still might be able to fetch another collection
        qWarning() << "Failed to fetch collection:" << reply->errorString() << reply->error();
    }

    if (!m_pendingCollections.isEmpty()) // more collections? go ahead..
        fetchCollectionsContents();
    else if (m_productModels.isEmpty())
        emit errorOccurred(0, "Failed to fetch any collection.");
}

void SectionedProducts::fetchCollectionsContents()
{
    QTC_ASSERT(!m_pendingCollections.isEmpty(), return);
    const QString collection = m_pendingCollections.dequeue();

    QNetworkReply *reply
            = Utils::NetworkAccessManager::instance()->get(constructRequest(collection));
    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() { onFetchSingleCollectionFinished(reply); });
}

void SectionedProducts::queueImageForDownload(const QString &url)
{
    m_pendingImages.insert(url);
    if (!m_isDownloadingImage)
        fetchNextImage();
}

static void updateModelIndexesForUrl(ListModel *model, const QString &url)
{
    const QList<ListItem *> items = model->items();
    for (int row = 0, end = items.size(); row < end; ++row) {
        if (items.at(row)->imageUrl == url) {
            const QModelIndex index = model->index(row);
            emit model->dataChanged(index, index, {ListModel::ItemImageRole, Qt::DisplayRole});
        }
    }
}

void SectionedProducts::fetchNextImage()
{
    if (m_pendingImages.isEmpty()) {
        m_isDownloadingImage = false;
        return;
    }

    const auto it = m_pendingImages.constBegin();
    const QString nextUrl = *it;
    m_pendingImages.erase(it);

    if (QPixmapCache::find(nextUrl, nullptr)) {
        // this image is already cached it might have been added while downloading
        for (ListModel *model : std::as_const(m_productModels))
            updateModelIndexesForUrl(model, nextUrl);
        fetchNextImage();
        return;
    }

    m_isDownloadingImage = true;
    QNetworkReply *reply = Utils::NetworkAccessManager::instance()->get(QNetworkRequest(nextUrl));
    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() { onImageDownloadFinished(reply); });
}

void SectionedProducts::onImageDownloadFinished(QNetworkReply *reply)
{
    QTC_ASSERT(reply, return);
    const QScopeGuard cleanup([reply] { reply->deleteLater(); });

    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray data = reply->readAll();
        QPixmap pixmap;
        const QUrl imageUrl = reply->request().url();
        const QString imageFormat = QFileInfo(imageUrl.fileName()).suffix();
        if (pixmap.loadFromData(data, imageFormat.toLatin1())) {
            const QString url = imageUrl.toString();
            const int dpr = qApp->devicePixelRatio();
            pixmap = pixmap.scaled(WelcomePageHelpers::GridItemImageSize * dpr,
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
            pixmap.setDevicePixelRatio(dpr);
            QPixmapCache::insert(url, pixmap);
            for (ListModel *model : std::as_const(m_productModels))
                updateModelIndexesForUrl(model, url);
        }
    } // handle error not needed - it's okay'ish to have no images as long as the rest works

    fetchNextImage();
}

void SectionedProducts::addNewSection(const Section &section, const QList<Core::ListItem *> &items)
{
    QTC_ASSERT(!items.isEmpty(), return);
    m_productModels.append(addSection(section, items));
}

void SectionedProducts::onTagClicked(const QString &tag)
{
    setCurrentIndex(1 /* search */);
    emit tagClicked(tag);
}

QList<Core::ListItem *> SectionedProducts::items()
{
    QList<Core::ListItem *> result;
    for (const ListModel *model : std::as_const(m_productModels))
        result.append(model->items());
    return result;
}

} // namespace Internal
} // namespace Marketplace
