/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

    QIcon icon(const QFileInfo &info);
    using QFileIconProvider::icon;

    void registerIconOverlayForSuffix(const QIcon &icon, const QString &suffix)
    {
        if (debug)
            qDebug() << "FileIconProvider::registerIconOverlayForSuffix" << suffix;

        QTC_ASSERT(!icon.isNull() && !suffix.isEmpty(), return);

        const QPixmap fileIconPixmap = FileIconProvider::overlayIcon(QStyle::SP_FileIcon, icon, QSize(16, 16));
        // replace old icon, if it exists
        m_cache.insert(suffix, fileIconPixmap);
    }

    void registerIconOverlayForMimeType(const QIcon &icon, const Utils::MimeType &mimeType)
    {
        foreach (const QString &suffix, mimeType.suffixes())
            registerIconOverlayForSuffix(icon, suffix);
    }

    // Mapping of file suffix to icon.
    QHash<QString, QIcon> m_cache;

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

QIcon FileIconProviderImplementation::icon(const QFileInfo &fileInfo)
{
    if (debug)
        qDebug() << "FileIconProvider::icon" << fileInfo.absoluteFilePath();
    // Check for cached overlay icons by file suffix.
    bool isDir = fileInfo.isDir();
    QString suffix = !isDir ? fileInfo.suffix() : QString();
    if (!m_cache.isEmpty() && !isDir && !suffix.isEmpty()) {
        if (m_cache.contains(suffix))
            return m_cache.value(suffix);
    }
    // Get icon from OS.
    QIcon icon;
    if (HostOsInfo::isWindowsHost() || HostOsInfo::isMacHost())
        icon = QFileIconProvider::icon(fileInfo);
    else // File icons are unknown on linux systems.
        icon = isDir ? QFileIconProvider::icon(fileInfo) : m_unknownFileIcon;
    if (!isDir && !suffix.isEmpty())
        m_cache.insert(suffix, icon);
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
  Creates a pixmap with baseicon at size and overlays overlayIcon over it.
  See platform note in class documentation about recommended usage.
  */
QPixmap overlayIcon(QStyle::StandardPixmap baseIcon, const QIcon &overlayIcon, const QSize &size)
{
    QPixmap iconPixmap = qApp->style()->standardIcon(baseIcon).pixmap(size);
    QPainter painter(&iconPixmap);
    painter.drawPixmap(0, 0, overlayIcon.pixmap(size));
    painter.end();
    return iconPixmap;
}

/*!
  Registers an icon for a given suffix, overlaying the system file icon.
  See platform note in class documentation about recommended usage.
  */
void registerIconOverlayForSuffix(const char *path, const char *suffix)
{
    instance()->registerIconOverlayForSuffix(QIcon(QLatin1String(path)), QLatin1String(suffix));
}

/*!
  Registers an icon for all the suffixes of a given mime type, overlaying the system file icon.
  */
void registerIconOverlayForMimeType(const QIcon &icon, const char *mimeType)
{
    Utils::MimeDatabase mdb;
    instance()->registerIconOverlayForMimeType(icon,
                                               mdb.mimeTypeForName(QString::fromLatin1(mimeType)));
}

/*!
 * \overload
 */
void registerIconOverlayForMimeType(const char *path, const char *mimeType)
{
    Utils::MimeDatabase mdb;
    instance()->registerIconOverlayForMimeType(QIcon(QLatin1String(path)),
                                               mdb.mimeTypeForName(QString::fromLatin1(mimeType)));
}

} // namespace FileIconProvider
} // namespace Core
