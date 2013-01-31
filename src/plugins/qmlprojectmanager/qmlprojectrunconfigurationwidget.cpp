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

#include "qmlprojectrunconfigurationwidget.h"
#include "qmlprojectrunconfiguration.h"
#include "qmlproject.h"

#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <projectexplorer/environmentwidget.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <utils/detailswidget.h>
#include <utils/environment.h>

#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>

using Core::ICore;

namespace QmlProjectManager {
namespace Internal {

QmlProjectRunConfigurationWidget::QmlProjectRunConfigurationWidget(QmlProjectRunConfiguration *rc) :
    m_runConfiguration(rc),
    m_fileListCombo(0),
    m_fileListModel(new QStandardItemModel(this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    //
    // Qt Version, Arguments
    //

    Utils::DetailsWidget *detailsWidget = new Utils::DetailsWidget();
    detailsWidget->setState(Utils::DetailsWidget::NoSummary);

    QWidget *formWidget = new QWidget(detailsWidget);
    detailsWidget->setWidget(formWidget);
    QFormLayout *form = new QFormLayout(formWidget);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_fileListCombo = new QComboBox;
    m_fileListCombo->setModel(m_fileListModel);

    connect(m_fileListCombo, SIGNAL(activated(int)), this, SLOT(setMainScript(int)));
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(fileListChanged()),
            SLOT(updateFileComboBox()));

    QLineEdit *qmlViewerArgs = new QLineEdit;
    qmlViewerArgs->setText(rc->m_qmlViewerArgs);
    connect(qmlViewerArgs, SIGNAL(textChanged(QString)), this, SLOT(onViewerArgsChanged()));

    form->addRow(tr("Arguments:"), qmlViewerArgs);
    form->addRow(tr("Main QML file:"), m_fileListCombo);

    layout->addWidget(detailsWidget);

    //
    // Environment
    //

    QLabel *environmentLabel = new QLabel(this);
    environmentLabel->setText(tr("Run Environment"));
    QFont f = environmentLabel->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() *1.2);
    environmentLabel->setFont(f);

    layout->addWidget(environmentLabel);

    QWidget *baseEnvironmentWidget = new QWidget;
    QHBoxLayout *baseEnvironmentLayout = new QHBoxLayout(baseEnvironmentWidget);
    baseEnvironmentLayout->setMargin(0);
    m_environmentWidget = new ProjectExplorer::EnvironmentWidget(this, baseEnvironmentWidget);
    m_environmentWidget->setBaseEnvironment(rc->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(tr("System Environment"));
    m_environmentWidget->setUserChanges(rc->userEnvironmentChanges());

    connect(m_environmentWidget, SIGNAL(userChangesChanged()),
            this, SLOT(userChangesChanged()));


    layout->addWidget(m_environmentWidget);

    updateFileComboBox();
}

static bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
{
    return s1.toLower() < s2.toLower();
}

void QmlProjectRunConfigurationWidget::updateFileComboBox()
{
    ProjectExplorer::Project *project = m_runConfiguration->target()->project();
    QDir projectDir(project->projectDirectory());

    if (m_runConfiguration->mainScriptSource() == QmlProjectRunConfiguration::FileInProjectFile) {
        const QString mainScriptInFilePath
                = projectDir.relativeFilePath(m_runConfiguration->mainScript());
        m_fileListModel->clear();
        m_fileListModel->appendRow(new QStandardItem(mainScriptInFilePath));
        m_fileListCombo->setEnabled(false);
        return;
    }

    m_fileListCombo->setEnabled(true);
    m_fileListModel->clear();
    m_fileListModel->appendRow(new QStandardItem(QLatin1String(CURRENT_FILE)));
    QModelIndex currentIndex;

    QStringList sortedFiles = project->files(ProjectExplorer::Project::AllFiles);

    // make paths relative to project directory
    QStringList relativeFiles;
    foreach (const QString &fn, sortedFiles) {
        relativeFiles += projectDir.relativeFilePath(fn);
    }
    sortedFiles = relativeFiles;

    qStableSort(sortedFiles.begin(), sortedFiles.end(), caseInsensitiveLessThan);

    QString mainScriptPath;
    if (m_runConfiguration->mainScriptSource() != QmlProjectRunConfiguration::FileInEditor)
        mainScriptPath = projectDir.relativeFilePath(m_runConfiguration->mainScript());

    foreach (const QString &fn, sortedFiles) {
        QFileInfo fileInfo(fn);
        if (fileInfo.suffix() != QLatin1String("qml"))
            continue;

        QStandardItem *item = new QStandardItem(fn);
        m_fileListModel->appendRow(item);

        if (mainScriptPath == fn)
            currentIndex = item->index();
    }

    if (currentIndex.isValid())
        m_fileListCombo->setCurrentIndex(currentIndex.row());
    else
        m_fileListCombo->setCurrentIndex(0);
}

void QmlProjectRunConfigurationWidget::setMainScript(int index)
{
    if (index == 0) {
        m_runConfiguration->setScriptSource(QmlProjectRunConfiguration::FileInEditor);
    } else {
        const QString path = m_fileListModel->data(m_fileListModel->index(index, 0)).toString();
        m_runConfiguration->setScriptSource(QmlProjectRunConfiguration::FileInSettings, path);
    }
}

void QmlProjectRunConfigurationWidget::onViewerArgsChanged()
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(sender()))
        m_runConfiguration->m_qmlViewerArgs = lineEdit->text();
}

void QmlProjectRunConfigurationWidget::userChangesChanged()
{
    m_runConfiguration->setUserEnvironmentChanges(m_environmentWidget->userChanges());
}

void QmlProjectRunConfigurationWidget::userEnvironmentChangesChanged()
{
    m_environmentWidget->setUserChanges(m_runConfiguration->userEnvironmentChanges());
}

} // namespace Internal
} // namespace QmlProjectManager
