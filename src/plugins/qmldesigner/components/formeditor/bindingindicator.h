/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
#pragma once

#include "layeritem.h"
#include "formeditoritem.h"

#include <QPointer>

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
