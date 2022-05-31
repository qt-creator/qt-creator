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

#pragma once

#include "../utils_global.h"

#ifdef QT_GUI_LIB

#include <QFileIconProvider>
#include <QStyle>

namespace Utils {
class FilePath;

namespace FileIconProvider {

// Access to the single instance
QTCREATOR_UTILS_EXPORT QFileIconProvider *iconProvider();

// Access to individual items
QTCREATOR_UTILS_EXPORT QIcon icon(const Utils::FilePath &filePath);
QTCREATOR_UTILS_EXPORT QIcon icon(QFileIconProvider::IconType type);

// Register additional overlay icons
QTCREATOR_UTILS_EXPORT QPixmap overlayIcon(const QPixmap &baseIcon, const QIcon &overlayIcon);
QTCREATOR_UTILS_EXPORT QPixmap overlayIcon(QStyle::StandardPixmap baseIcon,
                                           const QIcon &overlayIcon,
                                           const QSize &size);
QTCREATOR_UTILS_EXPORT void registerIconOverlayForSuffix(const QString &path, const QString &suffix);
QTCREATOR_UTILS_EXPORT void registerIconOverlayForFilename(const QString &path,
                                                           const QString &filename);
QTCREATOR_UTILS_EXPORT void registerIconOverlayForMimeType(const QString &path,
                                                           const QString &mimeType);
QTCREATOR_UTILS_EXPORT void registerIconOverlayForMimeType(const QIcon &icon,
                                                           const QString &mimeType);

QTCREATOR_UTILS_EXPORT QIcon directoryIcon(const QString &overlay);

} // namespace FileIconProvider
} // namespace Utils

#endif
