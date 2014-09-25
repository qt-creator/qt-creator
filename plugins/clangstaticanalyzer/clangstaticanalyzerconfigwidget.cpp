/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "clangstaticanalyzerconfigwidget.h"
#include "ui_clangstaticanalyzerconfigwidget.h"

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

    m_ui->clangExecutableChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui->clangExecutableChooser->setHistoryCompleter(
                QLatin1String("ClangStaticAnalyzer.ClangCommand.History"));
    m_ui->clangExecutableChooser->setPromptDialogTitle(tr("Clang Command"));
    m_ui->clangExecutableChooser->setPath(settings->clangExecutable());
    connect(m_ui->clangExecutableChooser, &Utils::PathChooser::changed,
            m_settings, &ClangStaticAnalyzerSettings::setClangExecutable);

    m_ui->simultaneousProccessesSpinBox->setValue(settings->simultaneousProcesses());
    m_ui->simultaneousProccessesSpinBox->setMinimum(1);
    m_ui->simultaneousProccessesSpinBox->setMaximum(QThread::idealThreadCount());
    connect(m_ui->simultaneousProccessesSpinBox,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            m_settings, &ClangStaticAnalyzerSettings::setSimultaneousProcesses);
}

ClangStaticAnalyzerConfigWidget::~ClangStaticAnalyzerConfigWidget()
{
    delete m_ui;
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
