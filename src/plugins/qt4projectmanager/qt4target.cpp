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

#include "qt4target.h"
#include "buildconfigurationinfo.h"

#include "makestep.h"
#include "qmakestep.h"
#include "qt4project.h"
#include "qt4basetargetfactory.h"
#include "qt4projectconfigwidget.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>
#include <utils/pathchooser.h>
#include <utils/detailswidget.h>

#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>
#include <QtGui/QCheckBox>
#include <QtGui/QMainWindow>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

// -------------------------------------------------------------------------
// Qt4BaseTargetFactory
// -------------------------------------------------------------------------

Qt4BaseTargetFactory::Qt4BaseTargetFactory(QObject *parent) :
    ITargetFactory(parent)
{

}

Qt4BaseTargetFactory::~Qt4BaseTargetFactory()
{

}

Qt4TargetSetupWidget *Qt4BaseTargetFactory::createTargetSetupWidget(const QString &id,
                                                                    const QString &proFilePath,
                                                                    const QtVersionNumber &number,
                                                                    bool importEnabled,
                                                                    QList<BuildConfigurationInfo> importInfos)
{
    QList<BuildConfigurationInfo> infos = this->availableBuildConfigurations(id, proFilePath, number);
    if (infos.isEmpty())
        return 0;
    return new Qt4DefaultTargetSetupWidget(this, id, proFilePath, infos, number, importEnabled, importInfos);
}

ProjectExplorer::Target *Qt4BaseTargetFactory::create(ProjectExplorer::Project *parent, const QString &id, Qt4TargetSetupWidget *widget)
{
    if (!widget->isTargetSelected())
        return 0;
    Q_ASSERT(qobject_cast<Qt4DefaultTargetSetupWidget *>(widget));
    Qt4DefaultTargetSetupWidget *w = static_cast<Qt4DefaultTargetSetupWidget *>(widget);
    return create(parent, id, w->buildConfigurationInfos());
}

Qt4BaseTargetFactory *Qt4BaseTargetFactory::qt4BaseTargetFactoryForId(const QString &id)
{
    QList<Qt4BaseTargetFactory *> factories = ExtensionSystem::PluginManager::instance()->getObjects<Qt4BaseTargetFactory>();
    foreach (Qt4BaseTargetFactory *fac, factories) {
        if (fac->supportsTargetId(id))
            return fac;
    }
    return 0;
}

// Return name of a build configuration.
QString Qt4BaseTargetFactory::msgBuildConfigurationName(const BuildConfigurationInfo &info)
{
    const QString qtVersionName = info.version->displayName();
    return (info.buildConfig & QtVersion::DebugBuild) ?
        //: Name of a debug build configuration to created by a project wizard, %1 being the Qt version name. We recommend not translating it.
        tr("%1 Debug").arg(qtVersionName) :
        //: Name of a release build configuration to created by a project wizard, %1 being the Qt version name. We recommend not translating it.
        tr("%1 Release").arg(qtVersionName);
}

QList<ProjectExplorer::Task> Qt4BaseTargetFactory::reportIssues(const QString &proFile)
{
    Q_UNUSED(proFile);
    return QList<ProjectExplorer::Task>();
}

// -------------------------------------------------------------------------
// Qt4BaseTarget
// -------------------------------------------------------------------------

Qt4BaseTarget::Qt4BaseTarget(Qt4Project *parent, const QString &id) :
    ProjectExplorer::Target(parent, id)
{
    connect(this, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(emitProFileEvaluateNeeded()));
    connect(this, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SIGNAL(environmentChanged()));
    connect(this, SIGNAL(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(onAddedBuildConfiguration(ProjectExplorer::BuildConfiguration*)));
}

Qt4BaseTarget::~Qt4BaseTarget()
{
}

ProjectExplorer::BuildConfigWidget *Qt4BaseTarget::createConfigWidget()
{
    return new Qt4ProjectConfigWidget(this);
}

Qt4BuildConfiguration *Qt4BaseTarget::activeBuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(Target::activeBuildConfiguration());
}

