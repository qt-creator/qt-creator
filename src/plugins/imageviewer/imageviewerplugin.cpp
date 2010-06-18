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

#include "imageviewerplugin.h"
#include "imageviewerfactory.h"
#include "imageviewerconstants.h"

#include <QtCore/QDebug>

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
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
    : d_ptr(new ImageViewerPluginPrivate)
{
}

ImageViewerPlugin::~ImageViewerPlugin()
{
}

bool ImageViewerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)

    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/imageviewer/ImageViewer.mimetypes.xml"), errorMessage))
        return false;

    d_ptr->factory = new ImageViewerFactory(this);
    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate;
    aggregate->add(d_ptr->factory);

    addAutoReleasedObject(d_ptr->factory);
    return true;
}

void ImageViewerPlugin::extensionsInitialized()
{
    d_ptr->factory->extensionsInitialized();
}

} // namespace Internal
} // namespace ImageViewer

Q_EXPORT_PLUGIN(ImageViewer::Internal::ImageViewerPlugin)
