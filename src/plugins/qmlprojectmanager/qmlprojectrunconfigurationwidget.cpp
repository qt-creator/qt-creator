/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmlprojectrunconfigurationwidget.h"
#include "qmlprojectrunconfiguration.h"
#include "qmlprojecttarget.h"
#include "qmlproject.h"

#include <coreplugin/icore.h>
#include <projectexplorer/environmenteditmodel.h>
#include <projectexplorer/projectexplorer.h>
#include <utils/debuggerlanguagechooser.h>
#include <utils/detailswidget.h>
#include <utils/qtcassert.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qt4projectmanager/qtversionmanager.h>

#include <QLineEdit>
#include <QFormLayout>
#include <QPushButton>
#include <QStandardItemModel>

using Core::ICore;
using Utils::DebuggerLanguageChooser;
using Qt4ProjectManager::QtVersionManager;

namespace QmlProjectManager {
namespace Internal {

QmlProjectRunConfigurationWidget::QmlProjectRunConfigurationWidget(QmlProjectRunConfiguration *rc) :
    m_runConfiguration(rc),
    m_qtVersionComboBox(0),
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

    m_qtVersionComboBox = new QComboBox;
    m_qtVersionComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(m_qtVersionComboBox, SIGNAL(activated(int)),
            this, SLOT(onQtVersionSelectionChanged()));

    QPushButton *pushButton = new QPushButton;
    pushButton->setText(tr("Manage Qt versions"));
    connect(pushButton, SIGNAL(clicked()), this, SLOT(manageQtVersions()));

    QHBoxLayout *qtVersionLayout = new QHBoxLayout;
    qtVersionLayout->addWidget(m_qtVersionComboBox);
    qtVersionLayout->addWidget(pushButton);

    QLineEdit *qmlViewerArgs = new QLineEdit;
    qmlViewerArgs->setText(rc->m_qmlViewerArgs);
    connect(qmlViewerArgs, SIGNAL(textChanged(QString)), this, SLOT(onViewerArgsChanged()));

    form->addRow(tr("Qt version:"), qtVersionLayout);
    form->addRow(tr("Arguments:"), qmlViewerArgs);
    form->addRow(tr("Main QML file:"), m_fileListCombo);

    layout->addWidget(detailsWidget);

    updateFileComboBox();
    updateQtVersionComboBox();

    //
    // Debugging
    //

    QWidget *debuggerLabelWidget = new QWidget;
    QVBoxLayout *debuggerLabelLayout = new QVBoxLayout(debuggerLabelWidget);
    debuggerLabelLayout->setMargin(0);
    debuggerLabelLayout->setSpacing(0);
    debuggerLabelWidget->setLayout(debuggerLabelLayout);
    QLabel *debuggerLabel = new QLabel(tr("Debugger:"));
    debuggerLabelLayout->addWidget(debuggerLabel);
    debuggerLabelLayout->addStretch(10);

    DebuggerLanguageChooser *debuggerLanguageChooser = new DebuggerLanguageChooser(formWidget);
    form->addRow(debuggerLabelWidget, debuggerLanguageChooser);

    debuggerLanguageChooser->setCppChecked(rc->useCppDebugger());
    debuggerLanguageChooser->setQmlChecked(rc->useQmlDebugger());
    debuggerLanguageChooser->setQmlDebugServerPort(rc->qmlDebugServerPort());

    connect(debuggerLanguageChooser, SIGNAL(cppLanguageToggled(bool)),
            this, SLOT(useCppDebuggerToggled(bool)));
    connect(debuggerLanguageChooser, SIGNAL(qmlLanguageToggled(bool)),
            this, SLOT(useQmlDebuggerToggled(bool)));
    connect(debuggerLanguageChooser, SIGNAL(qmlDebugServerPortChanged(uint)),
            this, SLOT(qmlDebugServerPortChanged(uint)));