Qt4Project *Qt4BaseTarget::qt4Project() const
{
    return static_cast<Qt4Project *>(project());
}

QString Qt4BaseTarget::defaultBuildDirectory() const
{
    Qt4BaseTargetFactory *fac = Qt4BaseTargetFactory::qt4BaseTargetFactoryForId(id());
    return fac->defaultShadowBuildDirectory(qt4Project()->defaultTopLevelBuildDirectory(), id());
}

QList<ProjectExplorer::ToolChain *> Qt4BaseTarget::possibleToolChains(ProjectExplorer::BuildConfiguration *bc) const
{
    QList<ProjectExplorer::ToolChain *> tmp;
    QList<ProjectExplorer::ToolChain *> result;

    Qt4BuildConfiguration *qt4bc = qobject_cast<Qt4BuildConfiguration *>(bc);
    if (!qt4bc && !qt4bc->qtVersion()->isValid())
        return tmp;

    QList<ProjectExplorer::Abi> abiList = qt4bc->qtVersion()->qtAbis();
    foreach (const ProjectExplorer::Abi &abi, abiList)
        tmp.append(ProjectExplorer::ToolChainManager::instance()->findToolChains(abi));

    foreach (ProjectExplorer::ToolChain *tc, tmp) {
        if (result.contains(tc))
            continue;
        if (tc->restrictedToTargets().isEmpty() || tc->restrictedToTargets().contains(id()))
            result.append(tc);
    }
    return result;
}

void Qt4BaseTarget::removeUnconfiguredCustomExectutableRunConfigurations()
{
    if (runConfigurations().count()) {
        // Remove all run configurations which the new project wizard created
        QList<ProjectExplorer::RunConfiguration*> toRemove;
        foreach (ProjectExplorer::RunConfiguration * rc, runConfigurations()) {
            ProjectExplorer::CustomExecutableRunConfiguration *cerc
                    = qobject_cast<ProjectExplorer::CustomExecutableRunConfiguration *>(rc);
            if (cerc && !cerc->isConfigured())
                toRemove.append(rc);
        }
        foreach (ProjectExplorer::RunConfiguration *rc, toRemove)
            removeRunConfiguration(rc);
    }
}

Qt4BuildConfiguration *Qt4BaseTarget::addQt4BuildConfiguration(QString displayName, QtVersion *qtversion,
                                                           QtVersion::QmakeBuildConfigs qmakeBuildConfiguration,
                                                           QString additionalArguments,
                                                           QString directory)
{
    Q_ASSERT(qtversion);
    bool debug = qmakeBuildConfiguration & QtVersion::DebugBuild;

    // Add the buildconfiguration
    Qt4BuildConfiguration *bc = new Qt4BuildConfiguration(this);
    bc->setDefaultDisplayName(displayName);

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    Q_ASSERT(buildSteps);
    Q_ASSERT(cleanSteps);

    QMakeStep *qmakeStep = new QMakeStep(buildSteps);
    buildSteps->insertStep(0, qmakeStep);

    MakeStep *makeStep = new MakeStep(buildSteps);
    buildSteps->insertStep(1, makeStep);

    MakeStep* cleanStep = new MakeStep(cleanSteps);
    cleanStep->setClean(true);
    cleanStep->setUserArguments("clean");
    cleanSteps->insertStep(0, cleanStep);
    if (!additionalArguments.isEmpty())
        qmakeStep->setUserArguments(additionalArguments);

    // set some options for qmake and make
    if (qmakeBuildConfiguration & QtVersion::BuildAll) // debug_and_release => explicit targets
        makeStep->setUserArguments(debug ? "debug" : "release");

    bc->setQMakeBuildConfiguration(qmakeBuildConfiguration);

    // Finally set the qt version & tool chain
    bc->setQtVersion(qtversion);
    if (!directory.isEmpty())
        bc->setShadowBuildAndDirectory(directory != project()->projectDirectory(), directory);
    addBuildConfiguration(bc);

    return bc;
}

