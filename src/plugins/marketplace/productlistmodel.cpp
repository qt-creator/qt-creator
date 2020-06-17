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

#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmapCache>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace Marketplace {
namespace Internal {

/**
 * @brief AllProductsModel does not own its items. Using this model only to display
 * the same items stored inside other models without the need to duplicate the items.
 */
class AllProductsModel : public ProductListModel
{
public:
    explicit AllProductsModel(QObject *parent) : ProductListModel(parent) {}
    ~AllProductsModel() override { m_items.clear(); }
};

class ProductGridView : public Core::GridView
{
public:
    ProductGridView(QWidget *parent) : Core::GridView(parent) {}
    QSize viewportSizeHint() const override
    {
        if (!model())
            return Core::GridView::viewportSizeHint();

        static int gridW = Core::GridProxyModel::GridItemWidth + Core::GridProxyModel::GridItemGap;
        static int gridH = Core::GridProxyModel::GridItemHeight + Core::GridProxyModel::GridItemGap;
        return QSize(model()->columnCount() * gridW, model()->rowCount() * gridH);
    }

    void setColumnCount(int columnCount)
    {
        if (columnCount < 1)
            columnCount = 1;

        auto gridProxyModel = static_cast<Core::GridProxyModel *>(model());
        gridProxyModel->setColumnCount(columnCount);
    }
};

class ProductItemDelegate : public Core::ListItemDelegate
{
public:
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        const Core::ListItem *item = index.data(Core::ListModel::ItemRole).value<Core::ListItem *>();

        // "empty" items (last row of a section)
        if (!item)
            return Core::ListItemDelegate::sizeHint(option, index);

        return QSize(Core::GridProxyModel::GridItemWidth + Core::GridProxyModel::GridItemGap,
                     Core::GridProxyModel::GridItemHeight + Core::GridProxyModel::GridItemGap);
    }

    void clickAction(const Core::ListItem *item) const override
    {
        QTC_ASSERT(item, return);
        auto productItem = static_cast<const ProductItem *>(item);
        const QUrl url(QString("https://marketplace.qt.io/products/").append(productItem->handle));
        QDesktopServices::openUrl(url);
    }
};

ProductListModel::ProductListModel(QObject *parent)
    : Core::ListModel(parent)
{
}

void ProductListModel::appendItems(const QList<Core::ListItem *> &items)
{
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size() + items.size());
    m_items.append(items);
    endInsertRows();
}

const QList<Core::ListItem *> ProductListModel::items() const
{
    return m_items;
}

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

static int priority(const QString &collection)
{
    if (collection == "featured")
        return 10;
    if (collection == "from-qt-partners")
        return 20;
    return 50;
}

SectionedProducts::SectionedProducts(QWidget *parent)
    : QStackedWidget(parent)
    , m_allProductsView(new ProductGridView(this))
    , m_filteredAllProductsModel(new Core::ListModelFilter(new AllProductsModel(this), this))
    , m_gridModel(new Core::GridProxyModel)
    , m_productDelegate(new ProductItemDelegate)
{
    auto area = new QScrollArea(this);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    area->setFrameShape(QFrame::NoFrame);
    area->setWidgetResizable(true);

    auto sectionedView = new QWidget;
    auto layout = new QVBoxLayout;
    layout->addStretch();
    sectionedView->setLayout(layout);
    area->setWidget(sectionedView);

    addWidget(area);

    m_gridModel->setSourceModel(m_filteredAllProductsModel);
    m_allProductsView->setItemDelegate(m_productDelegate);
    m_allProductsView->setModel(m_gridModel);
    addWidget(m_allProductsView);

    connect(m_productDelegate, &ProductItemDelegate::tagClicked,
            this, &SectionedProducts::onTagClicked);
}

SectionedProducts::~SectionedProducts()
{
    qDeleteAll(m_gridViews.values());
    delete m_productDelegate;
    delete m_gridModel;
}

void SectionedProducts::updateCollections()
{
    emit toggleProgressIndicator(true);
    QNetworkReply *reply = Utils::NetworkAccessManager::instance()->get(constructRequest({}));
    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() { onFetchCollectionsFinished(reply); });
}

