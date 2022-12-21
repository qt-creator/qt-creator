// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    auto comboBox = new QComboBox(parent);
    comboBox->setMinimumWidth(120);
    comboBox->setToolTip(tr("Edit sub components defined in this file."));
    comboBox->setModel(m_componentView->standardItemModel());
    comboBox->setCurrentIndex(-1);
    connect(comboBox, &QComboBox::activated, this, &ComponentAction::emitCurrentComponentChanged);
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
