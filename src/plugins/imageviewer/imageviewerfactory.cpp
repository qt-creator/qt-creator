/**************************************************************************
**
** Copyright (C) 2014 Denis Mingulov.
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

#include "imageviewerfactory.h"
#include "imageviewerconstants.h"
#include "imageviewer.h"

#include <QCoreApplication>
#include <QMap>
#include <QImageReader>
#include <QDebug>

namespace ImageViewer {
namespace Internal {

ImageViewerFactory::ImageViewerFactory(QObject *parent) :
    Core::IEditorFactory(parent)
{
    setId(Constants::IMAGEVIEWER_ID);
    setDisplayName(qApp->translate("OpenWith::Editors", Constants::IMAGEVIEWER_DISPLAY_NAME));

    QMap<QByteArray, const char *> possibleMimeTypes;
    possibleMimeTypes.insert("bmp", "image/bmp");
    possibleMimeTypes.insert("gif", "image/gif");
    possibleMimeTypes.insert("ico", "image/x-icon");
    possibleMimeTypes.insert("jpeg","image/jpeg");
    possibleMimeTypes.insert("jpg", "image/jpeg");
    possibleMimeTypes.insert("mng", "video/x-mng");
    possibleMimeTypes.insert("pbm", "image/x-portable-bitmap");
    possibleMimeTypes.insert("pgm", "image/x-portable-graymap");
    possibleMimeTypes.insert("png", "image/png");
    possibleMimeTypes.insert("ppm", "image/x-portable-pixmap");
    possibleMimeTypes.insert("svg", "image/svg+xml");
    possibleMimeTypes.insert("tif", "image/tiff");
    possibleMimeTypes.insert("tiff","image/tiff");
    possibleMimeTypes.insert("xbm", "image/xbm");
    possibleMimeTypes.insert("xpm", "image/xpm");

    QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
    foreach (const QByteArray &format, supportedFormats) {
        const char *value = possibleMimeTypes.value(format);
        if (value)
            addMimeType(value);
    }
}

Core::IEditor *ImageViewerFactory::createEditor()
{
    return new ImageViewer();
}

void ImageViewerFactory::extensionsInitialized()
{
    m_actionHandler.createActions();
}

} // namespace Internal
} // namespace ImageViewer
