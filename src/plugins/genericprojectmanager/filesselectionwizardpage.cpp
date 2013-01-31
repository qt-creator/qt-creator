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

#include "filesselectionwizardpage.h"

#include "genericprojectwizard.h"
#include "genericprojectconstants.h"
#include "selectablefilesmodel.h"

#include <coreplugin/mimedatabase.h>
#include <coreplugin/icore.h>

#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>

namespace GenericProjectManager {
namespace Internal {

FilesSelectionWizardPage::FilesSelectionWizardPage(GenericProjectWizardDialog *genericProjectWizard, QWidget *parent)
    : QWizardPage(parent), m_genericProjectWizardDialog(genericProjectWizard), m_model(0), m_finished(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *hbox = new QHBoxLayout;
    m_filterLabel = new QLabel;
    m_filterLabel->setText(tr("Hide files matching:"));
    m_filterLabel->hide();
    hbox->addWidget(m_filterLabel);
    m_filterLineEdit = new QLineEdit;

    const QString filter = Core::ICore::settings()->value(QLatin1String(Constants::FILEFILTER_SETTING),
                                                          QLatin1String(Constants::FILEFILTER_DEFAULT)).toString();
    m_filterLineEdit->setText(filter);
    m_filterLineEdit->hide();
    hbox->addWidget(m_filterLineEdit);
    m_applyFilterButton = new QPushButton(tr("Apply Filter"), this);
    m_applyFilterButton->hide();
    hbox->addWidget(m_applyFilterButton);
    layout->addLayout(hbox);

    m_view = new QTreeView;
    m_view->setMinimumSize(500, 400);
    m_view->setHeaderHidden(true);
    m_view->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    m_label = new QLabel;
    m_label->setMaximumWidth(500);

    layout->addWidget(m_view);
    layout->addWidget(m_label);

    connect(m_applyFilterButton, SIGNAL(clicked()), this, SLOT(applyFilter()));
}

void FilesSelectionWizardPage::initializePage()
{
    m_view->setModel(0);
    delete m_model;
    m_model = new SelectableFilesModel(m_genericProjectWizardDialog->path(), this);
    connect(m_model, SIGNAL(parsingProgress(QString)),
            this, SLOT(parsingProgress(QString)));
    connect(m_model, SIGNAL(parsingFinished()),
            this, SLOT(parsingFinished()));
    m_model->startParsing();
    m_filterLabel->setVisible(false);
    m_filterLineEdit->setVisible(false);
    m_applyFilterButton->setVisible(false);
    m_view->setVisible(false);
    m_label->setVisible(true);
    m_view->setModel(m_model);
}

void FilesSelectionWizardPage::cleanupPage()
{
    m_model->cancel();
    m_model->waitForFinished();
}

void FilesSelectionWizardPage::parsingProgress(const QString &text)
{
    m_label->setText(tr("Generating file list...\n\n%1").arg(text));
}

void FilesSelectionWizardPage::parsingFinished()
{
    m_finished = true;
    m_filterLabel->setVisible(true);
    m_filterLineEdit->setVisible(true);
    m_applyFilterButton->setVisible(true);
    m_view->setVisible(true);
    m_label->setVisible(false);
    m_view->expand(m_view->model()->index(0,0, QModelIndex()));
    emit completeChanged();
    applyFilter();
    // work around qt
    m_genericProjectWizardDialog->setTitleFormat(m_genericProjectWizardDialog->titleFormat());
}

bool FilesSelectionWizardPage::isComplete() const
{
    return m_finished;
}

QStringList FilesSelectionWizardPage::selectedPaths() const
{
    return m_model ? m_model->selectedPaths() : QStringList();
}

QStringList FilesSelectionWizardPage::selectedFiles() const
{
    return m_model ? m_model->selectedFiles() : QStringList();
}

void FilesSelectionWizardPage::applyFilter()
{
    const QString filter = m_filterLineEdit->text();
    Core::ICore::settings()->setValue(QLatin1String(Constants::FILEFILTER_SETTING), filter);
    m_model->applyFilter(filter);
}

} // namespace Internal
} // namespace GenericProjectManager