    QtVersionManager *qtVersions = QtVersionManager::instance();
    connect(qtVersions, SIGNAL(qtVersionsChanged(QList<int>)),
            this, SLOT(updateQtVersionComboBox()));

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
}

static bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
{
    return s1.toLower() < s2.toLower();
}

void QmlProjectRunConfigurationWidget::updateFileComboBox()
{
    QmlProject *project = m_runConfiguration->qmlTarget()->qmlProject();
    QDir projectDir = project->projectDir();

    m_fileListModel->clear();
    m_fileListModel->appendRow(new QStandardItem(CURRENT_FILE));
    QModelIndex currentIndex;
    QModelIndex fileInQmlProjectIndex;

    const QString mainScriptInFilePath = projectDir.absoluteFilePath(project->mainFile());

    QStringList sortedFiles = project->files();
    if (!sortedFiles.contains(mainScriptInFilePath))
        sortedFiles += mainScriptInFilePath;

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

        if (mainScriptInFilePath == fn)
            fileInQmlProjectIndex = item->index();
    }

    if (currentIndex.isValid()) {
        m_fileListCombo->setCurrentIndex(currentIndex.row());
    } else {
        m_fileListCombo->setCurrentIndex(0);
    }

    if (fileInQmlProjectIndex.isValid()) {
        QFont font;
        font.setBold(true);
        m_fileListModel->setData(fileInQmlProjectIndex, font, Qt::FontRole);
    }
}

void QmlProjectRunConfigurationWidget::setMainScript(int index)
{
    QmlProject *project = m_runConfiguration->qmlTarget()->qmlProject();
    QDir projectDir = project->projectDir();
    if (index == 0) {
        m_runConfiguration->setScriptSource(QmlProjectRunConfiguration::FileInEditor);
    } else {
        const QString path = m_fileListModel->data(m_fileListModel->index(index, 0)).toString();
        if (projectDir.relativeFilePath(project->mainFile()) == path) {
            m_runConfiguration->setScriptSource(QmlProjectRunConfiguration::FileInProjectFile);
        } else {
            m_runConfiguration->setScriptSource(QmlProjectRunConfiguration::FileInSettings, path);
        }
    }
}

void QmlProjectRunConfigurationWidget::onQtVersionSelectionChanged()
{
    QVariant data = m_qtVersionComboBox->itemData(m_qtVersionComboBox->currentIndex());
    QTC_ASSERT(data.isValid() && data.canConvert(QVariant::Int), return)
    m_runConfiguration->setQtVersionId(data.toInt());
    m_runConfiguration->updateEnabled();
}

void QmlProjectRunConfigurationWidget::onViewerArgsChanged()
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(sender()))
        m_runConfiguration->m_qmlViewerArgs = lineEdit->text();
}

void QmlProjectRunConfigurationWidget::useCppDebuggerToggled(bool toggled)
{
    m_runConfiguration->setUseCppDebugger(toggled);
    m_runConfiguration->updateEnabled();
}

void QmlProjectRunConfigurationWidget::useQmlDebuggerToggled(bool toggled)
{
    m_runConfiguration->setUseQmlDebugger(toggled);
    m_runConfiguration->updateEnabled();
}

void QmlProjectRunConfigurationWidget::qmlDebugServerPortChanged(uint port)
{
    m_runConfiguration->setQmlDebugServerPort(port);
}

void QmlProjectRunConfigurationWidget::manageQtVersions()
{
    ICore *core = ICore::instance();
    core->showOptionsDialog(Qt4ProjectManager::Constants::QT_SETTINGS_CATEGORY,
                            Qt4ProjectManager::Constants::QTVERSION_SETTINGS_PAGE_ID);
}

void QmlProjectRunConfigurationWidget::updateQtVersionComboBox()
{
    m_qtVersionComboBox->clear();

    QtVersionManager *qtVersions = QtVersionManager::instance();
    foreach (Qt4ProjectManager::QtVersion *version, qtVersions->validVersions()) {
        if (m_runConfiguration->isValidVersion(version)) {
            m_qtVersionComboBox->addItem(version->displayName(), version->uniqueId());
        }
    }

    if (m_runConfiguration->m_qtVersionId != -1) {
        int index = m_qtVersionComboBox->findData(m_runConfiguration->m_qtVersionId);
        QTC_ASSERT(index >= 0, return);
        m_qtVersionComboBox->setCurrentIndex(index);
    } else {
        m_qtVersionComboBox->addItem(tr("Invalid Qt version"), -1);
        m_qtVersionComboBox->setCurrentIndex(0);
    }
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
