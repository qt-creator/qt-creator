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

#include "qmakeprojectconfigwidget.h"

#include "qmakeproject.h"
#include "qmakebuildconfiguration.h"
#include "qmakenodes.h"
#include "qmakesettings.h"

#include <QBoxLayout>
#include <coreplugin/coreicons.h>
#include <coreplugin/variablechooser.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/utilsicons.h>

using namespace QmakeProjectManager;
using namespace QmakeProjectManager::Internal;
using namespace ProjectExplorer;

/// returns whether this is a shadow build configuration or not
/// note, even if shadowBuild() returns true, it might be using the
/// source directory as the shadow build directory, thus it
/// still is a in-source build
static bool isShadowBuild(BuildConfiguration *bc)
{
    return bc->buildDirectory() != bc->target()->project()->projectDirectory();
}

QmakeProjectConfigWidget::QmakeProjectConfigWidget(QmakeBuildConfiguration *bc)
    : NamedWidget(),
      m_buildConfiguration(bc)
{
    Project *project = bc->target()->project();

    m_defaultShadowBuildDir
            = QmakeBuildConfiguration::shadowBuildDirectory(project->projectFilePath(),
                                                            bc->target()->kit(),
                                                            Utils::FileUtils::qmakeFriendlyName(bc->displayName()),
                                                            bc->buildType());

    m_detailsContainer = new Utils::DetailsWidget(this);
    m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);

    auto vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->addWidget(m_detailsContainer);

    auto details = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(details);

    shadowBuildLabel = new QLabel(details);
    shadowBuildLabel->setText(tr("Shadow build:"));

    shadowBuildCheckBox = new QCheckBox(details);
    shadowBuildCheckBox->setChecked(isShadowBuild(m_buildConfiguration));

    buildDirLabel = new QLabel(details);
    buildDirLabel->setText(tr("Build directory:"));

    shadowBuildDirEdit = new Utils::PathChooser(details);

    inSourceBuildDirEdit = new Utils::PathChooser(details);

    warningLabel = new QLabel(details);
    warningLabel->setPixmap(Utils::Icons::WARNING.pixmap());

    problemLabel = new QLabel(details);
    QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy2.setHorizontalStretch(10);
    sizePolicy2.setVerticalStretch(0);
    problemLabel->setSizePolicy(sizePolicy2);
    problemLabel->setWordWrap(true);

    auto horizontalLayout_2 = new QHBoxLayout();
    horizontalLayout_2->addWidget(warningLabel);
    horizontalLayout_2->addWidget(problemLabel);

    auto horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(shadowBuildDirEdit);
    horizontalLayout->addWidget(inSourceBuildDirEdit);

    auto layout = new QGridLayout(details);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(shadowBuildLabel, 0, 0, 1, 1);
    layout->addWidget(shadowBuildCheckBox, 0, 1, 1, 1);
    layout->addWidget(buildDirLabel, 1, 0, 1, 1);
    layout->addLayout(horizontalLayout, 1, 1, 1, 1);
    layout->addLayout(horizontalLayout_2, 2, 1, 1, 1);

    problemLabel->setText(tr("problemLabel"));

    m_browseButton = shadowBuildDirEdit->buttonAtIndex(0);

    shadowBuildDirEdit->setPromptDialogTitle(tr("Shadow Build Directory"));
    shadowBuildDirEdit->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    shadowBuildDirEdit->setHistoryCompleter(QLatin1String("Qmake.BuildDir.History"));
    shadowBuildDirEdit->setEnvironment(bc->environment());
    shadowBuildDirEdit->setBaseFileName(project->projectDirectory());
    if (isShadowBuild(m_buildConfiguration)) {
        shadowBuildDirEdit->setPath(bc->rawBuildDirectory().toString());
        inSourceBuildDirEdit->setVisible(false);
    } else {
        shadowBuildDirEdit->setPath(m_defaultShadowBuildDir);
        shadowBuildDirEdit->setVisible(false);
    }
    inSourceBuildDirEdit->setFileName(project->projectDirectory());
    inSourceBuildDirEdit->setReadOnly(true);
    inSourceBuildDirEdit->setEnabled(false);

    auto chooser = new Core::VariableChooser(this);
    chooser->addSupportedWidget(shadowBuildDirEdit->lineEdit());

    connect(shadowBuildCheckBox, &QAbstractButton::clicked,
            this, &QmakeProjectConfigWidget::shadowBuildClicked);

    connect(shadowBuildDirEdit, &Utils::PathChooser::beforeBrowsing,
            this, &QmakeProjectConfigWidget::onBeforeBeforeShadowBuildDirBrowsed);

    connect(shadowBuildDirEdit, &Utils::PathChooser::rawPathChanged,
            this, &QmakeProjectConfigWidget::shadowBuildEdited);

    connect(bc, &BuildConfiguration::enabledChanged,
            this, &QmakeProjectConfigWidget::environmentChanged);

    auto qmakeProject = static_cast<QmakeProject *>(bc->target()->project());
    connect(qmakeProject, &QmakeProject::buildDirectoryInitialized,
            this, &QmakeProjectConfigWidget::updateProblemLabel);
    connect(qmakeProject, &Project::parsingFinished,
            this, &QmakeProjectConfigWidget::updateProblemLabel);
    connect(&QmakeSettings::instance(), &QmakeSettings::settingsChanged,
            this, &QmakeProjectConfigWidget::updateProblemLabel);

    connect(bc->target(), &Target::kitChanged, this, &QmakeProjectConfigWidget::updateProblemLabel);

    connect(m_buildConfiguration, &BuildConfiguration::buildDirectoryChanged,
            this, &QmakeProjectConfigWidget::buildDirectoryChanged);
    connect(m_buildConfiguration, &QmakeBuildConfiguration::qmakeBuildConfigurationChanged,
            this, &QmakeProjectConfigWidget::updateProblemLabel);

    setDisplayName(tr("General"));

    updateDetails();
    updateProblemLabel();
}