void Qt4BaseTarget::onAddedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc)
{
    Q_ASSERT(bc);
    Qt4BuildConfiguration *qt4bc = qobject_cast<Qt4BuildConfiguration *>(bc);
    Q_ASSERT(qt4bc);
    connect(qt4bc, SIGNAL(buildDirectoryInitialized()),
            this, SIGNAL(buildDirectoryInitialized()));
    connect(qt4bc, SIGNAL(proFileEvaluateNeeded(Qt4ProjectManager::Qt4BuildConfiguration *)),
            this, SLOT(onProFileEvaluateNeeded(Qt4ProjectManager::Qt4BuildConfiguration *)));
}

void Qt4BaseTarget::onProFileEvaluateNeeded(Qt4ProjectManager::Qt4BuildConfiguration *bc)
{
    if (bc && bc == activeBuildConfiguration())
        emit proFileEvaluateNeeded(this);
}

void Qt4BaseTarget::emitProFileEvaluateNeeded()
{
    emit proFileEvaluateNeeded(this);
}

// -------------------------------------------------------------------------
// Qt4TargetSetupWidget
// -------------------------------------------------------------------------

Qt4TargetSetupWidget::Qt4TargetSetupWidget(QWidget *parent)
    : QWidget(parent)
{

}

Qt4TargetSetupWidget::~Qt4TargetSetupWidget()
{

}

// -------------------------------------------------------------------------
// Qt4DefaultTargetSetupWidget
// -------------------------------------------------------------------------

QString issuesListToString(const QList<ProjectExplorer::Task> &issues)
{
    QStringList lines;
    foreach (const ProjectExplorer::Task &t, issues) {
        // set severity:
        QString severity;
        if (t.type == ProjectExplorer::Task::Error)
            severity = QCoreApplication::translate("Qt4DefaultTargetSetupWidget", "<b>Error:</b> ", "Severity is Task::Error");
        else if (t.type == ProjectExplorer::Task::Warning)
            severity = QCoreApplication::translate("Qt4DefaultTargetSetupWidget", "<b>Warning:</b> ", "Severity is Task::Warning");
        lines.append(severity + t.description);
    }
    return lines.join("<br>");
}

