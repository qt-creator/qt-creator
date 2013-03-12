/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "fileiconprovider.h"
#include "mimedatabase.h"

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QStyle>
#include <QPainter>
#include <QFileInfo>
#include <QPair>
#include <QDebug>

#include <QFileIconProvider>
#include <QIcon>

using namespace Utils;

/*!
  \class Core::FileIconProvider

  Provides icons based on file suffixes with the ability to overwrite system
  icons for specific subtypes. Implements the QFileIconProvider interface
  and can therefore be used for QFileSystemModel.

  Note: Registering overlay icons currently completely replaces the system
        icon and is therefore not recommended on platforms that have their
        own overlay icon handling (Mac/Windows).

  The class is a singleton: It's instance can be accessed via the static instance() method.
  Plugins can register custom overlay icons via registerIconOverlayForSuffix(), and
  retrieve icons via the icon() method.
  The instance is explicitly deleted by the core plugin for destruction order reasons.
  */

// Cache icons in a list of pairs suffix/icon which should be faster than
// hashes for small lists.

enum { debug = 0 };

typedef QPair<QString, QIcon> StringIconPair;
typedef QList<StringIconPair> StringIconPairList;

// Helper to find an icon by suffix in a list of pairs for const/non-const-iterators.

template <class StringIconPairListIterator>
inline StringIconPairListIterator
findBySuffix(const QString &suffix,
             StringIconPairListIterator iter,
             const StringIconPairListIterator &end)
{
    for (; iter != end; ++iter)
        if ((*iter).first == suffix)
            return iter;
    return end;
}

namespace Core {

struct FileIconProviderPrivate {
    FileIconProviderPrivate();

    // Mapping of file suffix to icon.
    StringIconPairList m_cache;

    QIcon m_unknownFileIcon;

    // singleton pattern
    static FileIconProvider *m_instance;
};

FileIconProviderPrivate::FileIconProviderPrivate() :
    m_unknownFileIcon(qApp->style()->standardIcon(QStyle::SP_FileIcon))
{
}

FileIconProvider *FileIconProviderPrivate::m_instance = 0;

// FileIconProvider

FileIconProvider::FileIconProvider() :
    d(new FileIconProviderPrivate)
{
    FileIconProviderPrivate::m_instance = this;
}

FileIconProvider::~FileIconProvider()
{
    FileIconProviderPrivate::m_instance = 0;
    delete d;
}

/*!
  Returns the icon associated with the file suffix in fileInfo. If there is none,
  the default icon of the operating system is returned.
  */

QIcon FileIconProvider::icon(const QFileInfo &fileInfo) const
{
    typedef StringIconPairList::const_iterator CacheConstIterator;

    if (debug)
        qDebug() << "FileIconProvider::icon" << fileInfo.absoluteFilePath();
    // Check for cached overlay icons by file suffix.
    if (!d->m_cache.isEmpty() && !fileInfo.isDir()) {
        const QString suffix = fileInfo.suffix();
        if (!suffix.isEmpty()) {
            const CacheConstIterator it = findBySuffix(suffix, d->m_cache.constBegin(), d->m_cache.constEnd());
            if (it != d->m_cache.constEnd())
                return (*it).second;
        }
    }
    // Get icon from OS.
    if (HostOsInfo::isWindowsHost() || HostOsInfo::isMacHost())
        return QFileIconProvider::icon(fileInfo);

    // File icons are unknown on linux systems.
    return (fileInfo.isDir()) ?
           QFileIconProvider::icon(fileInfo) :
           d->m_unknownFileIcon;
}

/*!
  Creates a pixmap with baseicon at size and overlays overlayIcon over it.
  See platform note in class documentation about recommended usage.
  */
QPixmap FileIconProvider::overlayIcon(QStyle::StandardPixmap baseIcon, const QIcon &overlayIcon, const QSize &size)
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
void FileIconProvider::registerIconOverlayForSuffix(const QIcon &icon,
                                                    const QString &suffix)
{
    typedef StringIconPairList::iterator CacheIterator;

    if (debug)
        qDebug() << "FileIconProvider::registerIconOverlayForSuffix" << suffix;

    QTC_ASSERT(!icon.isNull() && !suffix.isEmpty(), return);

    const QPixmap fileIconPixmap = overlayIcon(QStyle::SP_FileIcon, icon, QSize(16, 16));
    // replace old icon, if it exists
    const CacheIterator it = findBySuffix(suffix, d->m_cache.begin(), d->m_cache.end());
    if (it == d->m_cache.end())
        d->m_cache.append(StringIconPair(suffix, fileIconPixmap));
    else
       (*it).second = fileIconPixmap;
}

/*!
  Registers an icon for all the suffixes of a given mime type, overlaying the system file icon.
  */
void FileIconProvider::registerIconOverlayForMimeType(const QIcon &icon, const MimeType &mimeType)
{
    foreach (const QString &suffix, mimeType.suffixes())
        registerIconOverlayForSuffix(icon, suffix);
}

/*!
  Returns the sole instance of FileIconProvider.
  */

FileIconProvider *FileIconProvider::instance()
{
    if (!FileIconProviderPrivate::m_instance)
        FileIconProviderPrivate::m_instance = new FileIconProvider;
    return FileIconProviderPrivate::m_instance;
}

} // namespace core
