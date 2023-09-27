// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

QTCREATOR_UTILS_EXPORT void registerIconForMimeType(const QIcon &icon, const QString &mimeType);

} // namespace FileIconProvider
} // namespace Utils

#endif