QPixmap ProductListModel::fetchPixmapAndUpdatePixmapCache(const QString &url) const
{
    if (auto sectionedProducts = qobject_cast<SectionedProducts *>(parent()))
        sectionedProducts->queueImageForDownload(url);
    return QPixmap();
}

void SectionedProducts::onFetchCollectionsFinished(QNetworkReply *reply)
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
    Utils::ExecuteOnDestruction replyDeleter([reply]() { reply->deleteLater(); });

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

void SectionedProducts::setColumnCount(int columns)
{
    if (columns < 1)
        columns = 1;
    m_columnCount = columns;
    for (ProductGridView *view : m_gridViews.values()) {
        view->setColumnCount(columns);
        view->setFixedSize(view->viewportSizeHint());
    }
    m_allProductsView->setColumnCount(columns);
}

void SectionedProducts::setSearchString(const QString &searchString)
{
    int view = searchString.isEmpty() ? 0  // sectioned view
                                      : 1; // search view
    setCurrentIndex(view);
    m_filteredAllProductsModel->setSearchString(searchString);
}

void SectionedProducts::fetchNextImage()
{
    if (m_pendingImages.isEmpty()) {
        m_isDownloadingImage = false;
        return;
    }

    const auto it = m_pendingImages.begin();
    const QString nextUrl = *it;
    m_pendingImages.erase(it);

    if (QPixmapCache::find(nextUrl, nullptr)) {
        // this image is already cached it might have been added while downloading
        for (ProductListModel *model : m_productModels.values())
            model->updateModelIndexesForUrl(nextUrl);
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
    Utils::ExecuteOnDestruction replyDeleter([reply]() { reply->deleteLater(); });

    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray data = reply->readAll();
        QPixmap pixmap;
        if (pixmap.loadFromData(data)) {
            const QString url = reply->request().url().toString();
            QPixmapCache::insert(url, pixmap.scaled(ProductListModel::defaultImageSize,
                                                    Qt::KeepAspectRatio, Qt::SmoothTransformation));
            for (ProductListModel *model : m_productModels.values())
                model->updateModelIndexesForUrl(url);
        }
    } // handle error not needed - it's okay'ish to have no images as long as the rest works

    fetchNextImage();
}

void SectionedProducts::addNewSection(const Section &section, const QList<Core::ListItem *> &items)
{
    QTC_ASSERT(!items.isEmpty(), return);
    ProductListModel *productModel = new ProductListModel(this);
    productModel->appendItems(items);
    auto filteredModel = new Core::ListModelFilter(productModel, this);
    Core::GridProxyModel *gridModel = new Core::GridProxyModel;
    gridModel->setSourceModel(filteredModel);
    auto gridView = new ProductGridView(this);
    gridView->setItemDelegate(m_productDelegate);
    gridView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gridView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gridView->setModel(gridModel);
    gridModel->setColumnCount(m_columnCount);

    m_productModels.insert(section, productModel);
    m_gridViews.insert(section, gridView);

    QFont f = font();
    f.setPixelSize(16);
    auto sectionLabel = new QLabel(section.name);
    sectionLabel->setFont(f);
    auto scrollArea = qobject_cast<QScrollArea *>(widget(0));
    auto vbox = qobject_cast<QVBoxLayout *>(scrollArea->widget()->layout());

    // insert new section depending on its priority, but before the last (stretch) item
    int position = m_gridViews.keys().indexOf(section) * 2; // a section has a label and a grid
    QTC_ASSERT(position <= vbox->count() - 1, position = vbox->count() - 1);
    vbox->insertWidget(position, sectionLabel);
    vbox->insertWidget(position + 1, gridView);
    gridView->setFixedSize(gridView->viewportSizeHint());

    // add the items also to the all products model to be able to search correctly
    auto allProducts = static_cast<ProductListModel *>(m_filteredAllProductsModel->sourceModel());
    allProducts->appendItems(items);
    m_allProductsView->setColumnCount(m_columnCount);
}

void SectionedProducts::onTagClicked(const QString &tag)
{
    setCurrentIndex(1 /* search */);
    emit tagClicked(tag);
}

QList<Core::ListItem *> SectionedProducts::items()
{
    QList<Core::ListItem *> result;
    for (const ProductListModel *model : m_productModels.values())
        result.append(model->items());
    return result;
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
