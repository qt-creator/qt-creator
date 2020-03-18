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
#include <utils/optional.h>
#include <utils/qtcassert.h>
#include <utils/variant.h>

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
  \namespace Core::FileIconProvider
  \inmodule QtCreator
  \brief Provides functions for registering custom overlay icons for system
  icons.

  Provides icons based on file suffixes with the ability to overwrite system
  icons for specific subtypes. The underlying QFileIconProvider
  can be used for QFileSystemModel.

  \note Registering overlay icons currently completely replaces the system
        icon and is therefore not recommended on platforms that have their
        own overlay icon handling (\macOS and Windows).

  Plugins can register custom overlay icons via registerIconOverlayForSuffix(), and
  retrieve icons via the icon() function.
  */

using Item = Utils::variant<QIcon, QString>; // icon or filename for the icon

namespace Core {
namespace FileIconProvider {

enum { debug = 0 };

static Utils::optional<QIcon> getIcon(QHash<QString, Item> &cache, const QString &key)
{
    auto it = cache.constFind(key);
    if (it == cache.constEnd())
        return {};
    if (const QIcon *icon = Utils::get_if<QIcon>(&*it))
        return *icon;
    // need to create icon from file name first
    const QString *fileName = Utils::get_if<QString>(&*it);
    QTC_ASSERT(fileName, return {});
    const QIcon icon = QIcon(
        FileIconProvider::overlayIcon(QStyle::SP_FileIcon, QIcon(*fileName), QSize(16, 16)));
    cache.insert(key, icon);
    return icon;
}

class FileIconProviderImplementation : public QFileIconProvider
{
public:
    FileIconProviderImplementation()
    {}

    QIcon icon(const QFileInfo &info) const override;
    using QFileIconProvider::icon;

    void registerIconOverlayForFilename(const QString &iconFilePath, const QString &filename)
    {
        m_filenameCache.insert(filename, iconFilePath);
    }

    void registerIconOverlayForSuffix(const QString &iconFilePath, const QString &suffix)
    {
        m_suffixCache.insert(suffix, iconFilePath);
    }

    void registerIconOverlayForMimeType(const QIcon &icon, const Utils::MimeType &mimeType)
    {
        const QStringList suffixes = mimeType.suffixes();
        for (const QString &suffix : suffixes) {
            QTC_ASSERT(!icon.isNull() && !suffix.isEmpty(), return );

            const QPixmap fileIconPixmap = FileIconProvider::overlayIcon(QStyle::SP_FileIcon,
                                                                         icon,
                                                                         QSize(16, 16));
            // replace old icon, if it exists
            m_suffixCache.insert(suffix, fileIconPixmap);
        }
    }

    void registerIconOverlayForMimeType(const QString &iconFilePath, const Utils::MimeType &mimeType)
    {
        foreach (const QString &suffix, mimeType.suffixes())
            registerIconOverlayForSuffix(iconFilePath, suffix);
    }

    // Mapping of file suffix to icon.
    mutable QHash<QString, Item> m_suffixCache;
    mutable QHash<QString, Item> m_filenameCache;
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
        const Utils::optional<QIcon> icon = getIcon(m_filenameCache, filename);
        if (icon)
            return *icon;
    }
    const QString suffix = !isDir ? fileInfo.suffix() : QString();
    if (!suffix.isEmpty()) {
        const Utils::optional<QIcon> icon = getIcon(m_suffixCache, suffix);
        if (icon)
            return *icon;
    }

    // Get icon from OS (and cache it based on suffix!)
    QIcon icon;
    if (HostOsInfo::isWindowsHost() || HostOsInfo::isMacHost()) {
        icon = QFileIconProvider::icon(fileInfo);
    } else { // File icons are unknown on linux systems.
        static const QIcon unknownFileIcon(QApplication::style()->standardIcon(QStyle::SP_FileIcon));
        icon = isDir ? QFileIconProvider::icon(fileInfo) : unknownFileIcon;
    }
    if (!isDir && !suffix.isEmpty())
        m_suffixCache.insert(suffix, icon);
    return icon;
}

/*!
  Returns the icon associated with the file suffix in \a info. If there is none,
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
  Creates a pixmap with \a baseIcon and lays \a overlayIcon over it.
  */
QPixmap overlayIcon(const QPixmap &baseIcon, const QIcon &overlayIcon)
{
    QPixmap result = baseIcon;
    QPainter painter(&result);
    overlayIcon.paint(&painter, QRect(QPoint(), result.size() / result.devicePixelRatio()));
    return result;
}

/*!
  Creates a pixmap with \a baseIcon at \a size and \a overlay.
  */
QPixmap overlayIcon(QStyle::StandardPixmap baseIcon, const QIcon &overlay, const QSize &size)
{
    return overlayIcon(QApplication::style()->standardIcon(baseIcon).pixmap(size), overlay);
}

/*!
  Registers an icon at \a path for a given \a suffix, overlaying the system
  file icon.
 */
void registerIconOverlayForSuffix(const QString &path, const QString &suffix)
{
    instance()->registerIconOverlayForSuffix(path, suffix);
}

/*!
  Registers \a icon for all the suffixes of a the mime type \a mimeType,
  overlaying the system file icon.
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
    instance()->registerIconOverlayForMimeType(path, Utils::mimeTypeForName(mimeType));
}

void registerIconOverlayForFilename(const QString &path, const QString &filename)
{
    instance()->registerIconOverlayForFilename(path, filename);
}

// Return a standard directory icon with the specified overlay:
QIcon directoryIcon(const QString &overlay)
{
    // Overlay the SP_DirIcon with the custom icons
    const QSize desiredSize = QSize(16, 16);

    const QPixmap dirPixmap = QApplication::style()->standardIcon(QStyle::SP_DirIcon).pixmap(desiredSize);
    const QIcon overlayIcon(overlay);
    QIcon result;
    result.addPixmap(Core::FileIconProvider::overlayIcon(dirPixmap, overlayIcon));
    return result;
}

} // namespace FileIconProvider
} // namespace Core
