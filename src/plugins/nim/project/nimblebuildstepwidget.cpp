/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimblebuildstepwidget.h"
#include "ui_nimblebuildstepwidget.h"

#include "nimblebuildstep.h"
#include "nimbleproject.h"

#include <QAction>

#include <projectexplorer/buildconfiguration.h>
#include <utils/utilsicons.h>


using namespace Nim;
using namespace ProjectExplorer;

NimbleBuildStepWidget::NimbleBuildStepWidget(NimbleBuildStep *bs)
    : BuildStepConfigWidget(bs)
    , ui(new Ui::NimbleBuildStepWidget)
{
    ui->setupUi(this);

    ui->argumentsLineEdit->setText(bs->arguments());
    QObject::connect(bs, &NimbleBuildStep::argumentsChanged, ui->argumentsLineEdit, &QLineEdit::setText);
    QObject::connect(ui->argumentsLineEdit, &QLineEdit::textEdited, bs, &NimbleBuildStep::setArguments);

    ui->resetButton->setIcon(Utils::Icons::RESET.icon());
    QObject::connect(ui->resetButton, &QToolButton::triggered, bs, &NimbleBuildStep::resetArguments);
}

NimbleBuildStepWidget::~NimbleBuildStepWidget()
{
    delete ui;
}
