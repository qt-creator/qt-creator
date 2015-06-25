/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise ClangStaticAnalyzer Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "clangstaticanalyzerconfigwidget.h"
#include "ui_clangstaticanalyzerconfigwidget.h"

#include "clangstaticanalyzerutils.h"

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
    chooser->setPath(settings->clangExecutable());
    const auto validator = [chooser](Utils::FancyLineEdit *edit, QString *errorMessage) {
        return chooser->defaultValidationFunction()(edit, errorMessage)
                && isClangExecutableUsable(chooser->fileName().toString(), errorMessage);
    };
    chooser->setValidationFunction(validator);
    connect(m_ui->clangExecutableChooser, &Utils::PathChooser::changed,
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

} // namespace Internal
} // namespace ClangStaticAnalyzer
