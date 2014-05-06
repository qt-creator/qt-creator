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

///////////////////////////////// ImageViewerPlugin //////////////////////////////////

bool ImageViewerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)

    if (!Core::MimeDatabase::addMimeTypes(QLatin1String(":/imageviewer/ImageViewer.mimetypes.xml"), errorMessage))
        return false;

    m_factory = new ImageViewerFactory(this);
    addAutoReleasedObject(m_factory);
    return true;
}

void ImageViewerPlugin::extensionsInitialized()
{
    m_factory->extensionsInitialized();
}

} // namespace Internal
} // namespace ImageViewer

Q_EXPORT_PLUGIN(ImageViewer::Internal::ImageViewerPlugin)