void QmakeProjectConfigWidget::updateDetails()
{
    m_detailsContainer->setSummaryText(
                tr("building in <b>%1</b>")
                .arg(m_buildConfiguration->buildDirectory().toUserOutput()));
}

void QmakeProjectConfigWidget::setProblemLabel(const QString &text)
{
    warningLabel->setVisible(!text.isEmpty());
    problemLabel->setVisible(!text.isEmpty());
    problemLabel->setText(text);
}

void QmakeProjectConfigWidget::environmentChanged()
{
    shadowBuildDirEdit->setEnvironment(m_buildConfiguration->environment());
}

void QmakeProjectConfigWidget::buildDirectoryChanged()
{
    if (m_ignoreChange)
        return;

    bool shadowBuild = shadowBuildCheckBox->isChecked();
    inSourceBuildDirEdit->setVisible(!shadowBuild);

    shadowBuildDirEdit->setVisible(shadowBuild);
    shadowBuildDirEdit->setEnabled(shadowBuild);
    m_browseButton->setEnabled(shadowBuild);

    shadowBuildDirEdit->setPath(m_buildConfiguration->rawBuildDirectory().toString());

    updateDetails();
    updateProblemLabel();
}

void QmakeProjectConfigWidget::onBeforeBeforeShadowBuildDirBrowsed()
{
    Utils::FilePath initialDirectory = m_buildConfiguration->target()->project()->projectDirectory();
    if (!initialDirectory.isEmpty())
        shadowBuildDirEdit->setInitialBrowsePathBackup(initialDirectory.toString());
}

void QmakeProjectConfigWidget::shadowBuildClicked(bool checked)
{
    shadowBuildDirEdit->setEnabled(checked);
    m_browseButton->setEnabled(checked);

    shadowBuildDirEdit->setVisible(checked);
    inSourceBuildDirEdit->setVisible(!checked);

    m_ignoreChange = true;
    if (checked)
        m_buildConfiguration->setBuildDirectory(Utils::FilePath::fromString(shadowBuildDirEdit->rawPath()));
    else
        m_buildConfiguration->setBuildDirectory(Utils::FilePath::fromString(inSourceBuildDirEdit->rawPath()));
    m_ignoreChange = false;

    updateDetails();
    updateProblemLabel();
}