Qt4DefaultTargetSetupWidget::Qt4DefaultTargetSetupWidget(Qt4BaseTargetFactory *factory,
                                                         const QString &id,
                                                         const QString &proFilePath,
                                                         const QList<BuildConfigurationInfo> &infos,
                                                         const QtVersionNumber &minimumQtVersion,
                                                         bool importEnabled,
                                                         const QList<BuildConfigurationInfo> &importInfos)
    : Qt4TargetSetupWidget(),
      m_id(id),
      m_factory(factory),
      m_proFilePath(proFilePath),
      m_minimumQtVersion(minimumQtVersion),
      m_importInfos(importInfos),
      m_directoriesEnabled(true),
      m_hasInSourceBuild(false),
      m_ignoreChange(false),
      m_showImport(importEnabled),
      m_selected(0)
{
    QString sourceDir = QFileInfo(m_proFilePath).absolutePath();

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QVBoxLayout *vboxLayout = new QVBoxLayout();
    vboxLayout->setMargin(0);
    setLayout(vboxLayout);
    m_detailsWidget = new Utils::DetailsWidget(this);
    m_detailsWidget->setSummaryText(factory->displayNameForId(id));
    m_detailsWidget->setUseCheckBox(true);
    m_detailsWidget->setChecked(true);
    m_detailsWidget->setSummaryFontBold(true);
    m_detailsWidget->setIcon(factory->iconForId(id));
    m_detailsWidget->setAdditionalSummaryText(issuesListToString(factory->reportIssues(m_proFilePath)));
    vboxLayout->addWidget(m_detailsWidget);

    QWidget *widget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    widget->setLayout(layout);

    m_importLayout = new QGridLayout;
    m_importLayout->setMargin(0);
    layout->addLayout(m_importLayout);

    m_importLineLayout = new QHBoxLayout();
    m_importLineLabel = new QLabel();
    m_importLineLabel->setText(tr("Add build from:"));
    m_importLineLayout->addWidget(m_importLineLabel);

    m_importLinePath = new Utils::PathChooser();
    m_importLinePath->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_importLinePath->setPath(sourceDir);
    m_importLineLayout->addWidget(m_importLinePath);

    m_importLineButton = new QPushButton;
    m_importLineButton->setText(tr("Add Build"));
    m_importLineLayout->addWidget(m_importLineButton);
    m_importLineLayout->addStretch();
    layout->addLayout(m_importLineLayout);

    m_importLineLabel->setVisible(false);
    m_importLinePath->setVisible(false);
    m_importLineButton->setVisible(m_showImport);

    m_spacerTopWidget = new QWidget;
    m_spacerTopWidget->setMinimumHeight(12);
    layout->addWidget(m_spacerTopWidget);

    m_shadowBuildEnabled = new QCheckBox;
    m_shadowBuildEnabled->setText(tr("Use Shadow Building"));
    m_shadowBuildEnabled->setChecked(true);
    m_shadowBuildEnabled->setVisible(false);
    layout->addWidget(m_shadowBuildEnabled);

    m_spacerBottomWidget = new QWidget;
    m_spacerBottomWidget->setMinimumHeight(0);
    layout->addWidget(m_spacerBottomWidget);

    m_newBuildsLayout = new QGridLayout;
    m_newBuildsLayout->setMargin(0);
    layout->addLayout(m_newBuildsLayout);

    m_spacerTopWidget->setVisible(false);
    m_spacerBottomWidget->setVisible(false);

    m_detailsWidget->setWidget(widget);

    for (int i = 0; i < m_importInfos.size(); ++i) {
        if (m_importInfos.at(i).directory == sourceDir)
            m_hasInSourceBuild = true;
        m_importEnabled << true;
    }

    m_selected += m_importInfos.size();

    setupImportWidgets();

    setBuildConfigurationInfos(infos);

    if (!m_importInfos.isEmpty())
        m_detailsWidget->setState(Utils::DetailsWidget::Expanded);

    connect(m_importLineButton, SIGNAL(clicked()),
            this, SLOT(addImportClicked()));

    connect(m_detailsWidget, SIGNAL(checked(bool)),
            this, SIGNAL(selectedToggled()));
    connect(m_detailsWidget, SIGNAL(checked(bool)),
            widget, SLOT(setEnabled(bool)));
    connect(m_shadowBuildEnabled, SIGNAL(toggled(bool)),
            this, SLOT(shadowBuildingToggled()));

}

Qt4DefaultTargetSetupWidget::~Qt4DefaultTargetSetupWidget()
{
}

bool Qt4DefaultTargetSetupWidget::isTargetSelected() const
{
    return m_detailsWidget->isChecked() && m_selected;
}

void Qt4DefaultTargetSetupWidget::setTargetSelected(bool b)
{
    // Only check target if there are build configurations possible
    b == b && !buildConfigurationInfos().isEmpty();
    m_detailsWidget->setChecked(b);
    // We want the shadow build option to be visible
    if (m_shadowBuildEnabled->isVisible() && b)
        m_detailsWidget->setState(Utils::DetailsWidget::Expanded);
}

QString Qt4DefaultTargetSetupWidget::displayNameFrom(const BuildConfigurationInfo &info)
{
    QString buildType;
    if ((info.buildConfig & QtVersion::BuildAll) == 0) {
        if (info.buildConfig & QtVersion::DebugBuild)
            //: Debug build
            buildType = tr("debug");
        else
            //: release build
            buildType = tr("release");
    }
    return info.version->displayName() + ' ' + buildType;
}

