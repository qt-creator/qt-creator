/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef FILEICONPROVIDER_H
#define FILEICONPROVIDER_H

#include <coreplugin/core_global.h>

#include <QStyle>
#include <QFileIconProvider>

namespace Core {

namespace FileIconProvider {

// Access to the single instance
CORE_EXPORT QFileIconProvider *iconProvider();

// Access to individual items
CORE_EXPORT QIcon icon(const QFileInfo &info);
CORE_EXPORT QIcon icon(QFileIconProvider::IconType type);

// Register additional overlay icons
CORE_EXPORT QPixmap overlayIcon(const QPixmap &baseIcon, const QIcon &overlayIcon);
CORE_EXPORT QPixmap overlayIcon(QStyle::StandardPixmap baseIcon, const QIcon &overlayIcon, const QSize &size);
CORE_EXPORT void registerIconOverlayForSuffix(const char *path, const char *suffix);
CORE_EXPORT void registerIconOverlayForMimeType(const char *path, const char *mimeType);
CORE_EXPORT void registerIconOverlayForMimeType(const QIcon &icon, const char *mimeType);

} // namespace FileIconProvider
} // namespace Core

#endif // FILEICONPROVIDER_H
