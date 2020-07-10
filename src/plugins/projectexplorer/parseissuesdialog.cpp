/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "parseissuesdialog.h"

#include "ioutputparser.h"
#include "kitinformation.h"
#include "kitchooser.h"
#include "kitmanager.h"
#include "projectexplorerconstants.h"
#include "taskhub.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/runextensions.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <memory>

namespace ProjectExplorer {
namespace Internal {

class ParseIssuesDialog::Private
{
public:
    QPlainTextEdit compileOutputEdit;
    QCheckBox stderrCheckBox;
    QCheckBox clearTasksCheckBox;
    KitChooser kitChooser;
};

ParseIssuesDialog::ParseIssuesDialog(QWidget *parent) : QDialog(parent), d(new Private)
{
    setWindowTitle(tr("Parse Build Output"));

    d->stderrCheckBox.setText(tr("Output went to stderr"));
    d->stderrCheckBox.setChecked(true);

    d->clearTasksCheckBox.setText(tr("Clear existing tasks"));
    d->clearTasksCheckBox.setChecked(true);

    const auto loadFileButton = new QPushButton(tr("Load from File..."));
    connect(loadFileButton, &QPushButton::clicked, this, [this] {
        const QString filePath = QFileDialog::getOpenFileName(this, tr("Choose File"));
        if (filePath.isEmpty())
            return;
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, tr("Could Not Open File"),
                                  tr("Could not open file: \"%1\": %2")
                                  .arg(filePath, file.errorString()));
            return;
        }
        d->compileOutputEdit.setPlainText(QString::fromLocal8Bit(file.readAll()));
    });

    d->kitChooser.populate();
    if (!d->kitChooser.hasStartupKit()) {
        for (const Kit * const k : KitManager::kits()) {
            if (DeviceTypeKitAspect::deviceTypeId(k) == Constants::DESKTOP_DEVICE_TYPE) {
                d->kitChooser.setCurrentKitId(k->id());
                break;
            }
        }
    }

    const auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(d->kitChooser.currentKit());

    const auto layout = new QVBoxLayout(this);
    const auto outputGroupBox = new QGroupBox(tr("Build Output"));
    layout->addWidget(outputGroupBox);
    const auto outputLayout = new QHBoxLayout(outputGroupBox);
    outputLayout->addWidget(&d->compileOutputEdit);
    const auto buttonsWidget = new QWidget;
    const auto outputButtonsLayout = new QVBoxLayout(buttonsWidget);
    outputLayout->addWidget(buttonsWidget);
    outputButtonsLayout->addWidget(loadFileButton);
    outputButtonsLayout->addWidget(&d->stderrCheckBox);
    outputButtonsLayout->addStretch(1);

    // TODO: Only very few parsers are available from a Kit (basically just the Toolchain one).
    //       If we introduced factories for IOutputParsers, we could offer the user
    //       to combine arbitrary parsers here.
    const auto parserGroupBox = new QGroupBox(tr("Parsing Options"));
    layout->addWidget(parserGroupBox);
    const auto parserLayout = new QVBoxLayout(parserGroupBox);
    const auto kitChooserWidget = new QWidget;
    const auto kitChooserLayout = new QHBoxLayout(kitChooserWidget);
    kitChooserLayout->setContentsMargins(0, 0, 0, 0);
    kitChooserLayout->addWidget(new QLabel(tr("Use parsers from kit:")));
    kitChooserLayout->addWidget(&d->kitChooser);
    parserLayout->addWidget(kitChooserWidget);
    parserLayout->addWidget(&d->clearTasksCheckBox);

    layout->addWidget(buttonBox);
}

ParseIssuesDialog::~ParseIssuesDialog()
{
    delete d;
}

static void parse(QFutureInterface<void> &future, const QString &output,
                  const std::unique_ptr<Utils::OutputFormatter> &parser, bool isStderr)
{
    const QStringList lines = output.split('\n');
    future.setProgressRange(0, lines.count());
    const Utils::OutputFormat format = isStderr ? Utils::StdErrFormat : Utils::StdOutFormat;
    for (const QString &line : lines) {
        parser->appendMessage(line + '\n', format);
        future.setProgressValue(future.progressValue() + 1);
        if (future.isCanceled())
            return;
    }
    parser->flush();
}

void ParseIssuesDialog::accept()
{
    const QList<Utils::OutputLineParser *> lineParsers =
            d->kitChooser.currentKit()->createOutputParsers();
    if (lineParsers.isEmpty()) {
        QMessageBox::critical(this, tr("Cannot Parse"), tr("Cannot parse: The chosen kit does "
                                                           "not provide an output parser."));
        return;
    }
    std::unique_ptr<Utils::OutputFormatter> parser(new Utils::OutputFormatter);
    parser->setLineParsers(lineParsers);
    if (d->clearTasksCheckBox.isChecked())
        TaskHub::clearTasks();
    const QFuture<void> f = Utils::runAsync(&parse, d->compileOutputEdit.toPlainText(),
                                            std::move(parser), d->stderrCheckBox.isChecked());
    Core::ProgressManager::addTask(f, tr("Parsing build output"),
                                   "ProgressExplorer.ParseExternalBuildOutput");
    QDialog::accept();
}

} // namespace Internal
} // namespace ProjectExplorer
