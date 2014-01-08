/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FILEICONPROVIDER_H
#define FILEICONPROVIDER_H

#include <coreplugin/core_global.h>

#include <QStyle>
#include <QFileIconProvider>

namespace Core {

class MimeType;

namespace FileIconProvider {

// Access to the single instance
CORE_EXPORT QFileIconProvider *iconProvider();

// Access to individual items
CORE_EXPORT QIcon icon(const QFileInfo &info);
CORE_EXPORT QIcon icon(QFileIconProvider::IconType type);

// Register additional overlay icons
CORE_EXPORT QPixmap overlayIcon(QStyle::StandardPixmap baseIcon, const QIcon &overlayIcon, const QSize &size);
CORE_EXPORT void registerIconOverlayForSuffix(const char *path, const char *suffix);
CORE_EXPORT void registerIconOverlayForMimeType(const char *path, const char *mimeType);
CORE_EXPORT void registerIconOverlayForMimeType(const QIcon &icon, const char *mimeType);

} // namespace FileIconProvider
} // namespace Core

#endif // FILEICONPROVIDER_H
