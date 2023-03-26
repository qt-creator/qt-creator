// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

class SectionedProducts : public Core::SectionedGridView
{
    Q_OBJECT
public:
    explicit SectionedProducts(QWidget *parent);
    ~SectionedProducts() override;
    void updateCollections();
    void queueImageForDownload(const QString &url);

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
    void addNewSection(const Core::Section &section, const QList<Core::ListItem *> &items);
    void onTagClicked(const QString &tag);

    QList<Core::ListItem *> items();

    QQueue<QString> m_pendingCollections;
    QSet<QString> m_pendingImages;
    QMap<QString, QString> m_collectionTitles;
    QList<Core::ListModel *> m_productModels;
    ProductItemDelegate *m_productDelegate = nullptr;
    bool m_isDownloadingImage = false;
    int m_columnCount = 1;
};

} // namespace Internal
} // namespace Marketplace

Q_DECLARE_METATYPE(Marketplace::Internal::ProductItem *)
