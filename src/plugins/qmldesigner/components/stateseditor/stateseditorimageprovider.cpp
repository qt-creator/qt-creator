/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "stateseditorimageprovider.h"
#include "stateseditorview.h"
#include "nodeinstanceview.h"

#include <QtDebug>

namespace QmlDesigner {

namespace Internal {

StatesEditorImageProvider::StatesEditorImageProvider()
    : QDeclarativeImageProvider(QDeclarativeImageProvider::Image)
{
}

QImage StatesEditorImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    if (m_nodeInstanceView.isNull())
        return QImage();

    QSize newSize = requestedSize;

    if (newSize.isEmpty())
        newSize = QSize (100, 100);

    QString imageId = id.split("-").first();
    QImage image;

    if (imageId == "baseState") {
        image = m_nodeInstanceView->statePreviewImage(m_nodeInstanceView->rootModelNode());
    } else {
        bool canBeConverted;
        int instanceId = imageId.toInt(&canBeConverted);
        if (canBeConverted && m_nodeInstanceView->hasModelNodeForInternalId(instanceId)) {
                image = m_nodeInstanceView->statePreviewImage(m_nodeInstanceView->modelNodeForInternalId(instanceId));
        } else {
            image = QImage(newSize, QImage::Format_ARGB32);
            image.fill(0xFFFFFFFF);
        }
    }

    *size = image.size();

    return image;
}

void StatesEditorImageProvider::setNodeInstanceView(NodeInstanceView *nodeInstanceView)
{
    m_nodeInstanceView = nodeInstanceView;
}

}

}

