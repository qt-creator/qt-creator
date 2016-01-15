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

#include "cppeditorconstants.h"
#include "cppsnippetprovider.h"

#include <projectexplorer/session.h>

using namespace CppEditor::Internal;

static bool projectPartLessThan(const CppTools::ProjectPart::Ptr &projectPart1,
                                const CppTools::ProjectPart::Ptr &projectPart2)
{
    return projectPart1->displayName < projectPart2->displayName;
}

CppPreProcessorDialog::CppPreProcessorDialog(QWidget *parent, const QString &filePath,
                                             const QList<CppTools::ProjectPart::Ptr> &projectParts)
    : QDialog(parent)
    , m_ui(new Ui::CppPreProcessorDialog())
    , m_filePath(filePath)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_ui->setupUi(this);
    m_ui->editorLabel->setText(m_ui->editorLabel->text().arg(Utils::FileName::fromString(m_filePath).fileName()));
    m_ui->editWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    CppSnippetProvider().decorateEditor(m_ui->editWidget);

    const QString &projectPartIdToUse = ProjectExplorer::SessionManager::value(
                QLatin1String(Constants::CPP_PREPROCESSOR_PROJECT_PREFIX) + m_filePath).toString();
    int currentIndex = 0;

    QList<CppTools::ProjectPart::Ptr> sortedProjectParts(projectParts);
    qStableSort(sortedProjectParts.begin(), sortedProjectParts.end(), projectPartLessThan);

    foreach (CppTools::ProjectPart::Ptr projectPart, sortedProjectParts) {
        m_ui->projectComboBox->addItem(projectPart->displayName);
        ProjectPartAddition addition;
        addition.projectPart = projectPart;
        addition.additionalDirectives = ProjectExplorer::SessionManager::value(
                    projectPart->id() + QLatin1Char(',') +  m_filePath).toString();
        if (projectPart->id() == projectPartIdToUse)
            currentIndex = m_ui->projectComboBox->count() - 1;
        m_partAdditions << addition;
    }
    if (m_ui->projectComboBox->count() <= 1)
        m_ui->projectComboBox->setEnabled(false);
    m_ui->projectComboBox->setCurrentIndex(currentIndex);
    m_ui->editWidget->setPlainText(m_partAdditions.value(currentIndex).additionalDirectives);

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

    ProjectExplorer::SessionManager::setValue(
                QLatin1String(Constants::CPP_PREPROCESSOR_PROJECT_PREFIX) + m_filePath,
                m_partAdditions[m_ui->projectComboBox->currentIndex()].projectPart->id());

    foreach (ProjectPartAddition partAddition, m_partAdditions) {
        const QString &previousDirectives = ProjectExplorer::SessionManager::value(
                    partAddition.projectPart->id()
                    + QLatin1Char(',')
                    + m_filePath).toString();
        if (previousDirectives != partAddition.additionalDirectives) {
            ProjectExplorer::SessionManager::setValue(
                        partAddition.projectPart->id() + QLatin1Char(',') +  m_filePath,
                        partAddition.additionalDirectives);
        }
    }
    return Accepted;
}

CppTools::ProjectPart::Ptr CppPreProcessorDialog::projectPart() const
{
    return m_partAdditions[m_ui->projectComboBox->currentIndex()].projectPart;
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
