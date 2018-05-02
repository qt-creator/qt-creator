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

#include "clangtoolsconfigwidget.h"
#include "ui_clangtoolsconfigwidget.h"

#include "clangtoolsutils.h"

#include <QDir>
#include <QThread>

namespace ClangTools {
namespace Internal {

ClangToolsConfigWidget::ClangToolsConfigWidget(
        ClangToolsSettings *settings,
        QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::ClangToolsConfigWidget)
    , m_settings(settings)
{
    m_ui->setupUi(this);

    m_ui->simultaneousProccessesSpinBox->setValue(settings->savedSimultaneousProcesses());
    m_ui->simultaneousProccessesSpinBox->setMinimum(1);
    m_ui->simultaneousProccessesSpinBox->setMaximum(QThread::idealThreadCount());
    connect(m_ui->simultaneousProccessesSpinBox,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            [settings](int count) { settings->setSimultaneousProcesses(count); });

    m_ui->buildBeforeAnalysis->setCheckState(settings->savedBuildBeforeAnalysis()
                                              ? Qt::Checked : Qt::Unchecked);
    connect(m_ui->buildBeforeAnalysis, &QCheckBox::toggled, [settings](bool checked) {
        settings->setBuildBeforeAnalysis(checked);
    });
}

ClangToolsConfigWidget::~ClangToolsConfigWidget()
{
    delete m_ui;
}

} // namespace Internal
} // namespace ClangTools
