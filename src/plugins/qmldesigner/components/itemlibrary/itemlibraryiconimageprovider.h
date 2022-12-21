// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <nodeinstanceview.h>
#include <rewriterview.h>

#include <coreplugin/icore.h>
#include <asynchronousimagecache.h>
#include <imagecache/imagecachecollector.h>
#include <imagecache/imagecacheconnectionmanager.h>
#include <imagecache/imagecachegenerator.h>
#include <imagecache/imagecachestorage.h>
#include <imagecache/timestampprovider.h>

#include <sqlitedatabase.h>

#include <QQuickAsyncImageProvider>

namespace QmlDesigner {

class ItemLibraryIconImageProvider : public QQuickAsyncImageProvider
{
public:
    ItemLibraryIconImageProvider(AsynchronousImageCache &imageCache)
        : m_cache{imageCache}
    {}

    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

private:
    AsynchronousImageCache &m_cache;
};

} // namespace QmlDesigner
