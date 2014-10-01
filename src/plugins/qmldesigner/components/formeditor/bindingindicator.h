/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLDESIGNER_BINDINGINDICATOR_H
#define QMLDESIGNER_BINDINGINDICATOR_H

#include <QPointer>
#include "layeritem.h"
#include "formeditoritem.h"

namespace QmlDesigner {

class FormEditorItem;
class BindingIndicatorGraphicsItem;

class BindingIndicator
{
public:
    BindingIndicator(LayerItem *layerItem);
    BindingIndicator();
    ~BindingIndicator();

    void show();
    void hide();

    void clear();

    void setItems(const QList<FormEditorItem*> &itemList);
    void updateItems(const QList<FormEditorItem*> &itemList);

private:
    QPointer<LayerItem> m_layerItem;
    QPointer<FormEditorItem> m_formEditorItem;
    QPointer<BindingIndicatorGraphicsItem> m_indicatorTopShape;
    QPointer<BindingIndicatorGraphicsItem> m_indicatorBottomShape;
    QPointer<BindingIndicatorGraphicsItem> m_indicatorLeftShape;
    QPointer<BindingIndicatorGraphicsItem> m_indicatorRightShape;
};

} // namespace QmlDesigner

#endif // QMLDESIGNER_BINDINGINDICATOR_H
