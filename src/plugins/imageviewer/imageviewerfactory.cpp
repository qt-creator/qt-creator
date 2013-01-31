/**************************************************************************
**
** Copyright (C) 2013 Denis Mingulov.
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

#include "imageviewerfactory.h"
#include "imagevieweractionhandler.h"
#include "imageviewerconstants.h"
#include "imageviewer.h"

#include <QCoreApplication>
#include <QMap>
#include <QImageReader>
#include <QDebug>

namespace ImageViewer {
namespace Internal {

struct ImageViewerFactoryPrivate
{
    QStringList mimeTypes;
    QPointer<ImageViewerActionHandler> actionHandler;
};

ImageViewerFactory::ImageViewerFactory(QObject *parent) :
    Core::IEditorFactory(parent),
    d(new ImageViewerFactoryPrivate)
{
    d->actionHandler = new ImageViewerActionHandler(this);

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
            d->mimeTypes.append(value);
    }
}

ImageViewerFactory::~ImageViewerFactory()
{
    delete d;
}

Core::IEditor *ImageViewerFactory::createEditor(QWidget *parent)
{
    return new ImageViewer(parent);
}

QStringList ImageViewerFactory::mimeTypes() const
{
    return d->mimeTypes;
}

Core::Id ImageViewerFactory::id() const
{
    return Core::Id(Constants::IMAGEVIEWER_ID);
}

QString ImageViewerFactory::displayName() const
{
    return qApp->translate("OpenWith::Editors", Constants::IMAGEVIEWER_DISPLAY_NAME);
}

Core::IDocument *ImageViewerFactory::open(const QString & /*fileName*/)
{
    return 0;
}

void ImageViewerFactory::extensionsInitialized()
{
    d->actionHandler->createActions();
}

} // namespace Internal
} // namespace ImageViewer
