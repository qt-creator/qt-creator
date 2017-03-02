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

#include "fileiconprovider.h"

#include <utils/hostosinfo.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QStyle>
#include <QPainter>
#include <QFileInfo>
#include <QHash>
#include <QDebug>

#include <QFileIconProvider>
#include <QIcon>

using namespace Utils;

/*!
  \class Core::FileIconProvider

  Provides icons based on file suffixes with the ability to overwrite system
  icons for specific subtypes. The underlying QFileIconProvider
  can be used for QFileSystemModel.

  Note: Registering overlay icons currently completely replaces the system
        icon and is therefore not recommended on platforms that have their
        own overlay icon handling (Mac/Windows).

  Plugins can register custom overlay icons via registerIconOverlayForSuffix(), and
  retrieve icons via the icon() function.
  */

// Cache icons in a list of pairs suffix/icon which should be faster than
// hashes for small lists.

namespace Core {
namespace FileIconProvider {

enum { debug = 0 };

class FileIconProviderImplementation : public QFileIconProvider
{
public:
    FileIconProviderImplementation()
        : m_unknownFileIcon(qApp->style()->standardIcon(QStyle::SP_FileIcon))
    {}

    QIcon icon(const QFileInfo &info) const override;
    using QFileIconProvider::icon;

    void registerIconOverlayForFilename(const QIcon &icon, const QString &filename)
    {
        QTC_ASSERT(!icon.isNull() && !filename.isEmpty(), return);

        const QPixmap fileIconPixmap = FileIconProvider::overlayIcon(QStyle::SP_FileIcon, icon, QSize(16, 16));
        m_filenameCache.insert(filename, fileIconPixmap);
    }

    void registerIconOverlayForSuffix(const QIcon &icon, const QString &suffix)
    {
        if (debug)
            qDebug() << "FileIconProvider::registerIconOverlayForSuffix" << suffix;

        QTC_ASSERT(!icon.isNull() && !suffix.isEmpty(), return);

        const QPixmap fileIconPixmap = FileIconProvider::overlayIcon(QStyle::SP_FileIcon, icon, QSize(16, 16));
        // replace old icon, if it exists
        m_suffixCache.insert(suffix, fileIconPixmap);
    }

    void registerIconOverlayForMimeType(const QIcon &icon, const Utils::MimeType &mimeType)
    {
        foreach (const QString &suffix, mimeType.suffixes())
            registerIconOverlayForSuffix(icon, suffix);
    }

    // Mapping of file suffix to icon.
    mutable QHash<QString, QIcon> m_suffixCache;
    QHash<QString, QIcon> m_filenameCache;

    QIcon m_unknownFileIcon;
};

FileIconProviderImplementation *instance()
{
    static FileIconProviderImplementation theInstance;
    return &theInstance;
}

QFileIconProvider *iconProvider()
{
    return instance();
}

QIcon FileIconProviderImplementation::icon(const QFileInfo &fileInfo) const
{
    if (debug)
        qDebug() << "FileIconProvider::icon" << fileInfo.absoluteFilePath();
    // Check for cached overlay icons by file suffix.
    bool isDir = fileInfo.isDir();
    const QString filename = !isDir ? fileInfo.fileName() : QString();
    if (!filename.isEmpty()) {
        auto it = m_filenameCache.constFind(filename);
        if (it != m_filenameCache.constEnd())
            return it.value();
    }
    const QString suffix = !isDir ? fileInfo.suffix() : QString();
    if (!suffix.isEmpty()) {
        auto it = m_suffixCache.constFind(suffix);
        if (it != m_suffixCache.constEnd())
            return it.value();
    }

    // Get icon from OS (and cache it based on suffix!)
    QIcon icon;
    if (HostOsInfo::isWindowsHost() || HostOsInfo::isMacHost())
        icon = QFileIconProvider::icon(fileInfo);
    else // File icons are unknown on linux systems.
        icon = isDir ? QFileIconProvider::icon(fileInfo) : m_unknownFileIcon;
    if (!isDir && !suffix.isEmpty())
        m_suffixCache.insert(suffix, icon);
    return icon;
}

/*!
  Returns the icon associated with the file suffix in fileInfo. If there is none,
  the default icon of the operating system is returned.
  */

QIcon icon(const QFileInfo &info)
{
    return instance()->icon(info);
}

/*!
 * \overload
 */
QIcon icon(QFileIconProvider::IconType type)
{
    return instance()->icon(type);
}

/*!
  Creates a pixmap with baseicon and overlays overlayIcon over it.
  See platform note in class documentation about recommended usage.
  */
QPixmap overlayIcon(const QPixmap &baseIcon, const QIcon &overlayIcon)
{
    QPixmap result = baseIcon;
    QPainter painter(&result);
    overlayIcon.paint(&painter, QRect(QPoint(), result.size() / result.devicePixelRatio()));
    return result;
}

/*!
  Creates a pixmap with baseicon at size and overlays overlayIcon over it.
  See platform note in class documentation about recommended usage.
  */
QPixmap overlayIcon(QStyle::StandardPixmap baseIcon, const QIcon &overlay, const QSize &size)
{
    return overlayIcon(qApp->style()->standardIcon(baseIcon).pixmap(size), overlay);
}

/*!
  Registers an icon for a given suffix, overlaying the system file icon.
  See platform note in class documentation about recommended usage.
  */
void registerIconOverlayForSuffix(const QString &path, const QString &suffix)
{
    instance()->registerIconOverlayForSuffix(QIcon(path), suffix);
}

/*!
  Registers an icon for all the suffixes of a given mime type, overlaying the system file icon.
  */
void registerIconOverlayForMimeType(const QIcon &icon, const QString &mimeType)
{
    instance()->registerIconOverlayForMimeType(icon, Utils::mimeTypeForName(mimeType));
}

/*!
 * \overload
 */
void registerIconOverlayForMimeType(const QString &path, const QString &mimeType)
{
    instance()->registerIconOverlayForMimeType(QIcon(path), Utils::mimeTypeForName(mimeType));
}

void registerIconOverlayForFilename(const QString &path, const QString &filename)
{
    instance()->registerIconOverlayForFilename(QIcon(path), filename);
}

} // namespace FileIconProvider
} // namespace Core
