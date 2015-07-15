/****************************************************************************
**
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
#include "stateseditorimageprovider.h"
#include "nodeinstanceview.h"

#include <QDebug>

namespace QmlDesigner {

namespace Internal {

StatesEditorImageProvider::StatesEditorImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage StatesEditorImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QImage image;

    bool nodeInstanceViewIsDetached = m_nodeInstanceView.isNull() || !m_nodeInstanceView->model();
    if (!nodeInstanceViewIsDetached) {
        QString imageId = id.split(QLatin1Char('-')).first();
        if (imageId == QLatin1String("baseState")) {
            image = m_nodeInstanceView->statePreviewImage(m_nodeInstanceView->rootModelNode());
        } else {
            bool canBeConverted;
            int instanceId = imageId.toInt(&canBeConverted);
            if (canBeConverted && m_nodeInstanceView->hasModelNodeForInternalId(instanceId)) {
                image = m_nodeInstanceView->statePreviewImage(m_nodeInstanceView->modelNodeForInternalId(instanceId));
            }
        }
    }

    if (image.isNull()) {
        //creating white QImage
        QSize newSize = requestedSize;
        if (newSize.isEmpty())
            newSize = QSize (100, 100);

        QImage image(newSize, QImage::Format_ARGB32);
        image.fill(0xFFFFFFFF);
        return image;
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

