/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "qt4projectconfigwidget.h"

#include "makestep.h"
#include "qmakestep.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include "qt4projectmanager.h"
#include "qt4buildconfiguration.h"
#include "ui_qt4projectconfigwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/task.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildconfiguration.h>
#include <qtsupport/qtversionfactory.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtprofileinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <extensionsystem/pluginmanager.h>

#include <QFileDialog>
#include <QPushButton>
#include <utils/detailswidget.h>

namespace {
bool debug = false;
}

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using namespace ProjectExplorer;

Qt4ProjectConfigWidget::Qt4ProjectConfigWidget(ProjectExplorer::Target *target)
    : BuildConfigWidget(),
      m_buildConfiguration(0),
      m_ignoreChange(false)
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    m_detailsContainer = new Utils::DetailsWidget(this);
    m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);
    vbox->addWidget(m_detailsContainer);
    QWidget *details = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(details);
    m_ui = new Ui::Qt4ProjectConfigWidget();
    m_ui->setupUi(details);

    m_browseButton = m_ui->shadowBuildDirEdit->buttonAtIndex(0);

    m_ui->shadowBuildDirEdit->setPromptDialogTitle(tr("Shadow Build Directory"));
    m_ui->shadowBuildDirEdit->setExpectedKind(Utils::PathChooser::ExistingDirectory);

    connect(m_ui->shadowBuildCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(shadowBuildClicked(bool)));

    connect(m_ui->shadowBuildDirEdit, SIGNAL(beforeBrowsing()),
            this, SLOT(onBeforeBeforeShadowBuildDirBrowsed()));

    connect(m_ui->shadowBuildDirEdit, SIGNAL(changed(QString)),
            this, SLOT(shadowBuildEdited()));

    Qt4Project *project = qobject_cast<Qt4Project *>(target->project());
    if (project) {
        connect(project, SIGNAL(environmentChanged()), this, SLOT(environmentChanged()));
        connect(project, SIGNAL(buildDirectoryInitialized()), this, SLOT(updateProblemLabel()));
    }
}

Qt4ProjectConfigWidget::~Qt4ProjectConfigWidget()
{
    delete m_ui;
}

void Qt4ProjectConfigWidget::updateDetails()
{
    m_detailsContainer->setSummaryText(
                tr("building in <b>%3</b>")
                .arg(QDir::toNativeSeparators(m_buildConfiguration->buildDirectory())));
}

void Qt4ProjectConfigWidget::environmentChanged()
{
    m_ui->shadowBuildDirEdit->setEnvironment(m_buildConfiguration->environment());
}

QString Qt4ProjectConfigWidget::displayName() const
{
    return tr("General");
}

void Qt4ProjectConfigWidget::init(ProjectExplorer::BuildConfiguration *bc)
{
    QTC_ASSERT(bc, return);

    if (debug)
        qDebug() << "Qt4ProjectConfigWidget::init() for" << bc->displayName();

    if (m_buildConfiguration) {
        disconnect(m_buildConfiguration, SIGNAL(buildDirectoryChanged()),
                this, SLOT(buildDirectoryChanged()));
        disconnect(m_buildConfiguration, SIGNAL(qmakeBuildConfigurationChanged()),
                   this, SLOT(updateProblemLabel()));
    }
    m_buildConfiguration = static_cast<Qt4BuildConfiguration *>(bc);
    m_ui->shadowBuildDirEdit->setEnvironment(m_buildConfiguration->environment());

    connect(m_buildConfiguration, SIGNAL(buildDirectoryChanged()),
            this, SLOT(buildDirectoryChanged()));
    connect(m_buildConfiguration, SIGNAL(qmakeBuildConfigurationChanged()),
            this, SLOT(updateProblemLabel()));

    m_ui->shadowBuildDirEdit->setBaseDirectory(m_buildConfiguration->target()->project()->projectDirectory());

    buildDirectoryChanged();
}

void Qt4ProjectConfigWidget::buildDirectoryChanged()
{
    if (m_ignoreChange)
        return;
    m_ui->shadowBuildDirEdit->setPath(m_buildConfiguration->shadowBuildDirectory());
    bool shadowBuild = m_buildConfiguration->shadowBuild();
    m_ui->shadowBuildCheckBox->setChecked(shadowBuild);
    m_ui->shadowBuildDirEdit->setEnabled(shadowBuild);
    m_browseButton->setEnabled(shadowBuild);

    updateDetails();
    updateProblemLabel();
}

void Qt4ProjectConfigWidget::onBeforeBeforeShadowBuildDirBrowsed()
{
    QString initialDirectory = m_buildConfiguration->target()->project()->projectDirectory();
    if (!initialDirectory.isEmpty())
        m_ui->shadowBuildDirEdit->setInitialBrowsePathBackup(initialDirectory);
}

