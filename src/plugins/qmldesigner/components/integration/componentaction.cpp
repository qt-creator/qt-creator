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

#include "componentaction.h"

#include <QComboBox>
#include "componentview.h"
#include <QStandardItemModel>
#include <modelnode.h>

namespace QmlDesigner {

ComponentAction::ComponentAction(ComponentView  *componentView)
  :  QWidgetAction(componentView),
     m_componentView(componentView),
     dontEmitCurrentComponentChanged(false)
{
}

void ComponentAction::setCurrentIndex(int index)
{
    dontEmitCurrentComponentChanged = true;
    emit currentIndexChanged(index);
    dontEmitCurrentComponentChanged = false;
}

QWidget *ComponentAction::createWidget(QWidget *parent)
{
    QComboBox *comboBox = new QComboBox(parent);
    comboBox->setMinimumWidth(120);
    comboBox->setToolTip(tr("Edit sub components defined in this file."));
    comboBox->setModel(m_componentView->standardItemModel());
    comboBox->setCurrentIndex(-1);
    connect(comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &ComponentAction::emitCurrentComponentChanged);
    connect(this, &ComponentAction::currentIndexChanged, comboBox, &QComboBox::setCurrentIndex);

    return comboBox;
}

void ComponentAction::emitCurrentComponentChanged(int index)
{
    if (dontEmitCurrentComponentChanged)
        return;

    ModelNode componentModelNode = m_componentView->modelNode(index);

    if (componentModelNode.isRootNode())
        emit changedToMaster();
    else
        emit currentComponentChanged(componentModelNode);
}

} // namespace QmlDesigner
