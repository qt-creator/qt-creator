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