void Qt4ProjectConfigWidget::shadowBuildClicked(bool checked)
{
    m_ui->shadowBuildDirEdit->setEnabled(checked);
    m_browseButton->setEnabled(checked);
    bool b = m_ui->shadowBuildCheckBox->isChecked();

    m_ignoreChange = true;
    m_buildConfiguration->setShadowBuildAndDirectory(b, m_ui->shadowBuildDirEdit->rawPath());
    m_ignoreChange = false;

    updateDetails();
    updateProblemLabel();
}

void Qt4ProjectConfigWidget::shadowBuildEdited()
{
    if (m_buildConfiguration->shadowBuildDirectory() == m_ui->shadowBuildDirEdit->rawPath())
        return;
    m_ignoreChange = true;
    m_buildConfiguration->setShadowBuildAndDirectory(m_buildConfiguration->shadowBuild(), m_ui->shadowBuildDirEdit->rawPath());
    m_ignoreChange = false;

    // if the directory already exists
    // check if we have a build in there and
    // offer to import it
    updateProblemLabel();
    updateDetails();
}

void Qt4ProjectConfigWidget::updateProblemLabel()
{
    bool targetMismatch = false;
    bool incompatibleBuild = false;
    bool allGood = false;

    ProjectExplorer::Profile *p = m_buildConfiguration->target()->profile();
    const QString proFileName = m_buildConfiguration->target()->project()->document()->fileName();

    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(p);
    if (!version) {
        m_ui->problemLabel->setVisible(true);
        m_ui->warningLabel->setVisible(true);
        m_ui->problemLabel->setText(tr("This target cannot build this project since it does not define a "
                                       "Qt version."));
        return;
    }

    // we only show if we actually have a qmake and makestep
    if (m_buildConfiguration->qmakeStep() && m_buildConfiguration->makeStep()) {
        QString makefile = m_buildConfiguration->buildDirectory() + QLatin1Char('/');
        if (m_buildConfiguration->makefile().isEmpty())
            makefile.append(QLatin1String("Makefile"));
        else
            makefile.append(m_buildConfiguration->makefile());

        switch (m_buildConfiguration->compareToImportFrom(makefile)) {
        case Qt4BuildConfiguration::MakefileMatches:
            allGood = true;
            break;
        case Qt4BuildConfiguration::MakefileMissing:
            allGood = true;
            break;
        case Qt4BuildConfiguration::MakefileIncompatible:
            incompatibleBuild = true;
            break;
        case Qt4BuildConfiguration::MakefileForWrongProject:
            targetMismatch = true;
            break;
        }
    }

    QString shadowBuildWarning;
    if (!version->supportsShadowBuilds() && m_buildConfiguration->shadowBuild()) {
        shadowBuildWarning =tr("The qt version %1 does not support shadow builds, building might fail.")
                .arg(version->displayName())
                + QLatin1String("<br>");
    }

    if (allGood) {
        QString buildDirectory = m_buildConfiguration->target()->project()->projectDirectory();;
        if (m_buildConfiguration->shadowBuild())
            buildDirectory = m_buildConfiguration->buildDirectory();
        QList<ProjectExplorer::Task> issues;
        issues = version->reportIssues(proFileName, buildDirectory);
        qSort(issues);

        if (issues.isEmpty() && shadowBuildWarning.isEmpty()) {
            m_ui->problemLabel->setVisible(false);
            m_ui->warningLabel->setVisible(false);
        } else {
            m_ui->problemLabel->setVisible(true);
            m_ui->warningLabel->setVisible(true);
            QString text = QLatin1String("<nobr>") + shadowBuildWarning;
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
            m_ui->problemLabel->setText(text);
        }
    } else if (targetMismatch) {
        m_ui->problemLabel->setVisible(true);
        m_ui->warningLabel->setVisible(true);
        m_ui->problemLabel->setText(shadowBuildWarning + tr("A build for a different project exists in %1, which will be overwritten.",
                                                            "%1 build directory")
                                    .arg(m_ui->shadowBuildDirEdit->path()));
    } else if (incompatibleBuild) {
        m_ui->warningLabel->setVisible(true);
        m_ui->problemLabel->setVisible(true);
        m_ui->problemLabel->setText(shadowBuildWarning +tr("An incompatible build exists in %1, which will be overwritten.",
                                                           "%1 build directory")
                                    .arg(m_ui->shadowBuildDirEdit->path()));
    } else if (shadowBuildWarning.isEmpty()) {
        m_ui->warningLabel->setVisible(true);
        m_ui->problemLabel->setVisible(true);
        m_ui->problemLabel->setText(shadowBuildWarning);
    }
}
