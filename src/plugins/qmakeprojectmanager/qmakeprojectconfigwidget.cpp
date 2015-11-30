/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmakeprojectconfigwidget.h"

#include "qmakeproject.h"
#include "qmakebuildconfiguration.h"
#include "qmakenodes.h"
#include "ui_qmakeprojectconfigwidget.h"

#include <coreplugin/coreicons.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>

using namespace QmakeProjectManager;
using namespace QmakeProjectManager::Internal;
using namespace ProjectExplorer;

QmakeProjectConfigWidget::QmakeProjectConfigWidget(QmakeBuildConfiguration *bc)
    : NamedWidget(),
      m_buildConfiguration(bc)
{
    m_defaultShadowBuildDir
            = QmakeBuildConfiguration::shadowBuildDirectory(bc->target()->project()->projectFilePath().toString(),
                                                            bc->target()->kit(),
                                                            Utils::FileUtils::qmakeFriendlyName(bc->displayName()),
                                                            bc->buildType());

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    m_detailsContainer = new Utils::DetailsWidget(this);
    m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);
    vbox->addWidget(m_detailsContainer);
    QWidget *details = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(details);
    m_ui = new Ui::QmakeProjectConfigWidget();
    m_ui->setupUi(details);

    m_browseButton = m_ui->shadowBuildDirEdit->buttonAtIndex(0);

    m_ui->warningLabel->setPixmap(Core::Icons::WARNING.pixmap());
    m_ui->shadowBuildDirEdit->setPromptDialogTitle(tr("Shadow Build Directory"));
    m_ui->shadowBuildDirEdit->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_ui->shadowBuildDirEdit->setHistoryCompleter(QLatin1String("Qmake.BuildDir.History"));
    m_ui->shadowBuildDirEdit->setEnvironment(bc->environment());
    m_ui->shadowBuildDirEdit->setBaseFileName(bc->target()->project()->projectDirectory());
    bool isShadowBuild = bc->isShadowBuild();
    if (isShadowBuild) {
        m_ui->shadowBuildDirEdit->setPath(bc->rawBuildDirectory().toString());
        m_ui->inSourceBuildDirEdit->setVisible(false);
    } else {
        m_ui->shadowBuildDirEdit->setPath(m_defaultShadowBuildDir);
        m_ui->shadowBuildDirEdit->setVisible(false);
    }
    m_ui->inSourceBuildDirEdit->setFileName(bc->target()->project()->projectDirectory());
    m_ui->inSourceBuildDirEdit->setReadOnly(true);
    m_ui->inSourceBuildDirEdit->setEnabled(false);

    m_ui->shadowBuildCheckBox->setChecked(isShadowBuild);

    connect(m_ui->shadowBuildCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(shadowBuildClicked(bool)));

    connect(m_ui->shadowBuildDirEdit, SIGNAL(beforeBrowsing()),
            this, SLOT(onBeforeBeforeShadowBuildDirBrowsed()));

    connect(m_ui->shadowBuildDirEdit, SIGNAL(rawPathChanged(QString)),
            this, SLOT(shadowBuildEdited()));

    QmakeProject *project = static_cast<QmakeProject *>(bc->target()->project());
    connect(project, SIGNAL(environmentChanged()), this, SLOT(environmentChanged()));
    connect(project, SIGNAL(buildDirectoryInitialized()), this, SLOT(updateProblemLabel()));
    connect(project, SIGNAL(proFilesEvaluated()), this, SLOT(updateProblemLabel()));

    connect(bc->target(), SIGNAL(kitChanged()), this, SLOT(updateProblemLabel()));

    connect(m_buildConfiguration, SIGNAL(buildDirectoryChanged()),
            this, SLOT(buildDirectoryChanged()));
    connect(m_buildConfiguration, SIGNAL(qmakeBuildConfigurationChanged()),
            this, SLOT(updateProblemLabel()));

    setDisplayName(tr("General"));

    updateDetails();
    updateProblemLabel();
}

