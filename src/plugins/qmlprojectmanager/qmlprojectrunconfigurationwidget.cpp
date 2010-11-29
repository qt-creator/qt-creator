/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
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
#include <QStringListModel>

using Core::ICore;
using Utils::DebuggerLanguageChooser;
using Qt4ProjectManager::QtVersionManager;

namespace QmlProjectManager {
namespace Internal {

QmlProjectRunConfigurationWidget::QmlProjectRunConfigurationWidget(QmlProjectRunConfiguration *rc) :
    m_runConfiguration(rc),
    m_qtVersionComboBox(0),
    m_fileListCombo(0),
    m_fileListModel(new QStringListModel(this))
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

    connect(m_fileListCombo, SIGNAL(activated(QString)), this, SLOT(setMainScript(QString)));
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
    QLabel *label = new QLabel(tr("Base environment for this runconfiguration:"), this);
    baseEnvironmentLayout->addWidget(label);
    m_baseEnvironmentComboBox = new QComboBox(this);
    m_baseEnvironmentComboBox->addItems(QStringList()
                                        << tr("Clean Environment")
                                        << tr("System Environment")
                                        << tr("Build Environment"));
    m_baseEnvironmentComboBox->setCurrentIndex(rc->baseEnvironmentBase());
    connect(m_baseEnvironmentComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(baseEnvironmentSelected(int)));
    baseEnvironmentLayout->addWidget(m_baseEnvironmentComboBox);
    baseEnvironmentLayout->addStretch(10);

    m_environmentWidget = new ProjectExplorer::EnvironmentWidget(this, baseEnvironmentWidget);
    m_environmentWidget->setBaseEnvironment(rc->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(baseEnvironmentText());
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
    QStringList files;

    files.append(CURRENT_FILE);
    int currentIndex = -1;
    QStringList sortedFiles = project->files();
    qStableSort(sortedFiles.begin(), sortedFiles.end(), caseInsensitiveLessThan);

    foreach (const QString &fn, sortedFiles) {
        QFileInfo fileInfo(fn);
        if (fileInfo.suffix() != QLatin1String("qml"))
            continue;

        QString fileName = projectDir.relativeFilePath(fn);
        if (fileName == m_runConfiguration->m_scriptFile)
            currentIndex = files.size();

        files.append(fileName);
    }
    m_fileListModel->setStringList(files);

    if (currentIndex != -1)
        m_fileListCombo->setCurrentIndex(currentIndex);
    else
        m_fileListCombo->setCurrentIndex(0);
}

void QmlProjectRunConfigurationWidget::setMainScript(const QString &file)
{
    m_runConfiguration->setMainScript(file);
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

void QmlProjectRunConfigurationWidget::baseEnvironmentChanged()
{
//    if (m_ignoreChange)
//        return;

    int index = QmlProjectRunConfiguration::BaseEnvironmentBase(
            m_runConfiguration->baseEnvironmentBase());
    m_baseEnvironmentComboBox->setCurrentIndex(index);
    m_environmentWidget->setBaseEnvironment(m_runConfiguration->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(baseEnvironmentText());
}

void QmlProjectRunConfigurationWidget::userEnvironmentChangesChanged()
{
    m_environmentWidget->setUserChanges(m_runConfiguration->userEnvironmentChanges());
}

void QmlProjectRunConfigurationWidget::baseEnvironmentSelected(int index)
{
//    m_ignoreChange = true;
    m_runConfiguration->setBaseEnvironmentBase(
                QmlProjectRunConfiguration::BaseEnvironmentBase(index));

    m_environmentWidget->setBaseEnvironment(m_runConfiguration->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(baseEnvironmentText());
//    m_ignoreChange = false;
}

QString QmlProjectRunConfigurationWidget::baseEnvironmentText() const
{
    if (m_runConfiguration->m_baseEnvironmentBase
            == QmlProjectRunConfiguration::CleanEnvironmentBase) {
        return tr("Clean Environment");
    } else if (m_runConfiguration->m_baseEnvironmentBase
             == QmlProjectRunConfiguration::SystemEnvironmentBase) {
        return tr("System Environment");
    } else {
        return tr("Build Environment");
    }
}

} // namespace Internal
} // namespace QmlProjectManager
