/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "imageviewerplugin.h"
#include "imageviewerfactory.h"
#include "imageviewerconstants.h"

#include <QDebug>

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/id.h>
#include <extensionsystem/pluginmanager.h>

namespace ImageViewer {
namespace Internal {

///////////////////////////////// ImageViewerPluginPrivate //////////////////////////////////
struct ImageViewerPluginPrivate
{
    QPointer<ImageViewerFactory> factory;
};

///////////////////////////////// ImageViewerPlugin //////////////////////////////////

ImageViewerPlugin::ImageViewerPlugin()
    : d(new ImageViewerPluginPrivate)
{
}

ImageViewerPlugin::~ImageViewerPlugin()
{
    delete d;
}

bool ImageViewerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)

    if (!Core::ICore::mimeDatabase()->addMimeTypes(QLatin1String(":/imageviewer/ImageViewer.mimetypes.xml"), errorMessage))
        return false;

    d->factory = new ImageViewerFactory(this);
    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate;
    aggregate->add(d->factory);

    addAutoReleasedObject(d->factory);
    return true;
}

void ImageViewerPlugin::extensionsInitialized()
{
    d->factory->extensionsInitialized();
}

} // namespace Internal
} // namespace ImageViewer

Q_EXPORT_PLUGIN(ImageViewer::Internal::ImageViewerPlugin)
