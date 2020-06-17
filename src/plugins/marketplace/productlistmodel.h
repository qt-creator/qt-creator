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

#pragma once

#include <coreplugin/welcomepagehelper.h>

#include <QQueue>
#include <QStackedWidget>

QT_BEGIN_NAMESPACE
class QNetworkReply;
QT_END_NAMESPACE

namespace Marketplace {
namespace Internal {

class ProductGridView;
class ProductItemDelegate;

class ProductItem : public Core::ListItem
{
public:
    QString handle;
};

class ProductListModel : public Core::ListModel
{
public:
    explicit ProductListModel(QObject *parent);
    void appendItems(const QList<Core::ListItem *> &items);
    const QList<Core::ListItem *> items() const;
    void updateModelIndexesForUrl(const QString &url);

protected:
    QPixmap fetchPixmapAndUpdatePixmapCache(const QString &url) const override;
};

struct Section
{
    QString name;
    int priority;
};

inline bool operator<(const Section &lhs, const Section &rhs)
{
    if (lhs.priority < rhs.priority)
        return true;
    return lhs.priority > rhs.priority ? false : lhs.name < rhs.name;
}

inline bool operator==(const Section &lhs, const Section &rhs)
{
    return lhs.priority == rhs.priority && lhs.name == rhs.name;
}

class SectionedProducts : public QStackedWidget
{
    Q_OBJECT
public:
    explicit SectionedProducts(QWidget *parent);
    ~SectionedProducts() override;
    void updateCollections();
    void queueImageForDownload(const QString &url);
    void setColumnCount(int columns);
    void setSearchString(const QString &searchString);

signals:
    void errorOccurred(int errorCode, const QString &errorString);
    void toggleProgressIndicator(bool show);
    void tagClicked(const QString &tag);

private:
    void onFetchCollectionsFinished(QNetworkReply *reply);
    void onFetchSingleCollectionFinished(QNetworkReply *reply);
    void fetchCollectionsContents();

    void fetchNextImage();
    void onImageDownloadFinished(QNetworkReply *reply);
    void addNewSection(const Section &section, const QList<Core::ListItem *> &items);
    void onTagClicked(const QString &tag);

    QList<Core::ListItem *> items();

    QQueue<QString> m_pendingCollections;
    QSet<QString> m_pendingImages;
    QMap<QString, QString> m_collectionTitles;
    QMap<Section, ProductListModel *> m_productModels;
    QMap<Section, ProductGridView *> m_gridViews;
    ProductGridView *m_allProductsView = nullptr;
    Core::ListModelFilter *m_filteredAllProductsModel = nullptr;
    Core::GridProxyModel * const m_gridModel;
    ProductItemDelegate *m_productDelegate = nullptr;
    bool m_isDownloadingImage = false;
    int m_columnCount = 1;
};

} // namespace Internal
} // namespace Marketplace

Q_DECLARE_METATYPE(Marketplace::Internal::ProductItem *)
