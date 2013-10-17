/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cpppreprocessordialog.h"
#include "ui_cpppreprocessordialog.h"

#include "cppeditor.h"
#include "cppsnippetprovider.h"

#include <projectexplorer/session.h>

using namespace CppEditor::Internal;

CppPreProcessorDialog::CppPreProcessorDialog(CPPEditorWidget *editorWidget,
                                             const QList<CppTools::ProjectPart::Ptr> &projectParts)
    : QDialog(editorWidget)
    , m_ui(new Ui::CppPreProcessorDialog())
    , m_filePath(editorWidget->editor()->document()->filePath())
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_ui->setupUi(this);
    m_ui->editorLabel->setText(m_ui->editorLabel->text().arg(QFileInfo(m_filePath).fileName()));

    CppSnippetProvider().decorateEditor(m_ui->editWidget);

    foreach (CppTools::ProjectPart::Ptr projectPart, projectParts) {
        m_ui->projectComboBox->addItem(projectPart->displayName);
        ProjectPartAddition addition;
        addition.projectPart = projectPart;
        addition.additionalDirectives = ProjectExplorer::SessionManager::value(
                    projectPart->projectFile + QLatin1Char(',') +  m_filePath).toString();
        m_partAdditions << addition;
    }
    if (m_ui->projectComboBox->count() <= 1)
        m_ui->projectComboBox->setEnabled(false);
    m_ui->editWidget->setPlainText(
            m_partAdditions.value(m_ui->projectComboBox->currentIndex()).additionalDirectives);

    connect(m_ui->projectComboBox, SIGNAL(currentIndexChanged(int)), SLOT(projectChanged(int)));
    connect(m_ui->editWidget, SIGNAL(textChanged()), SLOT(textChanged()));
}

CppPreProcessorDialog::~CppPreProcessorDialog()
{
    delete m_ui;
}

int CppPreProcessorDialog::exec()
{
    if (QDialog::exec() == Rejected)
        return Rejected;

    foreach (ProjectPartAddition partAddition, m_partAdditions) {
        const QString &previousDirectives = ProjectExplorer::SessionManager::value(
                    partAddition.projectPart->projectFile
                    + QLatin1Char(',')
                    + m_filePath).toString();
        if (previousDirectives != partAddition.additionalDirectives) {
            ProjectExplorer::SessionManager::setValue(
                        partAddition.projectPart->projectFile + QLatin1Char(',') +  m_filePath,
                        partAddition.additionalDirectives);
        }
    }
    return Accepted;
}

QString CppPreProcessorDialog::additionalPreProcessorDirectives() const
{
    return m_ui->editWidget->toPlainText();
}

void CppPreProcessorDialog::projectChanged(int index)
{
    m_ui->editWidget->setPlainText(m_partAdditions[index].additionalDirectives);
}

void CppPreProcessorDialog::textChanged()
{
    m_partAdditions[m_ui->projectComboBox->currentIndex()].additionalDirectives
            = m_ui->editWidget->toPlainText();
}