void Qt4DefaultTargetSetupWidget::setProFilePath(const QString &proFilePath)
{
    m_proFilePath = proFilePath;
    m_detailsWidget->setAdditionalSummaryText(issuesListToString(m_factory->reportIssues(m_proFilePath)));
    setBuildConfigurationInfos(m_factory->availableBuildConfigurations(m_id, proFilePath, m_minimumQtVersion), false);
}

void Qt4DefaultTargetSetupWidget::setShadowBuildCheckBoxVisible(bool b)
{
    m_shadowBuildEnabled->setVisible(b);
    m_spacerTopWidget->setVisible(b && !m_importInfos.isEmpty());
    m_spacerBottomWidget->setVisible(b);
    m_shadowBuildEnabled->setChecked(!m_hasInSourceBuild);
}

QList<BuildConfigurationInfo> Qt4DefaultTargetSetupWidget::buildConfigurationInfos() const
{
    QList<BuildConfigurationInfo> infos;
    for (int i = 0; i < m_importInfos.size(); ++i) {
        if (m_importEnabled.at(i))
            infos << m_importInfos.at(i);
    }

    QString sourceDir = QFileInfo(m_proFilePath).absolutePath();

    int size = m_infos.size();
    for (int i=0; i < size; ++i) {
        if (m_enabled.at(i)) {
            BuildConfigurationInfo info = m_infos.at(i);
            if (!m_shadowBuildEnabled->isChecked())
                info.directory = sourceDir;
            infos << info;
        }
    }
    return infos;
}

void Qt4DefaultTargetSetupWidget::addImportClicked()
{
    if (!m_importLineLabel->isVisible()) {
        m_importLineLabel->setVisible(true);
        m_importLinePath->setVisible(true);
        return;
    }
    BuildConfigurationInfo info = BuildConfigurationInfo::checkForBuild(m_importLinePath->path(), m_proFilePath);
    if (!info.isValid()) {
        QMessageBox::critical(Core::ICore::instance()->mainWindow(),
                              tr("No build found"),
                              tr("No Build found in %1 matching project %2.").arg(m_importLinePath->path()).arg(m_proFilePath));
        return;
    }

    if (!info.version->supportsTargetId(m_id)) {
        QMessageBox::critical(this,
                              tr("Incompatible build found"),
                              tr("The Build found in %1 is incompatible with this target").arg(m_importLinePath->path()));
        return;
    }
    if (m_selected == m_enabled.size() && m_importInfos.isEmpty()) {
        // All regular builds are enabled and we didn't have a import before
        // That means we are still in the default setup, in that case
        // we disable all the builds
        m_selected = 0;
        for (int i=0; i<m_enabled.size(); ++i) {
            m_enabled[i] = false;
            m_checkboxes.at(i)->setChecked(false);
        }
    }


    ++m_selected;
    m_importEnabled << true;

    m_importInfos << info;

    createImportWidget(info, m_importEnabled.size() - 1);
    emit newImportBuildConfiguration(info);
}

QList<BuildConfigurationInfo> Qt4DefaultTargetSetupWidget::usedImportInfos()
{
    QList<BuildConfigurationInfo> infos;
    for (int i = 0; i < m_importInfos.size(); ++i) {
        if (m_importEnabled.at(i))
            infos << m_importInfos.at(i);
    }
    return infos;
}

