/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
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
#include "qt4nodes.h"
#include "qt4projectconfigwidget.h"
#include "qt4projectmanagerconstants.h"
#include "qt4buildconfiguration.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>
#include <qtsupport/qtversionfactory.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/pathchooser.h>
#include <utils/detailswidget.h>
#include <utils/qtcprocess.h>

#include <QtCore/QCoreApplication>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QVBoxLayout>

#include <algorithm>

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
                                                                    const QtSupport::QtVersionNumber &minimumQtVersion,
                                                                    const QtSupport::QtVersionNumber &maximumQtVersion,
                                                                    bool importEnabled,
                                                                    QList<BuildConfigurationInfo> importInfos)
{
    QList<BuildConfigurationInfo> infos = this->availableBuildConfigurations(id, proFilePath, minimumQtVersion, maximumQtVersion);
    if (infos.isEmpty())
        return 0;
    const bool supportsShadowBuilds
            = targetFeatures(id).contains(Constants::SHADOWBUILD_TARGETFEATURE_ID);
    Qt4DefaultTargetSetupWidget *widget
            = new Qt4DefaultTargetSetupWidget(this, id, proFilePath, infos, minimumQtVersion, maximumQtVersion,
                                              importEnabled && supportsShadowBuilds, importInfos,
                                              (supportsShadowBuilds
                                               ? Qt4DefaultTargetSetupWidget::ENABLE
                                               : Qt4DefaultTargetSetupWidget::DISABLE));
    return widget;
}

ProjectExplorer::Target *Qt4BaseTargetFactory::create(ProjectExplorer::Project *parent, const QString &id, Qt4TargetSetupWidget *widget)
{
    if (!widget->isTargetSelected())
        return 0;
    Q_ASSERT(qobject_cast<Qt4DefaultTargetSetupWidget *>(widget));
    Qt4DefaultTargetSetupWidget *w = static_cast<Qt4DefaultTargetSetupWidget *>(widget);
    w->storeSettings();
    return create(parent, id, w->buildConfigurationInfos());
}

QList<BuildConfigurationInfo> Qt4BaseTargetFactory::availableBuildConfigurations(const QString &id, const QString &proFilePath,
                                                                                 const QtSupport::QtVersionNumber &minimumQtVersion,
                                                                                 const QtSupport::QtVersionNumber &maximumQtVersion)
{
    QList<BuildConfigurationInfo> infoList;
    QList<QtSupport::BaseQtVersion *> knownVersions
            = QtSupport::QtVersionManager::instance()->versionsForTargetId(id, minimumQtVersion, maximumQtVersion);

    foreach (QtSupport::BaseQtVersion *version, knownVersions) {
        if (!version->isValid() || !version->toolChainAvailable(id))
            continue;
        QtSupport::BaseQtVersion::QmakeBuildConfigs config = version->defaultBuildConfig();
        BuildConfigurationInfo info = BuildConfigurationInfo(version, config, QString(), QString(), false, false);
        info.directory = shadowBuildDirectory(proFilePath, id, msgBuildConfigurationName(info));
        infoList.append(info);

        info.buildConfig = config ^ QtSupport::BaseQtVersion::DebugBuild;
        info.directory = shadowBuildDirectory(proFilePath, id, msgBuildConfigurationName(info));
        infoList.append(info);
    }
    return infoList;
}

QString sanitize(const QString &input)
{
    QString result;
    result.reserve(input.size());
    foreach (const QChar &c, input) {
        if ((c >= 'a' && c <='z')
                || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9')
                || c == '-'
                || c == '_')
            result.append(c);
        else
            result.append('_');
    }
    return result;
}

QString projectDirectory(const QString &proFile)
{
    if (proFile.isEmpty())
        return QString();
    QFileInfo info(proFile);
    return info.absoluteDir().path();
}

QString Qt4BaseTargetFactory::shadowBuildDirectory(const QString &profilePath, const QString &id, const QString &suffix)
{
    if (profilePath.isEmpty())
        return QString();
    QFileInfo info(profilePath);

    QString base = QDir::cleanPath(projectDirectory(profilePath) + QLatin1String("/../") + info.baseName() + QLatin1String("-build-"));
    return base + buildNameForId(id) + QLatin1String("-") + sanitize(suffix);
}

