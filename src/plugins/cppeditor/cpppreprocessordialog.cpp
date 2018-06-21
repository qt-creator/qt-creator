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

#include "cpppreprocessordialog.h"
#include "ui_cpppreprocessordialog.h"

#include "cppeditor.h"
#include "cppeditorwidget.h"
#include "cppeditorconstants.h"

#include <projectexplorer/session.h>

using namespace CppEditor::Internal;

CppPreProcessorDialog::CppPreProcessorDialog(const QString &filePath, QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::CppPreProcessorDialog())
    , m_filePath(filePath)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_ui->setupUi(this);
    m_ui->editorLabel->setText(m_ui->editorLabel->text().arg(Utils::FileName::fromString(m_filePath).fileName()));
    m_ui->editWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    CppEditor::decorateEditor(m_ui->editWidget);

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