void Qt4DefaultTargetSetupWidget::setBuildConfigurationInfos(const QList<BuildConfigurationInfo> &infos, bool resetEnabled)
{
    m_infos = infos;
    if (resetEnabled || m_infos.size() != m_enabled.size()) {
        m_enabled.clear();
        m_selected = 0;
        QStringList existingBuilds;
        for (int i = 0; i < m_importInfos.size(); ++i) {
            const BuildConfigurationInfo &info = m_importInfos.at(i);
            existingBuilds << info.directory;
            if (m_importEnabled.at(i))
                ++m_selected;
        }

        // Default to importing existing builds and disable
        // builds that would overwrite imports
        for (int i=0; i < m_infos.size(); ++i) {
            if (existingBuilds.contains(m_infos.at(i).directory) || m_hasInSourceBuild) {
                m_enabled << false;
            } else {
                m_enabled << true;
                ++m_selected;
            }
        }

        clearWidgets();
        setupWidgets();
    } else {
        bool foundIssues = false;
        m_ignoreChange = true;
        QString sourceDir = QFileInfo(m_proFilePath).absolutePath();
        for (int i=0; i < m_checkboxes.size(); ++i) {
            const BuildConfigurationInfo &info = m_infos.at(i);

            m_checkboxes[i]->setText(displayNameFrom(info));
            if (m_shadowBuildEnabled->isChecked())
                m_pathChoosers[i]->setPath(info.directory);
            else
                m_pathChoosers[i]->setPath(sourceDir);
            foundIssues |= reportIssues(i);
        }
        m_ignoreChange = false;
        if (foundIssues)
            m_detailsWidget->setState(Utils::DetailsWidget::Expanded);
    }
}

void Qt4DefaultTargetSetupWidget::setupImportWidgets()
{
    for (int i = 0; i < m_importInfos.size(); ++i)
        createImportWidget(m_importInfos.at(i), i);
}

void Qt4DefaultTargetSetupWidget::createImportWidget(const BuildConfigurationInfo &info, int pos)
{
    QCheckBox *checkBox = new QCheckBox;
    checkBox->setText(tr("Import build from %1").arg(info.directory));
    checkBox->setChecked(m_importEnabled.at(pos));
    if (info.version)
        checkBox->setToolTip(info.version->toHtml(false));
    m_importLayout->addWidget(checkBox, pos, 0, 1, 2);

    connect(checkBox, SIGNAL(toggled(bool)),
            this, SLOT(importCheckBoxToggled(bool)));

    m_importCheckBoxes.append(checkBox);
}

void Qt4DefaultTargetSetupWidget::setupWidgets()
{
    m_ignoreChange = true;
    QString sourceDir = QFileInfo(m_proFilePath).absolutePath();
    bool foundIssues = false;
    for (int i = 0; i < m_infos.size(); ++i) {
        const BuildConfigurationInfo &info = m_infos.at(i);
        QCheckBox *checkbox = new QCheckBox;
        checkbox->setText(displayNameFrom(info));
        checkbox->setChecked(m_enabled.at(i));
        if (info.version)
            checkbox->setToolTip(info.version->toHtml(false));
        m_newBuildsLayout->addWidget(checkbox, i * 2, 0);

        Utils::PathChooser *pathChooser = new Utils::PathChooser();
        pathChooser->setExpectedKind(Utils::PathChooser::Directory);
        if (m_shadowBuildEnabled->isChecked())
            pathChooser->setPath(info.directory);
        else
            pathChooser->setPath(sourceDir);
        pathChooser->setReadOnly(!m_directoriesEnabled);
        m_newBuildsLayout->addWidget(pathChooser, i * 2, 1);

        QLabel *reportIssuesLabel = new QLabel;
        reportIssuesLabel->setIndent(32);
        m_newBuildsLayout->addWidget(reportIssuesLabel, i * 2 + 1, 0, 1, 2);

        connect(checkbox, SIGNAL(toggled(bool)),
                this, SLOT(checkBoxToggled(bool)));

        connect(pathChooser, SIGNAL(changed(QString)),
                this, SLOT(pathChanged()));

        m_checkboxes.append(checkbox);
        m_pathChoosers.append(pathChooser);
        m_reportIssuesLabels.append(reportIssuesLabel);
        foundIssues |= reportIssues(i);
    }
    if (foundIssues)
        m_detailsWidget->setState(Utils::DetailsWidget::Expanded);
    m_ignoreChange = false;
}

