/****************************************************************************
**
** Copyright (C) 2016 Denis Mingulov.
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "imageviewerplugin.h"
#include "imageviewerfactory.h"
#include "imageviewerconstants.h"

#include <QDebug>

#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/mimetypes/mimedatabase.h>

namespace ImageViewer {
namespace Internal {

///////////////////////////////// ImageViewerPlugin //////////////////////////////////

bool ImageViewerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    Utils::MimeDatabase::addMimeTypes(QLatin1String(":/imageviewer/ImageViewer.mimetypes.xml"));

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
