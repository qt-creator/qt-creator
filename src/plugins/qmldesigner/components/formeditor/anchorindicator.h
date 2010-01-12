/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef ANCHORINDICATOR_H
#define ANCHORINDICATOR_H

#include "anchorcontroller.h"
#include <QList>
#include <QHash>

namespace QmlDesigner {

class AnchorIndicator
{
public:
    AnchorIndicator(LayerItem *layerItem);
    ~AnchorIndicator();

    void show();
    void hide();
    void clear();

    void setItems(const QList<FormEditorItem*> &itemList);

    void updateItems(const QList<FormEditorItem*> &itemList);
    void updateTargetPoint(FormEditorItem *item, AnchorLine::Type anchorLine, const QPointF &targetPoint);

    void clearHighlight();
    void highlight(FormEditorItem *item, AnchorLine::Type anchorLine);

private:
    QHash<FormEditorItem*, AnchorController> m_itemControllerHash;
    LayerItem *m_layerItem;
};

} // namespace QmlDesigner

#endif // ANCHORINDICATOR_H
