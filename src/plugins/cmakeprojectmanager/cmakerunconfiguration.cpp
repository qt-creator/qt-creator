/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cmakerunconfiguration.h"

#include "cmakeproject.h"
#include "cmakebuildconfiguration.h"
#include "cmakeprojectconstants.h"

#include <projectexplorer/environment.h>
#include <projectexplorer/debugginghelper.h>
#include <utils/qtcassert.h>
#include <QtGui/QFormLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QToolButton>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

CMakeRunConfiguration::CMakeRunConfiguration(CMakeProject *pro, const QString &target, const QString &workingDirectory, const QString &title)
    : ProjectExplorer::LocalApplicationRunConfiguration(pro)
    , m_runMode(Gui)
    , m_target(target)
    , m_workingDirectory(workingDirectory)
    , m_title(title)
    , m_baseEnvironmentBase(CMakeRunConfiguration::BuildEnvironmentBase)
{
    setName(title);

    connect(pro, SIGNAL(activeBuildConfigurationChanged()),
            this, SIGNAL(baseEnvironmentChanged()));

    // TODO
//    connect(pro, SIGNAL(environmentChanged(ProjectExplorer::BuildConfiguration *)),
//            this, SIGNAL(baseEnvironmentChanged()));
}

CMakeRunConfiguration::~CMakeRunConfiguration()
{

}

CMakeProject *CMakeRunConfiguration::cmakeProject() const
{
    return static_cast<CMakeProject *>(project());
}

QString CMakeRunConfiguration::type() const
{
    return Constants::CMAKERUNCONFIGURATION;
}

QString CMakeRunConfiguration::executable() const
{
    return m_target;
}

ProjectExplorer::LocalApplicationRunConfiguration::RunMode CMakeRunConfiguration::runMode() const
{
    return m_runMode;
}

QString CMakeRunConfiguration::workingDirectory() const
{
    if (!m_userWorkingDirectory.isEmpty())
        return m_userWorkingDirectory;
    return m_workingDirectory;
}

QStringList CMakeRunConfiguration::commandLineArguments() const
{
    return ProjectExplorer::Environment::parseCombinedArgString(m_arguments);
}

QString CMakeRunConfiguration::title() const
{
    return m_title;
}

void CMakeRunConfiguration::setExecutable(const QString &executable)
{
    m_target = executable;
}

void CMakeRunConfiguration::setWorkingDirectory(const QString &wd)
{
    const QString & oldWorkingDirectory = workingDirectory();

    m_workingDirectory = wd;

    const QString &newWorkingDirectory = workingDirectory();
    if (oldWorkingDirectory != newWorkingDirectory)
        emit workingDirectoryChanged(newWorkingDirectory);
}

void CMakeRunConfiguration::setUserWorkingDirectory(const QString &wd)
{
    const QString & oldWorkingDirectory = workingDirectory();

    m_userWorkingDirectory = wd;

    const QString &newWorkingDirectory = workingDirectory();
    if (oldWorkingDirectory != newWorkingDirectory)
        emit workingDirectoryChanged(newWorkingDirectory);
}

void CMakeRunConfiguration::save(ProjectExplorer::PersistentSettingsWriter &writer) const
{
    ProjectExplorer::LocalApplicationRunConfiguration::save(writer);
    writer.saveValue("CMakeRunConfiguration.Target", m_target);
    writer.saveValue("CMakeRunConfiguration.WorkingDirectory", m_workingDirectory);
    writer.saveValue("CMakeRunConfiguration.UserWorkingDirectory", m_userWorkingDirectory);
    writer.saveValue("CMakeRunConfiguration.UseTerminal", m_runMode == Console);
    writer.saveValue("CMakeRunConfiguation.Title", m_title);
    writer.saveValue("CMakeRunConfiguration.Arguments", m_arguments);
    writer.saveValue("CMakeRunConfiguration.UserEnvironmentChanges", ProjectExplorer::EnvironmentItem::toStringList(m_userEnvironmentChanges));
    writer.saveValue("BaseEnvironmentBase", m_baseEnvironmentBase);

}

