// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemutilfunctions.h"

#include <QRectF>
#include <QDebug>

namespace QmlDesigner {

class FormEditorItem;

QRectF boundingRectForItemList(const QList<FormEditorItem*> itemList)
{
    QRectF boundingRect;

    for (FormEditorItem *item : itemList)
    {
        boundingRect = boundingRect.united(item->mapToScene(item->qmlItemNode().instanceBoundingRect()).boundingRect());
    }

    return boundingRect;
}

}
