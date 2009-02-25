/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "fileiconprovider.h"
#include <QtGui/QApplication>
#include <QtGui/QStyle>
#include <QtGui/QPainter>

using namespace Core;

/*!
  \class FileIconProvider

  Provides icons based on file suffixes.

  The class is a singleton: It's instance can be accessed via the static instance() method.
  Plugins can register custom icons via registerIconSuffix(), and retrieve icons via the icon()
  method.
  */

FileIconProvider *FileIconProvider::m_instance = 0;

FileIconProvider::FileIconProvider()
    : m_unknownFileIcon(qApp->style()->standardIcon(QStyle::SP_FileIcon))
{
}

FileIconProvider::~FileIconProvider()
{
    m_instance = 0;
}

/*!
  Returns the icon associated with the file suffix in fileInfo. If there is none,
  the default icon of the operating system is returned.
  */
QIcon FileIconProvider::icon(const QFileInfo &fileInfo)
{
    const QString suffix = fileInfo.suffix();
    QIcon icon = iconForSuffix(suffix);

    if (icon.isNull()) {
        // Get icon from OS and store it in the cache

        // Disabled since for now we'll make sure that all icons fit with our
        // own custom icons by returning an empty one if we don't know it.
#ifdef Q_OS_WIN
        // This is incorrect if the OS does not always return the same icon for the
        // same suffix (Mac OS X), but should speed up the retrieval a lot ...
        icon = m_systemIconProvider.icon(fileInfo);
        if (!suffix.isEmpty())
            registerIconOverlayForSuffix(icon, suffix);
#else
        if (fileInfo.isDir()) {
            icon = m_systemIconProvider.icon(fileInfo);
        } else {
            icon = m_unknownFileIcon;
        }
#endif
    }

    return icon;
}

// Creates a pixmap with baseicon at size and overlayous overlayIcon over it.
QPixmap FileIconProvider::overlayIcon(QStyle::StandardPixmap baseIcon, const QIcon &overlayIcon, const QSize &size) const
{
    QPixmap iconPixmap = qApp->style()->standardIcon(baseIcon).pixmap(size);
    QPainter painter(&iconPixmap);
    painter.drawPixmap(0, 0, overlayIcon.pixmap(size));
    painter.end();
    return iconPixmap;
}

/*!
  Registers an icon for a given suffix, overlaying the system file icon
  */
void FileIconProvider::registerIconOverlayForSuffix(const QIcon &icon, const QString &suffix)
{
    QPixmap fileIconPixmap = overlayIcon(QStyle::SP_FileIcon, icon, QSize(16, 16));
    // delete old icon, if it exists
    QList<QPair<QString,QIcon> >::iterator iter = m_cache.begin();
    for (; iter != m_cache.end(); ++iter) {
        if ((*iter).first == suffix) {
            iter = m_cache.erase(iter);
            break;
        }
    }

    QPair<QString,QIcon> newEntry(suffix, fileIconPixmap);
    m_cache.append(newEntry);
}

/*!
  Returns an icon for the given suffix, or an empty one if none registered.
  */
QIcon FileIconProvider::iconForSuffix(const QString &suffix) const
{
    QIcon icon;
#ifndef Q_OS_WIN // On windows we use the file system icons
    if (suffix.isEmpty())
        return icon;

    QList<QPair<QString,QIcon> >::const_iterator iter = m_cache.constBegin();
    for (; iter != m_cache.constEnd(); ++iter) {
        if ((*iter).first == suffix) {
            icon = (*iter).second;
            break;
        }
    }
#endif
    return icon;
}

/*!
  Returns the sole instance of FileIconProvider.
  */
FileIconProvider *FileIconProvider::instance()
{
    if (!m_instance)
        m_instance = new FileIconProvider;
    return m_instance;
}
