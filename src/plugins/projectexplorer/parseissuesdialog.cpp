// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "parseissuesdialog.h"

#include "ioutputparser.h"
#include "kitaspects.h"
#include "kitchooser.h"
#include "kitmanager.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "taskhub.h"

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

using namespace Utils;

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
    setWindowTitle(Tr::tr("Parse Build Output"));

    d->stderrCheckBox.setText(Tr::tr("Output went to stderr"));
    d->stderrCheckBox.setChecked(true);

    d->clearTasksCheckBox.setText(Tr::tr("Clear existing tasks"));
    d->clearTasksCheckBox.setChecked(true);

    const auto loadFileButton = new QPushButton(Tr::tr("Load from File..."));
    connect(loadFileButton, &QPushButton::clicked, this, [this] {
        const FilePath filePath = FileUtils::getOpenFilePath(this, Tr::tr("Choose File"));
        if (filePath.isEmpty())
            return;
        QFile file(filePath.toString());
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, Tr::tr("Could Not Open File"),
                                  Tr::tr("Could not open file: \"%1\": %2")
                                  .arg(filePath.toUserOutput(), file.errorString()));
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
    const auto outputGroupBox = new QGroupBox(Tr::tr("Build Output"));
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
    const auto parserGroupBox = new QGroupBox(Tr::tr("Parsing Options"));
    layout->addWidget(parserGroupBox);
    const auto parserLayout = new QVBoxLayout(parserGroupBox);
    const auto kitChooserWidget = new QWidget;
    const auto kitChooserLayout = new QHBoxLayout(kitChooserWidget);
    kitChooserLayout->setContentsMargins(0, 0, 0, 0);
    kitChooserLayout->addWidget(new QLabel(Tr::tr("Use parsers from kit:")));
    kitChooserLayout->addWidget(&d->kitChooser);
    parserLayout->addWidget(kitChooserWidget);
    parserLayout->addWidget(&d->clearTasksCheckBox);

    layout->addWidget(buttonBox);
}

ParseIssuesDialog::~ParseIssuesDialog()
{
    delete d;
}

void ParseIssuesDialog::accept()
{
    const QList<Utils::OutputLineParser *> lineParsers =
            d->kitChooser.currentKit()->createOutputParsers();
    if (lineParsers.isEmpty()) {
        QMessageBox::critical(this, Tr::tr("Cannot Parse"), Tr::tr("Cannot parse: The chosen kit does "
                                                           "not provide an output parser."));
        return;
    }
    Utils::OutputFormatter parser;
    parser.setLineParsers(lineParsers);
    if (d->clearTasksCheckBox.isChecked())
        TaskHub::clearTasks();
    const QStringList lines = d->compileOutputEdit.toPlainText().split('\n');
    const Utils::OutputFormat format = d->stderrCheckBox.isChecked()
            ? Utils::StdErrFormat : Utils::StdOutFormat;
    for (const QString &line : lines)
        parser.appendMessage(line + '\n', format);
    parser.flush();
    QDialog::accept();
}

} // namespace Internal
} // namespace ProjectExplorer
