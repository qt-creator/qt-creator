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

#include "clangstaticanalyzerconfigwidget.h"
#include "ui_clangstaticanalyzerconfigwidget.h"

#include "clangstaticanalyzerutils.h"

#include <QDir>
#include <QThread>

namespace ClangStaticAnalyzer {
namespace Internal {

ClangStaticAnalyzerConfigWidget::ClangStaticAnalyzerConfigWidget(
        ClangStaticAnalyzerSettings *settings,
        QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::ClangStaticAnalyzerConfigWidget)
    , m_settings(settings)
{
    m_ui->setupUi(this);

    Utils::PathChooser * const chooser = m_ui->clangExecutableChooser;
    chooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    chooser->setHistoryCompleter(QLatin1String("ClangStaticAnalyzer.ClangCommand.History"));
    chooser->setPromptDialogTitle(tr("Clang Command"));
    const auto validator = [chooser, this](Utils::FancyLineEdit *edit, QString *errorMessage) {
        const QString currentFilePath = chooser->fileName().toString();
        Utils::PathChooser pc;
        Utils::PathChooser *helperPathChooser;
        if (currentFilePath.isEmpty()) {
            pc.setExpectedKind(chooser->expectedKind());
            pc.setPath(edit->placeholderText());
            helperPathChooser = &pc;
        } else {
            helperPathChooser = chooser;
        }

        const bool isExecutableValid =
                chooser->defaultValidationFunction()(helperPathChooser->lineEdit(), errorMessage)
                && isClangExecutableUsable(helperPathChooser->fileName().toString(), errorMessage);

        const ClangExecutableVersion detectedVersion = isExecutableValid
                ? clangExecutableVersion(helperPathChooser->fileName().toString())
                : ClangExecutableVersion();
        updateDetectedVersionLabel(isExecutableValid, detectedVersion);

        return isExecutableValid;
    };
    chooser->setValidationFunction(validator);
    bool clangExeIsSet;
    const QString clangExe = settings->clangExecutable(&clangExeIsSet);
    chooser->lineEdit()->setPlaceholderText(QDir::toNativeSeparators(
                                                settings->defaultClangExecutable()));
    if (clangExeIsSet) {
        chooser->setPath(clangExe);
    } else {
        // Setting an empty string does not trigger the validator, as that is the initial value
        // in the line edit.
        chooser->setPath(QLatin1String(" "));
        chooser->lineEdit()->clear();
    }
    connect(m_ui->clangExecutableChooser, &Utils::PathChooser::rawPathChanged,
            [settings](const QString &path) { settings->setClangExecutable(path); });

    m_ui->simultaneousProccessesSpinBox->setValue(settings->simultaneousProcesses());
    m_ui->simultaneousProccessesSpinBox->setMinimum(1);
    m_ui->simultaneousProccessesSpinBox->setMaximum(QThread::idealThreadCount());
    connect(m_ui->simultaneousProccessesSpinBox,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            [settings](int count) { settings->setSimultaneousProcesses(count); });
}

ClangStaticAnalyzerConfigWidget::~ClangStaticAnalyzerConfigWidget()
{
    delete m_ui;
}

void ClangStaticAnalyzerConfigWidget::updateDetectedVersionLabel(
        bool isExecutableValid,
        const ClangExecutableVersion &providedVersion)
{
    QLabel &label = *m_ui->detectedVersionLabel;

    if (isExecutableValid) {
        if (providedVersion.isValid()) {
            if (providedVersion.isSupportedVersion()) {
                label.setText(tr("Version: %1, supported.")
                              .arg(providedVersion.toString()));
            } else {
                label.setText(tr("Version: %1, unsupported (supported version is %2).")
                              .arg(providedVersion.toString())
                              .arg(ClangExecutableVersion::supportedVersionAsString()));
            }
        } else {
            label.setText(tr("Version: Could not determine version."));
        }
    } else {
        label.setText(tr("Version: Set valid executable first."));
    }
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
