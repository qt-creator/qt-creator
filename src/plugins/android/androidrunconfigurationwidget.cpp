/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#include "androidrunconfigurationwidget.h"
#include "adbcommandswidget.h"
#include "ui_androidrunconfigurationwidget.h"

#include "utils/utilsicons.h"
#include "utils/qtcprocess.h"

namespace Android {
namespace Internal {

AndroidRunConfigurationWidget::AndroidRunConfigurationWidget(QWidget *parent):
    Utils::DetailsWidget(parent),
    m_ui(new Ui::AndroidRunConfigurationWidget)
{
    auto detailsWidget = new QWidget(this);
    m_ui->setupUi(detailsWidget);
    m_ui->m_warningIconLabel->setPixmap(Utils::Icons::WARNING.pixmap());

    m_preStartCmdsWidget = new AdbCommandsWidget(detailsWidget);
    connect(m_preStartCmdsWidget, &AdbCommandsWidget::commandsChanged, [this]() {
            emit preStartCmdsChanged(m_preStartCmdsWidget->commandsList());
    });
    m_preStartCmdsWidget->setTitleText(tr("Shell commands to run on Android device before"
                                          " application launch."));

    m_postEndCmdsWidget = new AdbCommandsWidget(detailsWidget);
    connect(m_postEndCmdsWidget, &AdbCommandsWidget::commandsChanged, [this]() {
            emit postFinishCmdsChanged(m_postEndCmdsWidget->commandsList());
    });
    m_postEndCmdsWidget->setTitleText(tr("Shell commands to run on Android device after application"
                                         " quits."));

    auto mainLayout = static_cast<QGridLayout*>(detailsWidget->layout());
    mainLayout->addWidget(m_preStartCmdsWidget->widget(), mainLayout->rowCount(),
                          0, mainLayout->columnCount() - 1, 0);
    mainLayout->addWidget(m_postEndCmdsWidget->widget(), mainLayout->rowCount(),
                          0, mainLayout->columnCount() - 1, 0);

    setWidget(detailsWidget);
    setSummaryText(tr("Android run settings"));

    connect(m_ui->m_amStartArgsEdit, &QLineEdit::editingFinished, [this]() {
        QString optionText = m_ui->m_amStartArgsEdit->text().simplified();
        emit amStartArgsChanged(optionText.split(' '));
    });
}

AndroidRunConfigurationWidget::~AndroidRunConfigurationWidget() = default;

void AndroidRunConfigurationWidget::setAmStartArgs(const QStringList &args)
{
    m_ui->m_amStartArgsEdit->setText(Utils::QtcProcess::joinArgs(args, Utils::OsTypeLinux));
}

void AndroidRunConfigurationWidget::setPreStartShellCommands(const QStringList &cmdList)
{
    m_preStartCmdsWidget->setCommandList(cmdList);
}

void AndroidRunConfigurationWidget::setPostFinishShellCommands(const QStringList &cmdList)
{
    m_postEndCmdsWidget->setCommandList(cmdList);
}

} // namespace Internal
} // namespace Android

