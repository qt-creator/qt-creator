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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef QMLDESIGNER_ANCHORINDICATOR_H
#define QMLDESIGNER_ANCHORINDICATOR_H

#include <QPointer>
#include "layeritem.h"
#include "formeditoritem.h"


namespace QmlDesigner {

class FormEditorItem;
class AnchorIndicatorGraphicsItem;

class AnchorIndicator
{
public:
    AnchorIndicator(LayerItem *layerItem);
    AnchorIndicator();
    ~AnchorIndicator();

    void show();
    void hide();

    void clear();

    void setItems(const QList<FormEditorItem*> &itemList);
    void updateItems(const QList<FormEditorItem*> &itemList);


private:
    QPointer<LayerItem> m_layerItem;
    QPointer<FormEditorItem> m_formEditorItem;
    QPointer<AnchorIndicatorGraphicsItem> m_indicatorTopShape;
    QPointer<AnchorIndicatorGraphicsItem> m_indicatorBottomShape;
    QPointer<AnchorIndicatorGraphicsItem> m_indicatorLeftShape;
    QPointer<AnchorIndicatorGraphicsItem> m_indicatorRightShape;
};

} // namespace QmlDesigner

#endif // QMLDESIGNER_ANCHORINDICATOR_H