void Qt4DefaultTargetSetupWidget::clearWidgets()
{
    qDeleteAll(m_checkboxes);
    m_checkboxes.clear();
    qDeleteAll(m_pathChoosers);
    m_pathChoosers.clear();
    qDeleteAll(m_reportIssuesLabels);
    m_reportIssuesLabels.clear();
}

void Qt4DefaultTargetSetupWidget::checkBoxToggled(bool b)
{
    QCheckBox *box = qobject_cast<QCheckBox *>(sender());
    if (!box)
        return;
    int index = m_checkboxes.indexOf(box);
    if (index == -1)
        return;
    if (m_enabled[index] == b)
        return;
    m_selected += b ? 1 : -1;
    m_enabled[index] = b;
    if ((m_selected == 0 && !b) || (m_selected == 1 && b))
        emit selectedToggled();
}

void Qt4DefaultTargetSetupWidget::importCheckBoxToggled(bool b)
{
    QCheckBox *box = qobject_cast<QCheckBox *>(sender());
    if (!box)
        return;
    int index = m_importCheckBoxes.indexOf(box);
    if (index == -1)
        return;
    if (m_importEnabled[index] == b)
        return;
    m_selected += b ? 1 : -1;
    m_importEnabled[index] = b;
    if ((m_selected == 0 && !b) || (m_selected == 1 && b))
        emit selectedToggled();
}

void Qt4DefaultTargetSetupWidget::pathChanged()
{
    if (m_ignoreChange)
        return;
    Utils::PathChooser *pathChooser = qobject_cast<Utils::PathChooser *>(sender());
    if (!pathChooser)
        return;
    int index = m_pathChoosers.indexOf(pathChooser);
    if (index == -1)
        return;
    m_infos[index].directory = pathChooser->path();
    reportIssues(index);
}

void Qt4DefaultTargetSetupWidget::shadowBuildingToggled()
{
    m_ignoreChange = true;
    bool b = m_shadowBuildEnabled->isChecked();
    if (m_directoriesEnabled == b)
        return;
    m_directoriesEnabled = b;
    QString sourceDir = QFileInfo(m_proFilePath).absolutePath();
    for (int i = 0; i < m_pathChoosers.size(); ++i) {
        Utils::PathChooser *pathChooser = m_pathChoosers.at(i);
        pathChooser->setReadOnly(!b);
        if (b)
            pathChooser->setPath(m_infos.at(i).directory);
        else
            pathChooser->setPath(sourceDir);
        reportIssues(i);
    }
    m_ignoreChange = false;
}

bool Qt4DefaultTargetSetupWidget::reportIssues(int index)
{
    QPair<ProjectExplorer::Task::TaskType, QString> issues = findIssues(m_infos.at(index));
    QLabel *reportIssuesLabel = m_reportIssuesLabels.at(index);
    reportIssuesLabel->setText(issues.second);
    reportIssuesLabel->setVisible(issues.first != ProjectExplorer::Task::Unknown);
    return issues.first != ProjectExplorer::Task::Unknown;
}

QPair<ProjectExplorer::Task::TaskType, QString> Qt4DefaultTargetSetupWidget::findIssues(const BuildConfigurationInfo &info)
{
    if (m_proFilePath.isEmpty())
        return qMakePair(ProjectExplorer::Task::Unknown, QString());

    QString buildDir = info.directory;
    QtVersion *version = info.version;

    QList<ProjectExplorer::Task> issues = version->reportIssues(m_proFilePath, buildDir, false);

    QString text;
    ProjectExplorer::Task::TaskType highestType = ProjectExplorer::Task::Unknown;
    foreach (const ProjectExplorer::Task &t, issues) {
        if (!text.isEmpty())
            text.append(QLatin1String("<br>"));
        // set severity:
        QString severity;
        if (t.type == ProjectExplorer::Task::Error) {
            highestType = ProjectExplorer::Task::Error;
            severity = tr("<b>Error:</b> ", "Severity is Task::Error");
        } else if (t.type == ProjectExplorer::Task::Warning) {
            if (highestType == ProjectExplorer::Task::Unknown)
                highestType = ProjectExplorer::Task::Warning;
            severity = tr("<b>Warning:</b> ", "Severity is Task::Warning");
        }
        text.append(severity + t.description);
    }
    if (!text.isEmpty())
        text = QLatin1String("<nobr>") + text;
    return qMakePair(highestType, text);
}