void CMakeRunConfiguration::restore(const ProjectExplorer::PersistentSettingsReader &reader)
{
    ProjectExplorer::LocalApplicationRunConfiguration::restore(reader);
    m_target = reader.restoreValue("CMakeRunConfiguration.Target").toString();
    m_workingDirectory = reader.restoreValue("CMakeRunConfiguration.WorkingDirectory").toString();
    m_userWorkingDirectory = reader.restoreValue("CMakeRunConfiguration.UserWorkingDirectory").toString();
    m_runMode = reader.restoreValue("CMakeRunConfiguration.UseTerminal").toBool() ? Console : Gui;
    m_title = reader.restoreValue("CMakeRunConfiguation.Title").toString();
    m_arguments = reader.restoreValue("CMakeRunConfiguration.Arguments").toString();
    m_userEnvironmentChanges = ProjectExplorer::EnvironmentItem::fromStringList(reader.restoreValue("CMakeRunConfiguration.UserEnvironmentChanges").toStringList());
    QVariant tmp = reader.restoreValue("BaseEnvironmentBase");
    m_baseEnvironmentBase = tmp.isValid() ? BaseEnvironmentBase(tmp.toInt()) : CMakeRunConfiguration::BuildEnvironmentBase;
}

QWidget *CMakeRunConfiguration::configurationWidget()
{
    return new CMakeRunConfigurationWidget(this);
}

void CMakeRunConfiguration::setArguments(const QString &newText)
{
    m_arguments = newText;
}

QString CMakeRunConfiguration::dumperLibrary() const
{
    QString qmakePath = ProjectExplorer::DebuggingHelperLibrary::findSystemQt(environment());
    QString qtInstallData = ProjectExplorer::DebuggingHelperLibrary::qtInstallDataDir(qmakePath);
    QString dhl = ProjectExplorer::DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(qtInstallData);
    return dhl;
}

QStringList CMakeRunConfiguration::dumperLibraryLocations() const
{
    QString qmakePath = ProjectExplorer::DebuggingHelperLibrary::findSystemQt(environment());
    QString qtInstallData = ProjectExplorer::DebuggingHelperLibrary::qtInstallDataDir(qmakePath);
    return ProjectExplorer::DebuggingHelperLibrary::debuggingHelperLibraryLocationsByInstallData(qtInstallData);
}

ProjectExplorer::Environment CMakeRunConfiguration::baseEnvironment() const
{
    ProjectExplorer::Environment env;
    if (m_baseEnvironmentBase == CMakeRunConfiguration::CleanEnvironmentBase) {
        // Nothing
    } else  if (m_baseEnvironmentBase == CMakeRunConfiguration::SystemEnvironmentBase) {
        env = ProjectExplorer::Environment::systemEnvironment();
    } else  if (m_baseEnvironmentBase == CMakeRunConfiguration::BuildEnvironmentBase) {
        env = environment();
    }
    return env;
}

void CMakeRunConfiguration::setBaseEnvironmentBase(BaseEnvironmentBase env)
{
    if (m_baseEnvironmentBase == env)
        return;
    m_baseEnvironmentBase = env;
    emit baseEnvironmentChanged();
}

CMakeRunConfiguration::BaseEnvironmentBase CMakeRunConfiguration::baseEnvironmentBase() const
{
    return m_baseEnvironmentBase;
}

ProjectExplorer::Environment CMakeRunConfiguration::environment() const
{
    ProjectExplorer::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

QList<ProjectExplorer::EnvironmentItem> CMakeRunConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

void CMakeRunConfiguration::setUserEnvironmentChanges(const QList<ProjectExplorer::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges != diff) {
        m_userEnvironmentChanges = diff;
        emit userEnvironmentChangesChanged(diff);
    }
}

ProjectExplorer::ToolChain::ToolChainType CMakeRunConfiguration::toolChainType() const
{
    CMakeBuildConfiguration *bc = cmakeProject()->activeCMakeBuildConfiguration();
    return bc->toolChainType();
}

// Configuration widget


