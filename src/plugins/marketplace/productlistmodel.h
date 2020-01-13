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

QT_BEGIN_NAMESPACE
class QNetworkReply;
QT_END_NAMESPACE

namespace Marketplace {
namespace Internal {

class ProductItem : public Core::ListItem
{
public:
    QString handle;
};

class ProductListModel : public Core::ListModel
{
    Q_OBJECT
public:
    explicit ProductListModel(QObject *parent);
    void updateCollections();

signals:
    void errorOccurred(int errorCode, const QString &errorString);
    void toggleProgressIndicator(bool show);

protected:
    QPixmap fetchPixmapAndUpdatePixmapCache(const QString &url) const override;

private:
    void onFetchCollectionsFinished(QNetworkReply *reply);
    void onFetchSingleCollectionFinished(QNetworkReply *reply);
    void fetchCollectionsContents();

    void queueImageForDownload(const QString &url);
    void fetchNextImage();
    void onImageDownloadFinished(QNetworkReply *reply);
    void updateModelIndexesForUrl(const QString &url);

    QQueue<QString> m_pendingCollections;
    QSet<QString> m_pendingImages;
    bool m_isDownloadingImage = false;
};

} // namespace Internal
} // namespace Marketplace

Q_DECLARE_METATYPE(Marketplace::Internal::ProductItem *)