void QmakeProjectConfigWidget::shadowBuildEdited()
{
    if (m_buildConfiguration->rawBuildDirectory().toString() == shadowBuildDirEdit->rawPath())
        return;

    m_ignoreChange = true;
    m_buildConfiguration->setBuildDirectory(Utils::FilePath::fromString(shadowBuildDirEdit->rawPath()));
    m_ignoreChange = false;
}

void QmakeProjectConfigWidget::updateProblemLabel()
{
    shadowBuildDirEdit->triggerChanged();
    ProjectExplorer::Kit *k = m_buildConfiguration->target()->kit();
    const QString proFileName = m_buildConfiguration->target()->project()->projectFilePath().toString();

    // Check for Qt version:
    QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(k);
    if (!version) {
        setProblemLabel(tr("This kit cannot build this project since it does not define a Qt version."));
        return;
    }

    auto *p = static_cast<QmakeProject *>(m_buildConfiguration->target()->project());
    if (p->rootProFile()->parseInProgress() || !p->rootProFile()->validParse()) {
        setProblemLabel(QString());
        return;
    }

    bool targetMismatch = false;
    bool incompatibleBuild = false;
    bool allGood = false;
    // we only show if we actually have a qmake and makestep
    QString errorString;
    if (m_buildConfiguration->qmakeStep() && m_buildConfiguration->makeStep()) {
        QString makefile = m_buildConfiguration->buildDirectory().toString() + QLatin1Char('/');
        if (m_buildConfiguration->makefile().isEmpty())
            makefile.append(QLatin1String("Makefile"));
        else
            makefile.append(m_buildConfiguration->makefile());

        switch (m_buildConfiguration->compareToImportFrom(makefile, &errorString)) {
        case QmakeBuildConfiguration::MakefileMatches:
            allGood = true;
            break;
        case QmakeBuildConfiguration::MakefileMissing:
            allGood = true;
            break;
        case QmakeBuildConfiguration::MakefileIncompatible:
            incompatibleBuild = true;
            break;
        case QmakeBuildConfiguration::MakefileForWrongProject:
            targetMismatch = true;
            break;
        }
    }

    const bool unalignedBuildDir = QmakeSettings::warnAgainstUnalignedBuildDir()
            && !m_buildConfiguration->isBuildDirAtSafeLocation();
    if (unalignedBuildDir)
        allGood = false;

    if (allGood) {
        QString buildDirectory = m_buildConfiguration->target()->project()->projectDirectory().toString();
        if (isShadowBuild(m_buildConfiguration))
            buildDirectory = m_buildConfiguration->buildDirectory().toString();
        Tasks issues;
        issues = version->reportIssues(proFileName, buildDirectory);
        Utils::sort(issues);

        if (!issues.isEmpty()) {
            QString text = QLatin1String("<nobr>");
            foreach (const ProjectExplorer::Task &task, issues) {
                QString type;
                switch (task.type) {
                case ProjectExplorer::Task::Error:
                    type = tr("Error:");
                    type += QLatin1Char(' ');
                    break;
                case ProjectExplorer::Task::Warning:
                    type = tr("Warning:");
                    type += QLatin1Char(' ');
                    break;
                case ProjectExplorer::Task::Unknown:
                default:
                    break;
                }
                if (!text.endsWith(QLatin1String("br>")))
                    text.append(QLatin1String("<br>"));
                text.append(type + task.description);
            }
            setProblemLabel(text);
            return;
        }
    } else if (targetMismatch) {
        setProblemLabel(tr("A build for a different project exists in %1, which will be overwritten.",
                           "%1 build directory")
                        .arg(m_buildConfiguration->buildDirectory().toUserOutput()));
        return;
    } else if (incompatibleBuild) {
        setProblemLabel(tr("%1 The build in %2 will be overwritten.",
                           "%1 error message, %2 build directory")
                        .arg(errorString)
                        .arg(m_buildConfiguration->buildDirectory().toUserOutput()));
        return;
    } else if (unalignedBuildDir) {
        setProblemLabel(m_buildConfiguration->unalignedBuildDirWarning());
        return;
    }

    setProblemLabel(QString());
}
