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

#include "qmlprojectrunconfigurationwidget.h"
#include "qmlprojectrunconfiguration.h"
#include "qmlproject.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <utils/detailswidget.h>

#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QStandardItemModel>

#include <algorithm>

using Core::ICore;
using ProjectExplorer::ProjectExplorerPlugin;

namespace QmlProjectManager {
namespace Internal {

QmlProjectRunConfigurationWidget::QmlProjectRunConfigurationWidget(QmlProjectRunConfiguration *rc) :
    m_runConfiguration(rc),
    m_fileListCombo(0),
    m_fileListModel(new QStandardItemModel(this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);

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

    connect(m_fileListCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, &QmlProjectRunConfigurationWidget::setMainScript);
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::fileListChanged,
            this, &QmlProjectRunConfigurationWidget::updateFileComboBox);

    QLineEdit *qmlViewerArgs = new QLineEdit;
    qmlViewerArgs->setText(rc->m_qmlViewerArgs);
    connect(qmlViewerArgs, &QLineEdit::textChanged,
            this, &QmlProjectRunConfigurationWidget::onViewerArgsChanged);

    form->addRow(tr("Arguments:"), qmlViewerArgs);
    form->addRow(tr("Main QML file:"), m_fileListCombo);

    layout->addWidget(detailsWidget);

    updateFileComboBox();

    connect(rc, &QmlProjectRunConfiguration::scriptSourceChanged,
            this, &QmlProjectRunConfigurationWidget::updateFileComboBox);
}

static bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
{
    return s1.toLower() < s2.toLower();
}

void QmlProjectRunConfigurationWidget::updateFileComboBox()
{
    ProjectExplorer::Project *project = m_runConfiguration->target()->project();
    QDir projectDir(project->projectDirectory().toString());

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

    std::stable_sort(sortedFiles.begin(), sortedFiles.end(), caseInsensitiveLessThan);

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

} // namespace Internal
} // namespace QmlProjectManager