QmakeProjectConfigWidget::~QmakeProjectConfigWidget()
{
    delete m_ui;
}

void QmakeProjectConfigWidget::updateDetails()
{
    m_detailsContainer->setSummaryText(
                tr("building in <b>%1</b>")
                .arg(m_buildConfiguration->buildDirectory().toUserOutput()));
}

void QmakeProjectConfigWidget::setProblemLabel(const QString &text)
{
    m_ui->warningLabel->setVisible(!text.isEmpty());
    m_ui->problemLabel->setVisible(!text.isEmpty());
    m_ui->problemLabel->setText(text);
}

void QmakeProjectConfigWidget::environmentChanged()
{
    m_ui->shadowBuildDirEdit->setEnvironment(m_buildConfiguration->environment());
}

void QmakeProjectConfigWidget::buildDirectoryChanged()
{
    if (m_ignoreChange)
        return;

    bool shadowBuild = m_ui->shadowBuildCheckBox->isChecked();
    m_ui->inSourceBuildDirEdit->setVisible(!shadowBuild);

    m_ui->shadowBuildDirEdit->setVisible(shadowBuild);
    m_ui->shadowBuildDirEdit->setEnabled(shadowBuild);
    m_browseButton->setEnabled(shadowBuild);

    m_ui->shadowBuildDirEdit->setPath(m_buildConfiguration->rawBuildDirectory().toString());

    updateDetails();
    updateProblemLabel();
}

void QmakeProjectConfigWidget::onBeforeBeforeShadowBuildDirBrowsed()
{
    Utils::FileName initialDirectory = m_buildConfiguration->target()->project()->projectDirectory();
    if (!initialDirectory.isEmpty())
        m_ui->shadowBuildDirEdit->setInitialBrowsePathBackup(initialDirectory.toString());
}

void QmakeProjectConfigWidget::shadowBuildClicked(bool checked)
{
    m_ui->shadowBuildDirEdit->setEnabled(checked);
    m_browseButton->setEnabled(checked);

    m_ui->shadowBuildDirEdit->setVisible(checked);
    m_ui->inSourceBuildDirEdit->setVisible(!checked);

    m_ignoreChange = true;
    if (checked)
        m_buildConfiguration->setBuildDirectory(Utils::FileName::fromString(m_ui->shadowBuildDirEdit->rawPath()));
    else
        m_buildConfiguration->setBuildDirectory(Utils::FileName::fromString(m_ui->inSourceBuildDirEdit->rawPath()));
    m_ignoreChange = false;

    updateDetails();
    updateProblemLabel();
}

void QmakeProjectConfigWidget::shadowBuildEdited()
{
    if (m_buildConfiguration->rawBuildDirectory().toString() == m_ui->shadowBuildDirEdit->rawPath())
        return;

    m_ignoreChange = true;
    m_buildConfiguration->setBuildDirectory(Utils::FileName::fromString(m_ui->shadowBuildDirEdit->rawPath()));
    m_ignoreChange = false;
}

void QmakeProjectConfigWidget::updateProblemLabel()
{
    m_ui->shadowBuildDirEdit->triggerChanged();
    ProjectExplorer::Kit *k = m_buildConfiguration->target()->kit();
    const QString proFileName = m_buildConfiguration->target()->project()->projectFilePath().toString();

    // Check for Qt version:
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
    if (!version) {
        setProblemLabel(tr("This kit cannot build this project since it does not define a Qt version."));
        return;
    }

    QmakeProject *p = static_cast<QmakeProject *>(m_buildConfiguration->target()->project());
    if (p->rootQmakeProjectNode()->parseInProgress() || !p->rootQmakeProjectNode()->validParse()) {
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

    if (allGood) {
        QString buildDirectory = m_buildConfiguration->target()->project()->projectDirectory().toString();
        if (m_buildConfiguration->isShadowBuild())
            buildDirectory = m_buildConfiguration->buildDirectory().toString();
        QList<ProjectExplorer::Task> issues;
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
    }

    setProblemLabel(QString());
}