CMakeRunConfigurationWidget::CMakeRunConfigurationWidget(CMakeRunConfiguration *cmakeRunConfiguration, QWidget *parent)
    : QWidget(parent), m_ignoreChange(false), m_cmakeRunConfiguration(cmakeRunConfiguration)
{

    QFormLayout *fl = new QFormLayout();
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    QLineEdit *argumentsLineEdit = new QLineEdit();
    argumentsLineEdit->setText(ProjectExplorer::Environment::joinArgumentList(cmakeRunConfiguration->commandLineArguments()));
    connect(argumentsLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(setArguments(QString)));
    fl->addRow(tr("Arguments:"), argumentsLineEdit);

    m_workingDirectoryEdit = new Utils::PathChooser();
    m_workingDirectoryEdit->setPath(m_cmakeRunConfiguration->workingDirectory());
    m_workingDirectoryEdit->setExpectedKind(Utils::PathChooser::Directory);
    m_workingDirectoryEdit->setPromptDialogTitle(tr("Select the working directory"));

    QToolButton *resetButton = new QToolButton();
    resetButton->setToolTip(tr("Reset to default"));
    resetButton->setIcon(QIcon(":/core/images/reset.png"));

    QHBoxLayout *boxlayout = new QHBoxLayout();
    boxlayout->addWidget(m_workingDirectoryEdit);
    boxlayout->addWidget(resetButton);

    fl->addRow(tr("Working Directory:"), boxlayout);

    m_detailsContainer = new Utils::DetailsWidget(this);

    QWidget *m_details = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(m_details);
    m_details->setLayout(fl);

    QVBoxLayout *vbx = new QVBoxLayout(this);
    vbx->setMargin(0);;
    vbx->addWidget(m_detailsContainer);

    QLabel *environmentLabel = new QLabel(this);
    environmentLabel->setText(tr("Run Environment"));
    QFont f = environmentLabel->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() *1.2);
    environmentLabel->setFont(f);
    vbx->addWidget(environmentLabel);

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
    m_baseEnvironmentComboBox->setCurrentIndex(m_cmakeRunConfiguration->baseEnvironmentBase());
    connect(m_baseEnvironmentComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(baseEnvironmentComboBoxChanged(int)));
    baseEnvironmentLayout->addWidget(m_baseEnvironmentComboBox);
    baseEnvironmentLayout->addStretch(10);

    m_environmentWidget = new ProjectExplorer::EnvironmentWidget(this, baseEnvironmentWidget);
    m_environmentWidget->setBaseEnvironment(m_cmakeRunConfiguration->baseEnvironment());
    m_environmentWidget->setUserChanges(m_cmakeRunConfiguration->userEnvironmentChanges());

    vbx->addWidget(m_environmentWidget);

    updateSummary();

    connect(m_workingDirectoryEdit, SIGNAL(changed(QString)),
            this, SLOT(setWorkingDirectory()));

    connect(resetButton, SIGNAL(clicked()),
            this, SLOT(resetWorkingDirectory()));

    connect(m_environmentWidget, SIGNAL(userChangesUpdated()),
            this, SLOT(userChangesUpdated()));

    connect(m_cmakeRunConfiguration, SIGNAL(workingDirectoryChanged(QString)),
            this, SLOT(workingDirectoryChanged(QString)));
    connect(m_cmakeRunConfiguration, SIGNAL(baseEnvironmentChanged()),
            this, SLOT(baseEnvironmentChanged()));
    connect(m_cmakeRunConfiguration, SIGNAL(userEnvironmentChangesChanged(QList<ProjectExplorer::EnvironmentItem>)),
            this, SLOT(userEnvironmentChangesChanged()));

}

void CMakeRunConfigurationWidget::setWorkingDirectory()
{
    if (m_ignoreChange)
        return;
    m_ignoreChange = true;
    m_cmakeRunConfiguration->setUserWorkingDirectory(m_workingDirectoryEdit->path());
    m_ignoreChange = false;
}

void CMakeRunConfigurationWidget::workingDirectoryChanged(const QString &workingDirectory)
{
    if (!m_ignoreChange)
        m_workingDirectoryEdit->setPath(workingDirectory);
}

