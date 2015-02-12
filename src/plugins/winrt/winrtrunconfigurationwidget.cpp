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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "winrtrunconfigurationwidget.h"
#include "winrtrunconfiguration.h"
#include "ui_winrtrunconfigurationwidget.h"

namespace WinRt {
namespace Internal {

WinRtRunConfigurationWidget::WinRtRunConfigurationWidget(WinRtRunConfiguration *rc, QWidget *parent)
    : Utils::DetailsWidget(parent)
    , m_runConfiguration(rc)
    , m_ui(new Ui::WinRtRunConfigurationWidget)
{
    setState(Expanded);
    setSummaryText(tr("Launch App"));
    setWidget(new QWidget(this));
    m_ui->setupUi(widget());
    widget()->setContentsMargins(0, 0, 0, 0);
    m_ui->arguments->setText(rc->arguments());
    connect(m_ui->arguments, SIGNAL(textChanged(QString)),
            rc, SLOT(setArguments(QString)));
    connect(m_ui->uninstallAfterStop, SIGNAL(stateChanged(int)),
            SLOT(onUninstallCheckBoxChanged()));
}

WinRtRunConfigurationWidget::~WinRtRunConfigurationWidget()
{
    delete m_ui;
}

void WinRtRunConfigurationWidget::setArguments(const QString &args)
{
    m_ui->arguments->setText(args);
}

void WinRtRunConfigurationWidget::onUninstallCheckBoxChanged()
{
    m_runConfiguration->setUninstallAfterStop(m_ui->uninstallAfterStop->isChecked());
}

} // namespace Internal
} // namespace WinRt
