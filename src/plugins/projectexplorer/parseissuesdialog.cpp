// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "parseissuesdialog.h"

#include "devicesupport/devicekitaspects.h"
#include "kitchooser.h"
#include "kitmanager.h"
#include "projectexplorertr.h"
#include "projectexplorerconstants.h"
#include "taskhub.h"

#include <utils/fileutils.h>
#include <utils/guiutils.h>
#include <utils/outputformatter.h>

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

using namespace Utils;

namespace ProjectExplorer::Internal {

class ParseIssuesDialog final : public QDialog
{
public:
    ParseIssuesDialog();

private:
    void accept() final;

    QPlainTextEdit m_compileOutputEdit;
    QCheckBox m_stderrCheckBox;
    QCheckBox m_clearTasksCheckBox;
    KitChooser m_kitChooser;
};

ParseIssuesDialog::ParseIssuesDialog() : QDialog(dialogParent())
{
    setWindowTitle(Tr::tr("Parse Build Output"));

    m_stderrCheckBox.setText(Tr::tr("Output went to stderr"));
    m_stderrCheckBox.setChecked(true);

    m_clearTasksCheckBox.setText(Tr::tr("Clear existing tasks"));
    m_clearTasksCheckBox.setChecked(true);

    const auto loadFileButton = new QPushButton(Tr::tr("Load from File..."));
    connect(loadFileButton, &QPushButton::clicked, this, [this] {
        const FilePath filePath = FileUtils::getOpenFilePath(Tr::tr("Choose File"));
        if (filePath.isEmpty())
            return;
        const Result<QByteArray> res = filePath.fileContents();
        if (!res) {
            QMessageBox::critical(this, Tr::tr("Could Not Open File"),
                                  Tr::tr("Could not open file: \"%1\": %2")
                                  .arg(filePath.toUserOutput(), res.error()));
            return;
        }
        m_compileOutputEdit.setPlainText(QString::fromLocal8Bit(res.value()));
    });

    m_kitChooser.populate();
    if (!m_kitChooser.hasStartupKit()) {
        for (const Kit * const k : KitManager::kits()) {
            if (RunDeviceTypeKitAspect::deviceTypeId(k) == Constants::DESKTOP_DEVICE_TYPE) {
                m_kitChooser.setCurrentKitId(k->id());
                break;
            }
        }
    }

    const auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_kitChooser.currentKit());

    const auto layout = new QVBoxLayout(this);
    const auto outputGroupBox = new QGroupBox(Tr::tr("Build Output"));
    layout->addWidget(outputGroupBox);
    const auto outputLayout = new QHBoxLayout(outputGroupBox);
    outputLayout->addWidget(&m_compileOutputEdit);
    const auto buttonsWidget = new QWidget;
    const auto outputButtonsLayout = new QVBoxLayout(buttonsWidget);
    outputLayout->addWidget(buttonsWidget);
    outputButtonsLayout->addWidget(loadFileButton);
    outputButtonsLayout->addWidget(&m_stderrCheckBox);
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
    kitChooserLayout->addWidget(&m_kitChooser);
    parserLayout->addWidget(kitChooserWidget);
    parserLayout->addWidget(&m_clearTasksCheckBox);

    layout->addWidget(buttonBox);
}

void ParseIssuesDialog::accept()
{
    const QList<OutputLineParser *> lineParsers = m_kitChooser.currentKit()->createOutputParsers();
    if (lineParsers.isEmpty()) {
        QMessageBox::critical(this, Tr::tr("Cannot Parse"), Tr::tr("Cannot parse: The chosen kit does "
                                                           "not provide an output parser."));
        return;
    }
    OutputFormatter parser;
    parser.setLineParsers(lineParsers);
    if (m_clearTasksCheckBox.isChecked())
        TaskHub::clearTasks();
    const QStringList lines = m_compileOutputEdit.toPlainText().split('\n');
    const OutputFormat format = m_stderrCheckBox.isChecked()
            ? StdErrFormat : StdOutFormat;
    for (const QString &line : lines)
        parser.appendMessage(line + '\n', format);
    parser.flush();
    QDialog::accept();
}

void executeParseIssuesDialog()
{
    ParseIssuesDialog dialog;
    dialog.exec();
}

} // ProjectExplorer::Internal
