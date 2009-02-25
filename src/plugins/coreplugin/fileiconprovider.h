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

#ifndef FILEICONPROVIDER_H
#define FILEICONPROVIDER_H

#include <coreplugin/core_global.h>

#include <QtCore/QFileInfo>
#include <QtCore/QPair>
#include <QtGui/QFileIconProvider>
#include <QtGui/QIcon>
#include <QtGui/QStyle>

namespace Core {

class CORE_EXPORT FileIconProvider {
public:
    ~FileIconProvider(); // used to clear the cache
    QIcon icon(const QFileInfo &fileInfo);

    QPixmap overlayIcon(QStyle::StandardPixmap baseIcon, const QIcon &overlayIcon, const QSize &size) const;
    void registerIconOverlayForSuffix(const QIcon &icon, const QString &suffix);

    static FileIconProvider *instance();

private:
    QIcon iconForSuffix(const QString &suffix) const;

    // mapping of file ending to icon
    // TODO: Check if this is really faster than a QHash
    mutable QList<QPair<QString, QIcon> > m_cache;

    QFileIconProvider m_systemIconProvider;
    QIcon m_unknownFileIcon;

    // singleton pattern
    FileIconProvider();
    static FileIconProvider *m_instance;
};

} // namespace Core

#endif // FILEICONPROVIDER_H
