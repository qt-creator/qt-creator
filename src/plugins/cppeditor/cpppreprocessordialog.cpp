// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "cpppreprocessordialog.h"
#include "ui_cpppreprocessordialog.h"

#include "cppeditorwidget.h"
#include "cppeditorconstants.h"
#include "cpptoolsreuse.h"

#include <projectexplorer/session.h>

using namespace CppEditor::Internal;

CppPreProcessorDialog::CppPreProcessorDialog(const QString &filePath, QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::CppPreProcessorDialog())
    , m_filePath(filePath)
{
    m_ui->setupUi(this);
    m_ui->editorLabel->setText(m_ui->editorLabel->text().arg(Utils::FilePath::fromString(m_filePath).fileName()));
    m_ui->editWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    decorateCppEditor(m_ui->editWidget);

    const QString key = Constants::EXTRA_PREPROCESSOR_DIRECTIVES + m_filePath;
    const QString directives = ProjectExplorer::SessionManager::value(key).toString();
    m_ui->editWidget->setPlainText(directives);
}

CppPreProcessorDialog::~CppPreProcessorDialog()
{
    delete m_ui;
}

int CppPreProcessorDialog::exec()
{
    if (QDialog::exec() == Rejected)
        return Rejected;

    const QString key = Constants::EXTRA_PREPROCESSOR_DIRECTIVES + m_filePath;
    ProjectExplorer::SessionManager::setValue(key, extraPreprocessorDirectives());

    return Accepted;
}

QString CppPreProcessorDialog::extraPreprocessorDirectives() const
{
    return m_ui->editWidget->toPlainText();
}