// -----------------------
// BuildConfigurationInfo
// -----------------------

QList<BuildConfigurationInfo> BuildConfigurationInfo::filterBuildConfigurationInfos(const QList<BuildConfigurationInfo> &infos, const QString &id)
{
    QList<BuildConfigurationInfo> result;
    foreach (const BuildConfigurationInfo &info, infos)
        if (info.version->supportsTargetId(id))
            result.append(info);
    return result;
}

QList<BuildConfigurationInfo> BuildConfigurationInfo::importBuildConfigurations(const QString &proFilePath)
{
    QList<BuildConfigurationInfo> result;

    // Check for in source build first
    QString sourceDir = QFileInfo(proFilePath).absolutePath();
    BuildConfigurationInfo info = checkForBuild(sourceDir, proFilePath);
    if (info.isValid())
        result.append(info);

    // If we found a in source build, we do not search for out of source builds
    if (!result.isEmpty())
        return result;

    // Check for builds in build directoy
    QList<Qt4BaseTargetFactory *> factories =
            ExtensionSystem::PluginManager::instance()->getObjects<Qt4BaseTargetFactory>();
    QString defaultTopLevelBuildDirectory = Qt4Project::defaultTopLevelBuildDirectory(proFilePath);
    foreach (Qt4BaseTargetFactory *factory, factories) {
        foreach (const QString &id, factory->supportedTargetIds(0)) {
            QString expectedBuild = factory->defaultShadowBuildDirectory(defaultTopLevelBuildDirectory, id);
            BuildConfigurationInfo info = checkForBuild(expectedBuild, proFilePath);
            if (info.isValid())
                result.append(info);
        }
    }
    return result;
}

BuildConfigurationInfo BuildConfigurationInfo::checkForBuild(const QString &directory, const QString &proFilePath)
{
    QString makefile = directory + "/Makefile";
    QString qmakeBinary = QtVersionManager::findQMakeBinaryFromMakefile(makefile);
    if (qmakeBinary.isEmpty())
        return BuildConfigurationInfo();
    if (!QtVersionManager::makefileIsFor(makefile, proFilePath))
        return BuildConfigurationInfo();

    bool temporaryQtVersion = false;
    QtVersion *version = QtVersionManager::instance()->qtVersionForQMakeBinary(qmakeBinary);
    if (!version) {
        version = new QtVersion(qmakeBinary);
        temporaryQtVersion = true;
    }

    QPair<QtVersion::QmakeBuildConfigs, QString> makefileBuildConfig =
            QtVersionManager::scanMakeFile(makefile, version->defaultBuildConfig());

    QString additionalArguments = makefileBuildConfig.second;
    QString parsedSpec = Qt4BuildConfiguration::extractSpecFromArguments(&additionalArguments, directory, version);
    QString versionSpec = version->mkspec();

    QString specArgument;
    // Compare mkspecs and add to additional arguments
    if (parsedSpec.isEmpty() || parsedSpec == versionSpec || parsedSpec == "default") {
        // using the default spec, don't modify additional arguments
    } else {
        specArgument = "-spec " + Utils::QtcProcess::quoteArg(parsedSpec);
    }
    Utils::QtcProcess::addArgs(&specArgument, additionalArguments);


    BuildConfigurationInfo info = BuildConfigurationInfo(version,
                                                         makefileBuildConfig.first,
                                                         specArgument,
                                                         directory,
                                                         true,
                                                         temporaryQtVersion);
    return info;
}
