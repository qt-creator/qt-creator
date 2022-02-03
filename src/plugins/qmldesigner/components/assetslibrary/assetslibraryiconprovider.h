/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <synchronousimagecache.h>

#include <QQuickImageProvider>

namespace QmlDesigner {

class AssetsLibraryIconProvider : public QQuickImageProvider
{
public:
    AssetsLibraryIconProvider(SynchronousImageCache &fontImageCache);

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    QPixmap generateFontIcons(const QString &filePath, const QSize &requestedSize) const;

    SynchronousImageCache &m_fontImageCache;

    // Generated icon sizes should contain all ItemLibraryResourceView needed icon sizes, and their
    // x2 versions for HDPI sceens
    std::vector<QSize> iconSizes = {{128, 128}, // Drag
                                    {96, 96},   // list @2x
                                    {48, 48}};  // list
};

} // namespace QmlDesigner
