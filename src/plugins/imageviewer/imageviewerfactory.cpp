/**************************************************************************
**
** Copyright (C) 2015 Denis Mingulov.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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

    const QList<QByteArray> supportedMimeTypes = QImageReader::supportedMimeTypes();
    foreach (const QByteArray &format, supportedMimeTypes)
        addMimeType(format.constData());

#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0)) && !QT_NO_SVGRENDERER
    // Workaround for https://codereview.qt-project.org/108693
    addMimeType("image/svg+xml");
#endif
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