void CMakeRunConfigurationWidget::resetWorkingDirectory()
{
    // This emits a signal connected to workingDirectoryChanged()
    // that sets the m_workingDirectoryEdit
    m_cmakeRunConfiguration->setUserWorkingDirectory("");
}

void CMakeRunConfigurationWidget::userChangesUpdated()
{
    m_cmakeRunConfiguration->setUserEnvironmentChanges(m_environmentWidget->userChanges());
}

void CMakeRunConfigurationWidget::baseEnvironmentComboBoxChanged(int index)
{
    m_ignoreChange = true;
    m_cmakeRunConfiguration->setBaseEnvironmentBase(CMakeRunConfiguration::BaseEnvironmentBase(index));

    m_environmentWidget->setBaseEnvironment(m_cmakeRunConfiguration->baseEnvironment());
    m_ignoreChange = false;
}

void CMakeRunConfigurationWidget::baseEnvironmentChanged()
{
    if (m_ignoreChange)
        return;

    m_baseEnvironmentComboBox->setCurrentIndex(m_cmakeRunConfiguration->baseEnvironmentBase());
    m_environmentWidget->setBaseEnvironment(m_cmakeRunConfiguration->baseEnvironment());
}

void CMakeRunConfigurationWidget::userEnvironmentChangesChanged()
{
    m_environmentWidget->setUserChanges(m_cmakeRunConfiguration->userEnvironmentChanges());
}

void CMakeRunConfigurationWidget::setArguments(const QString &args)
{
    m_cmakeRunConfiguration->setArguments(args);
    updateSummary();
}

void CMakeRunConfigurationWidget::updateSummary()
{
    QString text = tr("Running executable: <b>%1</b> %2")
                   .arg(QFileInfo(m_cmakeRunConfiguration->executable()).fileName(),
                        ProjectExplorer::Environment::joinArgumentList(m_cmakeRunConfiguration->commandLineArguments()));
    m_detailsContainer->setSummaryText(text);
}


// Factory
CMakeRunConfigurationFactory::CMakeRunConfigurationFactory()
{

}

CMakeRunConfigurationFactory::~CMakeRunConfigurationFactory()
{

}

// used to recreate the runConfigurations when restoring settings
bool CMakeRunConfigurationFactory::canRestore(const QString &type) const
{
    if (type.startsWith(Constants::CMAKERUNCONFIGURATION))
        return true;
    return false;
}

// used to show the list of possible additons to a project, returns a list of types
QStringList CMakeRunConfigurationFactory::availableCreationTypes(ProjectExplorer::Project *project) const
{
    CMakeProject *pro = qobject_cast<CMakeProject *>(project);
    if (!pro)
        return QStringList();
    QStringList allTargets = pro->targets();
    for (int i=0; i<allTargets.size(); ++i) {
        allTargets[i] = Constants::CMAKERUNCONFIGURATION + allTargets[i];
    }
    return allTargets;
}

// used to translate the types to names to display to the user
QString CMakeRunConfigurationFactory::displayNameForType(const QString &type) const
{
    Q_ASSERT(type.startsWith(Constants::CMAKERUNCONFIGURATION));

    if (type == Constants::CMAKERUNCONFIGURATION)
        return "CMake"; // Doesn't happen
    else
        return type.mid(QString(Constants::CMAKERUNCONFIGURATION).length());
}

ProjectExplorer::RunConfiguration* CMakeRunConfigurationFactory::create(ProjectExplorer::Project *project, const QString &type)
{
    CMakeProject *pro = qobject_cast<CMakeProject *>(project);
    Q_ASSERT(pro);
    if (type == Constants::CMAKERUNCONFIGURATION) {
        // Restoring, filename will be added by restoreSettings
        ProjectExplorer::RunConfiguration* rc = new CMakeRunConfiguration(pro, QString::null, QString::null, QString::null);
        return rc;
    } else {
        // Adding new
        const QString title = type.mid(QString(Constants::CMAKERUNCONFIGURATION).length());
        const CMakeTarget &ct = pro->targetForTitle(title);
        ProjectExplorer::RunConfiguration * rc = new CMakeRunConfiguration(pro, ct.executable, ct.workingDirectory, ct.title);
        return rc;
    }
}
