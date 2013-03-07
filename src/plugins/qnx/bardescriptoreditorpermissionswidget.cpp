/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "bardescriptoreditorpermissionswidget.h"
#include "ui_bardescriptoreditorpermissionswidget.h"

#include "bardescriptorpermissionsmodel.h"

using namespace Qnx;
using namespace Qnx::Internal;

BarDescriptorEditorPermissionsWidget::BarDescriptorEditorPermissionsWidget(QWidget *parent) :
    BarDescriptorEditorAbstractPanelWidget(parent),
    m_ui(new Ui::BarDescriptorEditorPermissionsWidget)
{
    m_ui->setupUi(this);

    m_permissionsModel = new BarDescriptorPermissionsModel(this);
    m_ui->permissionsView->setModel(m_permissionsModel);

    connect(m_ui->selectAllPermissions, SIGNAL(clicked()), m_permissionsModel, SLOT(checkAll()));
    connect(m_ui->deselectAllPermissions, SIGNAL(clicked()), m_permissionsModel, SLOT(uncheckAll()));
    connect(m_permissionsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(changed()));
}

BarDescriptorEditorPermissionsWidget::~BarDescriptorEditorPermissionsWidget()
{
    delete m_ui;
}

void BarDescriptorEditorPermissionsWidget::clear()
{
    disconnect(m_permissionsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(changed()));
    m_permissionsModel->uncheckAll();
    connect(m_permissionsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(changed()));
}

QStringList BarDescriptorEditorPermissionsWidget::checkedPermissions() const
{
    return m_permissionsModel->checkedIdentifiers();
}

void BarDescriptorEditorPermissionsWidget::checkPermission(const QString &identifier)
{
    disconnect(m_permissionsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(changed()));
    m_permissionsModel->checkPermission(identifier);
    connect(m_permissionsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(changed()));
}
