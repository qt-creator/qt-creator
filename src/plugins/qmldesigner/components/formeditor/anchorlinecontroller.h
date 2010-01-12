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

#ifndef ANCHORLINECONTROLLER_H
#define ANCHORLINECONTROLLER_H

#include <QWeakPointer>
#include <QSharedPointer>
#include <qmlanchors.h>

namespace QmlDesigner {

class FormEditorItem;
class LayerItem;
class AnchorLineHandleItem;

class AnchorLineControllerData
{
public:
    AnchorLineControllerData(LayerItem *layerItem,
                         FormEditorItem *formEditorItem);
    AnchorLineControllerData(const AnchorLineControllerData &other);
    ~AnchorLineControllerData();

    QWeakPointer<LayerItem> layerItem;
    FormEditorItem *formEditorItem;

    AnchorLineHandleItem *topItem;
    AnchorLineHandleItem *leftItem;
    AnchorLineHandleItem *rightItem;
    AnchorLineHandleItem *bottomItem;
};


class AnchorLineController
{
 public:
    AnchorLineController();
    AnchorLineController(LayerItem *layerItem, FormEditorItem *formEditorItem);
    AnchorLineController(const QSharedPointer<AnchorLineControllerData> &data);

    void show(AnchorLine::Type anchorLineMask);
    void hide();

    void updatePosition();

    bool isValid() const;

    QWeakPointer<AnchorLineControllerData> weakPointer() const;


    FormEditorItem *formEditorItem() const;

    bool isTopHandle(const AnchorLineHandleItem *handle) const;
    bool isLeftHandle(const AnchorLineHandleItem *handle) const;
    bool isRightHandle(const AnchorLineHandleItem *handle) const;
    bool isBottomHandle(const AnchorLineHandleItem *handle) const;

    void clearHighlight();

private:
    QSharedPointer<AnchorLineControllerData> m_data;
};

} // namespace QmlDesigner

#endif // ANCHORLINECONTROLLER_H
