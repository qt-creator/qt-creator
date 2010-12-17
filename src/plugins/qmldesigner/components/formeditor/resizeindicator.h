/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef RESIZEINDICATOR_H
#define RESIZEINDICATOR_H

#include <QHash>
#include <QPair>

#include "resizecontroller.h"

class QGraphicsRectItem;

namespace QmlDesigner {

class FormEditorItem;
class LayerItem;

class ResizeIndicator
{
public:
    enum Orientation {
        Top = 1,
        Right = 2,
        Bottom = 4,
        Left = 8
    };

    ResizeIndicator(LayerItem *layerItem);
    ~ResizeIndicator();

    void show();
    void hide();

    void clear();

    void setItems(const QList<FormEditorItem*> &itemList);
    void updateItems(const QList<FormEditorItem*> &itemList);

//
//    QPair<FormEditorItem*,Orientation> pick(QGraphicsRectItem* pickedItem) const;
//
//    void show();
//    void hide();



private:
    QHash<FormEditorItem*, ResizeController> m_itemControllerHash;

    LayerItem *m_layerItem;
};

}
#endif // SCALEINDICATOR_H
