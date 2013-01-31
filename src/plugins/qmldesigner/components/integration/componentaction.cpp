/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "componentaction.h"

#include <QComboBox>
#include "componentview.h"
#include <QStandardItemModel>
#include <qmldesignerplugin.h>
#include <modelnode.h>

namespace QmlDesigner {

ComponentAction::ComponentAction(ComponentView  *componentView)
  :  QWidgetAction(componentView),
     m_componentView(componentView)
{
}

void ComponentAction::setCurrentIndex(int index)
{
    emit currentIndexChanged(index);
}

QWidget  *ComponentAction::createWidget(QWidget *parent)
{
    QComboBox *comboBox = new QComboBox(parent);
    comboBox->setMinimumWidth(120);
    comboBox->setToolTip(tr("Edit sub components defined in this file"));
    comboBox->setModel(m_componentView->standardItemModel());
    connect(comboBox, SIGNAL(currentIndexChanged(int)), SLOT(emitCurrentComponentChanged(int)));
    connect(this, SIGNAL(currentIndexChanged(int)), comboBox, SLOT(setCurrentIndex(int)));

    return comboBox;
}

static const QString fileNameOfCurrentDocument()
{
    return QmlDesignerPlugin::instance()->documentManager().currentDesignDocument()->textEditor()->document()->fileName();
}

void ComponentAction::emitCurrentComponentChanged(int index)
{
    ModelNode componentNode = m_componentView->modelNode(index);

    if ( index > 0)
        QmlDesignerPlugin::instance()->viewManager().pushInFileComponentOnCrambleBar(componentNode.id());
    else
         QmlDesignerPlugin::instance()->viewManager().pushFileOnCrambleBar(fileNameOfCurrentDocument());

    emit currentComponentChanged(componentNode);
}

} // namespace QmlDesigner
