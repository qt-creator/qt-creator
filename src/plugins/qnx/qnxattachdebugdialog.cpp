/****************************************************************************
**
** Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company
** Contact: info@kdab.com
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

#include "qnxattachdebugdialog.h"

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

using namespace Qnx;
using namespace Qnx::Internal;

QnxAttachDebugDialog::QnxAttachDebugDialog(ProjectExplorer::KitChooser *kitChooser, QWidget *parent)
    : ProjectExplorer::DeviceProcessesDialog(kitChooser, parent)
{
    auto sourceLabel = new QLabel(tr("Project source directory:"), this);
    m_projectSource = new Utils::PathChooser(this);
    m_projectSource->setExpectedKind(Utils::PathChooser::ExistingDirectory);

    auto binaryLabel = new QLabel(tr("Local executable:"), this);
    m_localExecutable = new Utils::PathChooser(this);
    m_localExecutable->setExpectedKind(Utils::PathChooser::File);

    auto formLayout = new QFormLayout;
    formLayout->addRow(sourceLabel, m_projectSource);
    formLayout->addRow(binaryLabel, m_localExecutable);

    auto mainLayout = dynamic_cast<QVBoxLayout*>(layout());
    QTC_ASSERT(mainLayout, return);
    mainLayout->insertLayout(mainLayout->count() - 2, formLayout);
}

QString QnxAttachDebugDialog::projectSource() const
{
    return m_projectSource->path();
}

QString QnxAttachDebugDialog::localExecutable() const
{
    return m_localExecutable->path();
}
