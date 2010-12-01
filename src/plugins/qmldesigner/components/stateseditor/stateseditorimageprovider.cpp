/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "stateseditorimageprovider.h"
#include "stateseditorview.h"

#include <QtDebug>

namespace QmlDesigner {

namespace Internal {

StatesEditorImageProvider::StatesEditorImageProvider() :
        QDeclarativeImageProvider(QDeclarativeImageProvider::Image)
{
}

QImage StatesEditorImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QSize newSize = requestedSize;

    if (newSize.isEmpty())
        newSize = QSize (100, 100);

    QImage image = m_imageHash.value(id, QImage(newSize, QImage::Format_ARGB32));
    image.fill(0xFFFFFFFF);

    *size = image.size();

    return image;
}

void StatesEditorImageProvider::setImage(const QString &id, const QImage &image)
{
    m_imageHash.insert(id, image);
}

void StatesEditorImageProvider::removeImage(const QString &id)
{
    m_imageHash.remove(id);
}

}

}

