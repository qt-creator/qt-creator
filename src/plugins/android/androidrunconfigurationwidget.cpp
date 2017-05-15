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

    setWidget(detailsWidget);
    setSummaryText(tr("Android run settings"));

    connect(m_ui->m_amStartArgsEdit, &QLineEdit::editingFinished, [this]() {
        QString optionText = m_ui->m_amStartArgsEdit->text().simplified();
        emit amStartArgsChanged(optionText.split(' '));
    });
}

AndroidRunConfigurationWidget::~AndroidRunConfigurationWidget()
{
}

void AndroidRunConfigurationWidget::setAmStartArgs(const QStringList &args)
{
    if (m_ui->m_amStartArgsEdit && !args.isEmpty())
        m_ui->m_amStartArgsEdit->setText(Utils::QtcProcess::joinArgs(args, Utils::OsTypeLinux));
}

} // namespace Internal
} // namespace Android

