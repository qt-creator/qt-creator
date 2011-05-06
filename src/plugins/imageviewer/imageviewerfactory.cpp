/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "imageviewerfactory.h"
#include "imagevieweractionhandler.h"
#include "imageviewerconstants.h"
#include "imageviewer.h"

#include <QtCore/QMap>
#include <QtGui/QImageReader>
#include <QtCore/QtDebug>

namespace ImageViewer {
namespace Internal {

struct ImageViewerFactoryPrivate
{
    QStringList mimeTypes;
    QPointer<ImageViewerActionHandler> actionHandler;
};

ImageViewerFactory::ImageViewerFactory(QObject *parent) :
    Core::IEditorFactory(parent),
    d_ptr(new ImageViewerFactoryPrivate)
{
    d_ptr->actionHandler = new ImageViewerActionHandler(this);

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
            d_ptr->mimeTypes.append(value);
    }
}

ImageViewerFactory::~ImageViewerFactory()
{
}

Core::IEditor *ImageViewerFactory::createEditor(QWidget *parent)
{
    return new ImageViewer(parent);
}

QStringList ImageViewerFactory::mimeTypes() const
{
    return d_ptr->mimeTypes;
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

void ImageViewerFactory::extensionsInitialized()
{
    d_ptr->actionHandler->createActions();
}

} // namespace Internal
} // namespace ImageViewer
