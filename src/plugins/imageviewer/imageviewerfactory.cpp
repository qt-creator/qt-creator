/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "imageviewerfactory.h"
#include "imageviewerconstants.h"
#include "imageviewer.h"

#include <QtCore/QMap>
#include <QtGui/QImageReader>
#include <QtCore/QtDebug>

namespace ImageViewer {
namespace Internal {

ImageViewerFactory::ImageViewerFactory(QObject *parent) :
    Core::IEditorFactory(parent)
{
    QMap<QByteArray, QString> possibleMimeTypes;
    possibleMimeTypes.insert("bmp", QLatin1String("image/bmp"));
    possibleMimeTypes.insert("gif", QLatin1String("image/gif"));
    possibleMimeTypes.insert("ico", QLatin1String("image/x-icon"));
    possibleMimeTypes.insert("jpeg", QLatin1String("image/jpeg"));
    possibleMimeTypes.insert("jpg", QLatin1String("image/jpeg"));
    possibleMimeTypes.insert("mng", QLatin1String("video/x-mng"));
    possibleMimeTypes.insert("pbm", QLatin1String("image/x-portable-bitmap"));
    possibleMimeTypes.insert("pgm", QLatin1String("image/x-portable-graymap"));
    possibleMimeTypes.insert("png", QLatin1String("image/png"));
    possibleMimeTypes.insert("ppm", QLatin1String("image/x-portable-pixmap"));
    possibleMimeTypes.insert("svg", QLatin1String("image/svg+xml"));
    possibleMimeTypes.insert("tif", QLatin1String("image/tiff"));
    possibleMimeTypes.insert("tiff", QLatin1String("image/tiff"));
    possibleMimeTypes.insert("xbm", QLatin1String("image/xbm"));
    possibleMimeTypes.insert("xpm", QLatin1String("image/xpm"));

    QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
    foreach (const QByteArray &format, supportedFormats) {
        const QString &value = possibleMimeTypes.value(format);
        if (!value.isEmpty())
            m_mimeTypes.append(value);
    }
}

Core::IEditor *ImageViewerFactory::createEditor(QWidget *parent)
{
    return new ImageViewer(parent);
}

QStringList ImageViewerFactory::mimeTypes() const
{
    return m_mimeTypes;
}

QString ImageViewerFactory::id() const
{
    return QLatin1String(Constants::IMAGEVIEWER_ID);
}

QString ImageViewerFactory::displayName() const
{
    return tr(Constants::IMAGEVIEWER_DISPLAY_NAME);
}

Core::IFile *ImageViewerFactory::open(const QString & /*fileName*/)
{
    return 0;
}

} // namespace Internal
} // namespace ImageViewer
