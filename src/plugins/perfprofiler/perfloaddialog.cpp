/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "perfloaddialog.h"
#include "perfprofilerconstants.h"
#include "ui_perfloaddialog.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

using namespace Utils;

namespace PerfProfiler {
namespace Internal {

PerfLoadDialog::PerfLoadDialog(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::PerfLoadDialog)
{
    ui->setupUi(this);
    ui->kitChooser->populate();
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->browseExecutableDirButton, &QPushButton::pressed,
            this, &PerfLoadDialog::on_browseExecutableDirButton_pressed);
    connect(ui->browseTraceFileButton, &QPushButton::pressed,
            this, &PerfLoadDialog::on_browseTraceFileButton_pressed);
    chooseDefaults();
}

PerfLoadDialog::~PerfLoadDialog()
{
    delete ui;
}

QString PerfLoadDialog::traceFilePath() const
{
    return ui->traceFileLineEdit->text();
}

QString PerfLoadDialog::executableDirPath() const
{
    return ui->executableDirLineEdit->text();
}

ProjectExplorer::Kit *PerfLoadDialog::kit() const
{
    return ui->kitChooser->currentKit();
}

void PerfLoadDialog::on_browseTraceFileButton_pressed()
{
    FilePath filePath = FileUtils::getOpenFilePath(
                this, tr("Choose Perf Trace"), {},
                tr("Perf traces (*%1)").arg(Constants::TraceFileExtension));
    if (filePath.isEmpty())
        return;

    ui->traceFileLineEdit->setText(filePath.toUserOutput());
}

void PerfLoadDialog::on_browseExecutableDirButton_pressed()
{
    FilePath filePath = FileUtils::getExistingDirectory(
                this, tr("Choose Directory of Executable"));
    if (filePath.isEmpty())
        return;

    ui->executableDirLineEdit->setText(filePath.toUserOutput());
}

void PerfLoadDialog::chooseDefaults()
{
    ProjectExplorer::Target *target = ProjectExplorer::SessionManager::startupTarget();
    if (!target)
        return;

    ui->kitChooser->setCurrentKitId(target->kit()->id());

    if (auto *bc = target->activeBuildConfiguration())
        ui->executableDirLineEdit->setText(bc->buildDirectory().toString());
}

} // namespace Internal
} // namespace PerfProfiler
