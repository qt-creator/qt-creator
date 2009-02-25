/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "qt4buildenvironmentwidget.h"

#include "ui_qt4buildenvironmentwidget.h"
#include "qt4project.h"

#include <projectexplorer/environmenteditmodel.h>

namespace {
bool debug = false;
}

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using ProjectExplorer::EnvironmentModel;

Qt4BuildEnvironmentWidget::Qt4BuildEnvironmentWidget(Qt4Project *project)
    : BuildStepConfigWidget(), m_pro(project)
{
    m_ui = new Ui::Qt4BuildEnvironmentWidget();
    m_ui->setupUi(this);

    m_environmentModel = new EnvironmentModel();
    m_environmentModel->setMergedEnvironments(true);
    m_ui->environmentTreeView->setModel(m_environmentModel);
    m_ui->environmentTreeView->header()->resizeSection(0, 250);

    connect(m_environmentModel, SIGNAL(userChangesUpdated()),
            this, SLOT(environmentModelUserChangesUpdated()));

    connect(m_environmentModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
            this, SLOT(updateButtonsEnabled()));

    connect(m_ui->editButton, SIGNAL(clicked(bool)),
            this, SLOT(editEnvironmentButtonClicked()));
    connect(m_ui->addButton, SIGNAL(clicked(bool)),
            this, SLOT(addEnvironmentButtonClicked()));
    connect(m_ui->removeButton, SIGNAL(clicked(bool)),
            this, SLOT(removeEnvironmentButtonClicked()));
    connect(m_ui->unsetButton, SIGNAL(clicked(bool)),
            this, SLOT(unsetEnvironmentButtonClicked()));
    connect(m_ui->environmentTreeView->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)),
            this, SLOT(environmentCurrentIndexChanged(QModelIndex, QModelIndex)));
    connect(m_ui->clearSystemEnvironmentCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(clearSystemEnvironmentCheckBoxClicked(bool)));
}

Qt4BuildEnvironmentWidget::~Qt4BuildEnvironmentWidget()
{
    delete m_ui;
    delete m_environmentModel;
}

QString Qt4BuildEnvironmentWidget::displayName() const
{
    return tr("Build Environment");
}

void Qt4BuildEnvironmentWidget::init(const QString &buildConfiguration)
{
    if (debug)
        qDebug() << "Qt4BuildConfigWidget::init()";

    m_buildConfiguration = buildConfiguration;
    m_ui->clearSystemEnvironmentCheckBox->setChecked(!m_pro->useSystemEnvironment(buildConfiguration));
    m_environmentModel->setBaseEnvironment(m_pro->baseEnvironment(buildConfiguration));
    m_environmentModel->setUserChanges(m_pro->userEnvironmentChanges(buildConfiguration));

    updateButtonsEnabled();
}


void Qt4BuildEnvironmentWidget::editEnvironmentButtonClicked()
{
    m_ui->environmentTreeView->edit(m_ui->environmentTreeView->currentIndex());
}

void Qt4BuildEnvironmentWidget::addEnvironmentButtonClicked()
{
    QModelIndex index = m_environmentModel->addVariable();
    m_ui->environmentTreeView->setCurrentIndex(index);
    m_ui->environmentTreeView->edit(index);
    updateButtonsEnabled();
}

void Qt4BuildEnvironmentWidget::removeEnvironmentButtonClicked()
{
    const QString &name = m_environmentModel->indexToVariable(m_ui->environmentTreeView->currentIndex());
    m_environmentModel->removeVariable(name);
    updateButtonsEnabled();
}
// unset in Merged Environment Mode means, unset if it comes from the base environment
// or remove when it is just a change we added
// unset in changes view, means just unset
void Qt4BuildEnvironmentWidget::unsetEnvironmentButtonClicked()
{
    const QString &name = m_environmentModel->indexToVariable(m_ui->environmentTreeView->currentIndex());
    if (!m_environmentModel->isInBaseEnvironment(name) && m_environmentModel->mergedEnvironments())
        m_environmentModel->removeVariable(name);
    else
        m_environmentModel->unset(name);
    updateButtonsEnabled();
}

void Qt4BuildEnvironmentWidget::switchEnvironmentTab(int newTab)
{
    bool mergedEnvironments = (newTab == 0);
    m_environmentModel->setMergedEnvironments(mergedEnvironments);
    if (mergedEnvironments) {
        m_ui->removeButton->setText(tr("Reset"));
    } else {
        m_ui->removeButton->setText(tr("Remove"));
    }
}

void Qt4BuildEnvironmentWidget::updateButtonsEnabled()
{
    environmentCurrentIndexChanged(m_ui->environmentTreeView->currentIndex(), QModelIndex());
}

void Qt4BuildEnvironmentWidget::environmentCurrentIndexChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)
    if (current.isValid()) {
        if (m_environmentModel->mergedEnvironments()) {
            const QString &name = m_environmentModel->indexToVariable(current);
            bool modified = m_environmentModel->isInBaseEnvironment(name) && m_environmentModel->changes(name);
            bool unset = m_environmentModel->isUnset(name);
            m_ui->removeButton->setEnabled(modified || unset);
            m_ui->unsetButton->setEnabled(!unset);
            return;
        } else {
            m_ui->removeButton->setEnabled(true);
            m_ui->unsetButton->setEnabled(!m_environmentModel->isUnset(m_environmentModel->indexToVariable(current)));
            return;
        }
    }
    m_ui->editButton->setEnabled(current.isValid());
    m_ui->removeButton->setEnabled(false);
    m_ui->unsetButton->setEnabled(false);
}

void Qt4BuildEnvironmentWidget::clearSystemEnvironmentCheckBoxClicked(bool checked)
{
    m_pro->setUseSystemEnvironment(m_buildConfiguration, !checked);
    m_environmentModel->setBaseEnvironment(m_pro->baseEnvironment(m_buildConfiguration));
}

void Qt4BuildEnvironmentWidget::environmentModelUserChangesUpdated()
{
    m_pro->setUserEnvironmentChanges(m_buildConfiguration, m_environmentModel->userChanges());
}