QString Qt4BaseTargetFactory::buildNameForId(const QString &id) const
{
    Q_UNUSED(id);
    return QString();
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

QList<Qt4BaseTargetFactory *> Qt4BaseTargetFactory::qt4BaseTargetFactoriesForIds(const QStringList &ids)
{
    QList<Qt4BaseTargetFactory *> factories;
    foreach (const QString &id, ids)
        if (Qt4BaseTargetFactory *factory = qt4BaseTargetFactoryForId(id))
            factories << factory;

    qSort(factories);
    factories.erase(std::unique(factories.begin(), factories.end()), factories.end());
    return factories;
}

// Return name of a build configuration.
QString Qt4BaseTargetFactory::msgBuildConfigurationName(const BuildConfigurationInfo &info)
{
    const QString qtVersionName = info.version->displayName();
    return (info.buildConfig & QtSupport::BaseQtVersion::DebugBuild) ?
        //: Name of a debug build configuration to created by a project wizard, %1 being the Qt version name. We recommend not translating it.
        tr("%1 Debug").arg(qtVersionName) :
        //: Name of a release build configuration to be created by a project wizard, %1 being the Qt version name. We recommend not translating it.
        tr("%1 Release").arg(qtVersionName);
}

QList<ProjectExplorer::Task> Qt4BaseTargetFactory::reportIssues(const QString &proFile)
{
    Q_UNUSED(proFile);
    return QList<ProjectExplorer::Task>();
}

bool Qt4BaseTargetFactory::selectByDefault(const QString &id) const
{
    Q_UNUSED(id);
    return true;
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

Qt4BuildConfiguration *Qt4BaseTarget::activeQt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(Target::activeBuildConfiguration());
}

Qt4Project *Qt4BaseTarget::qt4Project() const
{
    return static_cast<Qt4Project *>(project());
}

QList<ProjectExplorer::ToolChain *> Qt4BaseTarget::possibleToolChains(ProjectExplorer::BuildConfiguration *bc) const
{
    QList<ProjectExplorer::ToolChain *> tmp;
    QList<ProjectExplorer::ToolChain *> result;

    Qt4BuildConfiguration *qt4bc = qobject_cast<Qt4BuildConfiguration *>(bc);
    if (!qt4bc || !qt4bc->qtVersion())
        return tmp;

    QList<Qt4ProFileNode *> profiles = qt4Project()->allProFiles();
    bool qtUsed = false;
    foreach (Qt4ProFileNode *pro, profiles) {
        if (!pro->variableValue(QtVar).isEmpty()) {
            qtUsed = true;
            break;
        }
    }

    if (!qtUsed || !qt4bc->qtVersion()->isValid())
        return ProjectExplorer::ToolChainManager::instance()->toolChains();

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

ProjectExplorer::ToolChain *Qt4BaseTarget::preferredToolChain(ProjectExplorer::BuildConfiguration *bc) const
{
    Qt4BuildConfiguration *qtBc = qobject_cast<Qt4BuildConfiguration *>(bc);
    if (!qtBc || !qtBc->qtVersion())
        return Target::preferredToolChain(bc);

    QList<ProjectExplorer::ToolChain *> tcs = possibleToolChains(bc);
    const Utils::FileName mkspec = qtBc->qtVersion()->mkspec();
    foreach (ProjectExplorer::ToolChain *tc, tcs)
        if (tc->mkspec() == mkspec)
            return tc;
    return tcs.isEmpty() ? 0 : tcs.at(0);
}

Utils::FileName Qt4BaseTarget::mkspec(const Qt4BuildConfiguration *bc) const
{
    QtSupport::BaseQtVersion *version = bc->qtVersion();
    // We do not know which abi the Qt version has, so let's stick with the defaults
    if (version && version->qtAbis().count() == 1 && version->qtAbis().first().isNull())
        return Utils::FileName();

    const Utils::FileName tcSpec = bc->toolChain() ? bc->toolChain()->mkspec() : Utils::FileName();
    if (!version)
        return tcSpec;
    if (!tcSpec.isEmpty() && version->hasMkspec(tcSpec))
        return tcSpec;
    return version->mkspec();
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

Qt4BuildConfiguration *Qt4BaseTarget::addQt4BuildConfiguration(QString defaultDisplayName,
                                                           QString displayName, QtSupport::BaseQtVersion *qtversion,
                                                           QtSupport::BaseQtVersion::QmakeBuildConfigs qmakeBuildConfiguration,
                                                           QString additionalArguments,
                                                           QString directory,
                                                           bool importing)
{
    Q_ASSERT(qtversion);
    bool debug = qmakeBuildConfiguration & QtSupport::BaseQtVersion::DebugBuild;

    // Add the buildconfiguration
    Qt4BuildConfiguration *bc = new Qt4BuildConfiguration(this);
    bc->setDefaultDisplayName(defaultDisplayName);
    bc->setDisplayName(displayName);

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

    bool enableQmlDebugger
            = Qt4BuildConfiguration::removeQMLInspectorFromArguments(&additionalArguments);
    if (!additionalArguments.isEmpty())
        qmakeStep->setUserArguments(additionalArguments);
    if (importing)
        qmakeStep->setLinkQmlDebuggingLibrary(enableQmlDebugger);

    // set some options for qmake and make
    if (qmakeBuildConfiguration & QtSupport::BaseQtVersion::BuildAll) // debug_and_release => explicit targets
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
                                                         const QtSupport::QtVersionNumber &minimumQtVersion,
                                                         const QtSupport::QtVersionNumber &maximumQtVersion,
                                                         bool importEnabled,
                                                         const QList<BuildConfigurationInfo> &importInfos,
                                                         ShadowBuildOption shadowBuild)
    : Qt4TargetSetupWidget(),
      m_id(id),
      m_factory(factory),
      m_proFilePath(proFilePath),
      m_minimumQtVersion(minimumQtVersion),
      m_maximumQtVersion(maximumQtVersion),
      m_importInfos(importInfos),
      m_directoriesEnabled(true),
      m_hasInSourceBuild(false),
      m_ignoreChange(false),
      m_showImport(importEnabled),
      m_buildConfigurationTemplateUnchanged(true),
      m_shadowBuildCheckBoxVisible(false),
      m_selected(0),
      m_qtVersionId(-1)
{
    QSettings *s = Core::ICore::instance()->settings();
    QString sourceDir = QFileInfo(m_proFilePath).absolutePath();

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QVBoxLayout *vboxLayout = new QVBoxLayout();
    setLayout(vboxLayout);
    vboxLayout->setContentsMargins(0, 0, 0, 0);
    m_detailsWidget = new Utils::DetailsWidget(this);
    m_detailsWidget->setUseCheckBox(true);
    m_detailsWidget->setSummaryText(factory->displayNameForId(id));
    m_detailsWidget->setChecked(false);
    m_detailsWidget->setSummaryFontBold(true);
    m_detailsWidget->setIcon(factory->iconForId(id));
    m_detailsWidget->setAdditionalSummaryText(issuesListToString(factory->reportIssues(m_proFilePath)));
    vboxLayout->addWidget(m_detailsWidget);

    QWidget *widget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    widget->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);

    QWidget *w = new QWidget;
    m_importLayout = new QGridLayout;
    m_importLayout->setMargin(0);
    w->setLayout(m_importLayout);
    layout->addWidget(w);

    w = new QWidget;
    m_importLineLayout = new QHBoxLayout();
    m_importLineLayout->setContentsMargins(0, 0, 0, 0);
    w->setLayout(m_importLineLayout);
    m_importLineLabel = new QLabel();
    m_importLineLabel->setText(tr("Add build from:"));
    m_importLineLayout->addWidget(m_importLineLabel);

    m_importLinePath = new Utils::PathChooser();
    m_importLinePath->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_importLinePath->setPath(sourceDir);
    m_importLinePath->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_importLineLayout->addWidget(m_importLinePath);

    m_importLineButton = new QPushButton;
    m_importLineButton->setText(tr("Add Build"));
    m_importLineButton->setAttribute(Qt::WA_MacSmallSize);
    // make it in line with import path chooser button on mac
    m_importLineButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    m_importLineLayout->addWidget(m_importLineButton);
    m_importLineStretch = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_importLineLayout->addSpacerItem(m_importLineStretch);
    layout->addWidget(w);

    m_importLineLabel->setVisible(false);
    m_importLinePath->setVisible(false);
    m_importLineButton->setVisible(m_showImport);

    m_buildConfigurationLabel = new QLabel;
    m_buildConfigurationLabel->setText(tr("Create Build Configurations:"));
    m_buildConfigurationLabel->setVisible(false);

    m_buildConfigurationComboBox = new QComboBox;
    m_buildConfigurationComboBox->addItem(tr("For Each Qt Version One Debug And One Release"), PERQT);
    m_buildConfigurationComboBox->addItem(tr("For One Qt Version One Debug And One Release"), ONEQT);
    m_buildConfigurationComboBox->addItem(tr("Manually"), MANUALLY);
    m_buildConfigurationComboBox->addItem(tr("None"), NONE);

    if (m_importInfos.isEmpty())
        m_buildConfigurationComboBox->setCurrentIndex(s->value("Qt4ProjectManager.TargetSetupPage.BuildTemplate", 0).toInt());
    else
        m_buildConfigurationComboBox->setCurrentIndex(3); // NONE

    m_buildConfigurationComboBox->setVisible(false);

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->addWidget(m_buildConfigurationLabel);
    hbox->addWidget(m_buildConfigurationComboBox);
    hbox->addStretch();
    layout->addLayout(hbox);

    m_shadowBuildEnabled = new QCheckBox;
    m_shadowBuildEnabled->setText(tr("Use Shadow Building"));
    m_shadowBuildCheckBoxVisible = shadowBuild == USER;

    layout->addWidget(m_shadowBuildEnabled);
    m_shadowBuildEnabled->setVisible(m_shadowBuildCheckBoxVisible);

    m_versionLabel = new QLabel;
    m_versionLabel->setText(tr("Qt Version:"));
    m_versionLabel->setVisible(false);
    m_versionComboBox = new QComboBox;
    m_versionComboBox->setVisible(false);
    hbox = new QHBoxLayout();
    hbox->addWidget(m_versionLabel);
    hbox->addWidget(m_versionComboBox);
    hbox->addStretch();
    layout->addLayout(hbox);

    w = new QWidget;
    m_newBuildsLayout = new QGridLayout;
    m_newBuildsLayout->setMargin(0);
#ifdef Q_WS_MAC
    m_newBuildsLayout->setSpacing(0);
#endif
    w->setLayout(m_newBuildsLayout);
    layout->addWidget(w);

    widget->setEnabled(false);
    m_detailsWidget->setWidget(widget);

    for (int i = 0; i < m_importInfos.size(); ++i) {
        if (m_importInfos.at(i).directory == sourceDir)
            m_hasInSourceBuild = true;
        m_importEnabled << true;
    }

    if (m_hasInSourceBuild || shadowBuild == DISABLE) {
        m_shadowBuildEnabled->setChecked(false);
        m_directoriesEnabled = false;
    } else if (shadowBuild == ENABLE) {
        m_shadowBuildEnabled->setChecked(true);
        m_directoriesEnabled = true;
    } else {
        m_directoriesEnabled = s->value("Qt4ProjectManager.TargetSetupPage.ShadowBuilding", true).toBool();
        m_shadowBuildEnabled->setChecked(m_directoriesEnabled);
    }

    m_selected += m_importInfos.size();

    setupImportWidgets();

    setBuildConfigurationInfos(infos);

    int qtVersionId = s->value("Qt4ProjectManager.TargetSetupPage.QtVersionId", -1).toInt();
    int index = m_versionComboBox->findData(qtVersionId);
    if (index != -1)
        m_versionComboBox->setCurrentIndex(index);
    qtVersionChanged();

    if (!m_importInfos.isEmpty())
        m_detailsWidget->setState(Utils::DetailsWidget::Expanded);

    connect(m_importLineButton, SIGNAL(clicked()),
            this, SLOT(addImportClicked()));

    connect(m_detailsWidget, SIGNAL(checked(bool)),
            this, SLOT(targetCheckBoxToggled(bool)));
    connect(m_shadowBuildEnabled, SIGNAL(toggled(bool)),
            this, SLOT(shadowBuildingToggled()));
    connect(m_buildConfigurationComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(buildConfigurationComboBoxChanged()));
    connect(m_versionComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(qtVersionChanged()));
}

Qt4DefaultTargetSetupWidget::~Qt4DefaultTargetSetupWidget()
{

}

bool Qt4DefaultTargetSetupWidget::isTargetSelected() const
{
    if (!m_detailsWidget->isChecked())
        return false;

    return !buildConfigurationInfos().isEmpty();
}

void Qt4DefaultTargetSetupWidget::setTargetSelected(bool b)
{
    // Only check target if there are build configurations possible
    b &= !buildConfigurationInfos().isEmpty();
    m_ignoreChange = true;
    m_detailsWidget->setChecked(b);
    m_detailsWidget->widget()->setEnabled(b);
    m_ignoreChange = false;
    // We want the shadow build option to be visible
    if (b && (m_shadowBuildEnabled->isVisibleTo(m_shadowBuildEnabled->parentWidget())
              || m_buildConfigurationComboBox->isVisibleTo(m_buildConfigurationComboBox->parentWidget())))
        m_detailsWidget->setState(Utils::DetailsWidget::Expanded);
}

void Qt4DefaultTargetSetupWidget::targetCheckBoxToggled(bool b)
{
    if (m_ignoreChange)
        return;
    m_detailsWidget->widget()->setEnabled(b);
    if (b) {
        foreach (bool error, m_issues) {
            if (error) {
                m_detailsWidget->setState(Utils::DetailsWidget::Expanded);
                break;
            }
        }
    }
    emit selectedToggled();
}

QString Qt4DefaultTargetSetupWidget::displayNameFrom(const BuildConfigurationInfo &info)
{
    QString buildType;
    if ((info.buildConfig & QtSupport::BaseQtVersion::BuildAll) == 0) {
        if (info.buildConfig & QtSupport::BaseQtVersion::DebugBuild)
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
    setBuildConfigurationInfos(m_factory->availableBuildConfigurations(m_id, proFilePath, m_minimumQtVersion, m_maximumQtVersion), false);
}

void Qt4DefaultTargetSetupWidget::setBuildConfiguraionComboBoxVisible(bool b)
{
    m_buildConfigurationLabel->setVisible(b);
    m_buildConfigurationComboBox->setVisible(b);
    updateWidgetVisibility();
}


Qt4DefaultTargetSetupWidget::BuildConfigurationTemplate Qt4DefaultTargetSetupWidget::buildConfigurationTemplate() const
{
    if (!m_buildConfigurationComboBox->isVisibleTo(m_buildConfigurationComboBox->parentWidget()))
        return MANUALLY; // the default
    return static_cast<BuildConfigurationTemplate>(m_buildConfigurationComboBox->itemData(m_buildConfigurationComboBox->currentIndex()).toInt());
}

void Qt4DefaultTargetSetupWidget::setBuildConfigurationTemplate(Qt4DefaultTargetSetupWidget::BuildConfigurationTemplate value)
{
    int index = m_buildConfigurationComboBox->findData(QVariant::fromValue(int(value)));
    m_buildConfigurationComboBox->setCurrentIndex(index);
}

void Qt4DefaultTargetSetupWidget::storeSettings() const
{
    bool importing = false;
    for (int i=0; i < m_importEnabled.size(); ++i) {
        if (m_importEnabled.at(i)) {
            importing = true;
            break;
        }
    }

    QSettings *s = Core::ICore::instance()->settings();
    s->setValue("Qt4ProjectManager.TargetSetupPage.ShadowBuilding", m_shadowBuildEnabled->isChecked());
    int id = -1;
    int ci = m_versionComboBox->currentIndex();
    if (ci != -1)
        id = m_versionComboBox->itemData(ci).toInt();
    s->setValue("Qt4ProjectManager.TargetSetupPage.QtVersionId", id);

    // if we are importing we don't save the template setting
    // essentially we assume that the settings apply for the new project case
    // and for the importing case "None" is likely the most sensible
    if (!importing)
        s->setValue("Qt4ProjectManager.TargetSetupPage.BuildTemplate", m_buildConfigurationComboBox->currentIndex());
}

QList<BuildConfigurationInfo> Qt4DefaultTargetSetupWidget::buildConfigurationInfos() const
{
    QList<BuildConfigurationInfo> infos;
    for (int i = 0; i < m_importInfos.size(); ++i) {
        if (m_importEnabled.at(i))
            infos << m_importInfos.at(i);
    }

    BuildConfigurationTemplate state = buildConfigurationTemplate();
    if (state == NONE)
        return infos;

    int qtVersionId = -1;
    if (state == ONEQT && m_versionComboBox->currentIndex() != -1)
        qtVersionId = m_versionComboBox->itemData(m_versionComboBox->currentIndex()).toInt();

    QString sourceDir = QFileInfo(m_proFilePath).absolutePath();
    int size = m_infos.size();
    for (int i=0; i < size; ++i) {
        if (state == PERQT || (m_enabled.at(i)  && (state == MANUALLY || (state == ONEQT && m_infos.at(i).version->uniqueId() == qtVersionId)))) {
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
        m_importLineButton->setAttribute(Qt::WA_MacNormalSize);
        m_importLineStretch->changeSize(0,0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_importLineLayout->invalidate();
        return;
    }

    QList<BuildConfigurationInfo> infos = BuildConfigurationInfo::checkForBuild(m_importLinePath->path(), m_proFilePath);
    if (infos.isEmpty()) {
        QMessageBox::critical(this,
                              tr("No build found"),
                              tr("No build found in %1 matching project %2.").arg(m_importLinePath->path()).arg(m_proFilePath));
        return;
    }

    QList<BuildConfigurationInfo> filterdInfos;
    bool filtered = false;
    foreach (const BuildConfigurationInfo &info, infos) {
        if (info.version->supportsTargetId(m_id))
            filterdInfos << info;
        else
            filtered = true;
    }

    if (filtered) {
        if (filterdInfos.isEmpty()) {
            QMessageBox::critical(this,
                                  tr("Incompatible build found"),
                                  tr("The build found in %1 is incompatible with this target").arg(m_importLinePath->path()));
            return;
        }
        // show something if we found incompatible builds?
    }

    // Filter out already imported builds
    infos = filterdInfos;
    filterdInfos.clear();
    foreach (const BuildConfigurationInfo &info, infos) {
        bool alreadyImported = false;
        foreach (const BuildConfigurationInfo &existingImport, m_importInfos) {
            if (info == existingImport) {
                alreadyImported = true;
                break;
            }
        }
        if (!alreadyImported)
            filterdInfos << info;
    }

    if (filterdInfos.isEmpty() && !infos.isEmpty()) {
        QMessageBox::critical(this,
                              tr("Already imported build"),
                              tr("The build found in %1 is already imported").arg(m_importLinePath->path()));
        return;
    }

    // We switch from to "NONE" on importing if the user has not changed it
    if (m_buildConfigurationTemplateUnchanged)
        setBuildConfigurationTemplate(NONE);

    foreach (const BuildConfigurationInfo &info, filterdInfos) {
        ++m_selected;
        m_importEnabled << true;

        m_importInfos << info;

        createImportWidget(info, m_importEnabled.size() - 1);
        emit newImportBuildConfiguration(info);
    }
    emit selectedToggled();
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
        if (foundIssues && isTargetSelected())
            m_detailsWidget->setState(Utils::DetailsWidget::Expanded);
    }

    // update version combobox
    int oldQtVersionId = -1;
    if (m_versionComboBox->currentIndex() != -1)
        oldQtVersionId = m_versionComboBox->itemData(m_versionComboBox->currentIndex()).toInt();
    QList<QtSupport::BaseQtVersion *> list;
    foreach (const BuildConfigurationInfo &info, m_infos) {
        if (!list.contains(info.version))
            list << info.version;
    }
    m_ignoreChange = true;
    m_versionComboBox->clear();
    foreach (QtSupport::BaseQtVersion *v, list) {
        m_versionComboBox->addItem(v->displayName(), v->uniqueId());
        if (v->uniqueId() == oldQtVersionId)
            m_versionComboBox->setCurrentIndex(m_versionComboBox->count() - 1);
    }
    m_ignoreChange = false;
    updateWidgetVisibility();
}

void Qt4DefaultTargetSetupWidget::setupImportWidgets()
{
    for (int i = 0; i < m_importInfos.size(); ++i)
        createImportWidget(m_importInfos.at(i), i);
}

void Qt4DefaultTargetSetupWidget::createImportWidget(const BuildConfigurationInfo &info, int pos)
{
    QCheckBox *checkBox = new QCheckBox;
    checkBox->setText(tr("Import build from %1").arg(QDir::toNativeSeparators(info.directory)));
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
        checkbox->setAttribute(Qt::WA_LayoutUsesWidgetRect);
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
        reportIssuesLabel->setVisible(false);

        connect(checkbox, SIGNAL(toggled(bool)),
                this, SLOT(checkBoxToggled(bool)));

        connect(pathChooser, SIGNAL(changed(QString)),
                this, SLOT(pathChanged()));

        m_checkboxes.append(checkbox);
        m_pathChoosers.append(pathChooser);
        m_reportIssuesLabels.append(reportIssuesLabel);
        m_issues.append(false);
        bool issue = reportIssues(i);
        foundIssues |= issue;
    }
    if (foundIssues && isTargetSelected())
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
    m_issues.clear();
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

void Qt4DefaultTargetSetupWidget::buildConfigurationComboBoxChanged()
{
    m_buildConfigurationTemplateUnchanged = false;
    updateWidgetVisibility();
}

void Qt4DefaultTargetSetupWidget::updateWidgetVisibility()
{
    m_versionLabel->setVisible(false);
    m_versionComboBox->setVisible(false);
    BuildConfigurationTemplate state = buildConfigurationTemplate();
    if (state == NONE || state == PERQT) {
        foreach (QCheckBox *cb, m_checkboxes)
            cb->setVisible(false);
        foreach (Utils::PathChooser *pc, m_pathChoosers)
            pc->setVisible(false);
        foreach (QLabel *label, m_reportIssuesLabels)
            label->setVisible(false);
    } else if (state == MANUALLY) {
        foreach (QCheckBox *cb, m_checkboxes)
            cb->setVisible(true);
        foreach (Utils::PathChooser *pc, m_pathChoosers)
            pc->setVisible(true);
        for (int i = 0; i < m_reportIssuesLabels.count(); ++i)
            m_reportIssuesLabels.at(i)->setVisible(m_issues.at(i));
    } else if (state == ONEQT) {
        m_versionLabel->setVisible(true);
        m_versionComboBox->setVisible(true);
        qtVersionChanged();
    }
    m_shadowBuildEnabled->setVisible(m_shadowBuildCheckBoxVisible && (state != NONE));
    emit selectedToggled();
}

void Qt4DefaultTargetSetupWidget::qtVersionChanged()
{
    if (m_ignoreChange)
        return;
    int id = -1;
    if (m_versionComboBox->currentIndex() != -1)
        id = m_versionComboBox->itemData(m_versionComboBox->currentIndex()).toInt();
    if (buildConfigurationTemplate() != ONEQT)
        return;
    for (int i = 0; i < m_infos.size(); ++i) {
        bool visible = m_infos.at(i).version->uniqueId() == id;
        m_checkboxes.at(i)->setVisible(visible);
        m_pathChoosers.at(i)->setVisible(visible);
        m_reportIssuesLabels.at(i)->setVisible(m_issues.at(i));
    }
}

bool Qt4DefaultTargetSetupWidget::reportIssues(int index)
{
    QPair<ProjectExplorer::Task::TaskType, QString> issues = findIssues(m_infos.at(index));
    QLabel *reportIssuesLabel = m_reportIssuesLabels.at(index);
    reportIssuesLabel->setText(issues.second);
    bool error = issues.first != ProjectExplorer::Task::Unknown;
    reportIssuesLabel->setVisible(error);
    m_issues[index] = error;
    return error;
}

QPair<ProjectExplorer::Task::TaskType, QString> Qt4DefaultTargetSetupWidget::findIssues(const BuildConfigurationInfo &info)
{
    if (m_proFilePath.isEmpty())
        return qMakePair(ProjectExplorer::Task::Unknown, QString());

    QString buildDir = info.directory;
    if (!m_shadowBuildEnabled->isChecked())
        buildDir = QFileInfo(m_proFilePath).absolutePath();
    QtSupport::BaseQtVersion *version = info.version;

    QList<ProjectExplorer::Task> issues = version->reportIssues(m_proFilePath, buildDir);

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
    QList<BuildConfigurationInfo> infos = checkForBuild(sourceDir, proFilePath);
    if (!infos.isEmpty())
        result.append(infos);

    // If we found a in source build, we do not search for out of source builds
    if (!result.isEmpty())
        return result;

    // Check for builds in build directory
    QList<Qt4BaseTargetFactory *> factories =
            ExtensionSystem::PluginManager::instance()->getObjects<Qt4BaseTargetFactory>();
    foreach (Qt4BaseTargetFactory *factory, factories) {
        foreach (const QString &id, factory->supportedTargetIds(0)) {
            QString expectedBuildprefix = factory->shadowBuildDirectory(proFilePath, id, "");
            QString baseDir = QFileInfo(expectedBuildprefix).absolutePath();
            foreach (const QString &dir, QDir(baseDir).entryList()) {
                if (dir.startsWith(expectedBuildprefix)) {
                    QList<BuildConfigurationInfo> infos = checkForBuild(dir, proFilePath);
                    if (infos.isEmpty())
                        result.append(infos);
                }
            }
        }
    }
    return result;
}

QList<BuildConfigurationInfo> BuildConfigurationInfo::checkForBuild(const QString &directory, const QString &proFilePath)
{
    QStringList makefiles = QDir(directory).entryList(QStringList() << "Makefile*");
    QList<BuildConfigurationInfo> infos;
    foreach (const QString &file, makefiles) {
        QString makefile = directory + '/' + file;
        Utils::FileName qmakeBinary = QtSupport::QtVersionManager::findQMakeBinaryFromMakefile(makefile);
        if (qmakeBinary.isEmpty())
            continue;
        if (QtSupport::QtVersionManager::makefileIsFor(makefile, proFilePath) != QtSupport::QtVersionManager::SameProject)
            continue;

        bool temporaryQtVersion = false;
        QtSupport::BaseQtVersion *version = QtSupport::QtVersionManager::instance()->qtVersionForQMakeBinary(qmakeBinary);
        if (!version) {
            version = QtSupport::QtVersionFactory::createQtVersionFromQMakePath(qmakeBinary);
            temporaryQtVersion = true;
            if (!version)
                continue;
        }

        QPair<QtSupport::BaseQtVersion::QmakeBuildConfigs, QString> makefileBuildConfig =
                QtSupport::QtVersionManager::scanMakeFile(makefile, version->defaultBuildConfig());

        QString additionalArguments = makefileBuildConfig.second;
        Utils::FileName parsedSpec = Qt4BuildConfiguration::extractSpecFromArguments(&additionalArguments, directory, version);
        Utils::FileName versionSpec = version->mkspec();

        QString specArgument;
        // Compare mkspecs and add to additional arguments
        if (parsedSpec.isEmpty() || parsedSpec == versionSpec || parsedSpec == Utils::FileName::fromString("default")) {
            // using the default spec, don't modify additional arguments
        } else {
            specArgument = "-spec " + Utils::QtcProcess::quoteArg(parsedSpec.toUserOutput());
        }
        Utils::QtcProcess::addArgs(&specArgument, additionalArguments);

        BuildConfigurationInfo info = BuildConfigurationInfo(version,
                                                             makefileBuildConfig.first,
                                                             specArgument,
                                                             directory,
                                                             true,
                                                             temporaryQtVersion);
        infos.append(info);
    }
    return infos;
}
