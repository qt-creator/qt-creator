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

#include "qt4projectconfigwidget.h"

#include "makestep.h"
#include "qmakestep.h"
#include "qt4project.h"
#include "qt4target.h"
#include "qt4projectmanagerconstants.h"
#include "qt4projectmanager.h"
#include "qt4buildconfiguration.h"
#include "ui_qt4projectconfigwidget.h"

#include <coreplugin/icore.h>

#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildconfiguration.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <extensionsystem/pluginmanager.h>

#include <QtGui/QFileDialog>
#include <QtGui/QPushButton>
#include <utils/detailswidget.h>

namespace {
bool debug = false;
}

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using namespace ProjectExplorer;

Qt4ProjectConfigWidget::Qt4ProjectConfigWidget(Qt4BaseTarget *target)
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
    // TODO refix the layout

    m_ui->shadowBuildDirEdit->setPromptDialogTitle(tr("Shadow Build Directory"));
    m_ui->shadowBuildDirEdit->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_ui->shadowBuildDirEdit->setBaseDirectory(target->qt4Project()->projectDirectory());

    connect(m_ui->shadowBuildCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(shadowBuildClicked(bool)));

    connect(m_ui->shadowBuildDirEdit, SIGNAL(beforeBrowsing()),
            this, SLOT(onBeforeBeforeShadowBuildDirBrowsed()));

    connect(m_ui->shadowBuildDirEdit, SIGNAL(changed(QString)),
            this, SLOT(shadowBuildEdited()));

    connect(m_ui->qtVersionComboBox, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(qtVersionSelected(QString)));

    connect(m_ui->toolChainComboBox, SIGNAL(activated(int)),
            this, SLOT(toolChainSelected(int)));

    connect(m_ui->importLabel, SIGNAL(linkActivated(QString)),
            this, SLOT(importLabelClicked()));

    connect(m_ui->manageQtVersionPushButtons, SIGNAL(clicked()),
            this, SLOT(manageQtVersions()));

    connect(m_ui->manageToolChainPushButton, SIGNAL(clicked()),
            this, SLOT(manageToolChains()));

    connect(target->qt4Project(), SIGNAL(environmentChanged()),
            this, SLOT(environmentChanged()));

    connect(target->qt4Project(), SIGNAL(buildDirectoryInitialized()),
            this, SLOT(updateImportLabel()));
}

Qt4ProjectConfigWidget::~Qt4ProjectConfigWidget()
{
    delete m_ui;
}

void Qt4ProjectConfigWidget::updateDetails()
{
    QtVersion *version = m_buildConfiguration->qtVersion();

    QString versionString;
    versionString = version->displayName();

    if (!version || !version->isValid()) {
        // Not a valid qt version
        m_detailsContainer->setSummaryText(
                tr("using <font color=\"#ff0000\">invalid</font> Qt Version: <b>%1</b><br>"
                   "%2")
                .arg(versionString,
                     version ? version->invalidReason() : tr("No Qt Version found.")));
    } else {
        // Qt Version, Build Directory and Toolchain
        m_detailsContainer->setSummaryText(
                tr("using Qt version: <b>%1</b><br>"
                   "with tool chain <b>%2</b><br>"
                   "building in <b>%3</b>")
                .arg(versionString,
                     m_buildConfiguration->toolChain() ? m_buildConfiguration->toolChain()->displayName() :
                                                         tr("<Invalid ToolChain>"),
                     QDir::toNativeSeparators(m_buildConfiguration->buildDirectory())));
    }
}

void Qt4ProjectConfigWidget::environmentChanged()
{
    m_ui->shadowBuildDirEdit->setEnvironment(m_buildConfiguration->environment());
}

void Qt4ProjectConfigWidget::updateShadowBuildUi()
{
    m_ui->shadowBuildCheckBox->setEnabled(m_buildConfiguration->qtVersion()->supportsShadowBuilds());
    bool isShadowbuilding = m_buildConfiguration->shadowBuild();
    m_ui->shadowBuildDirEdit->setEnabled(isShadowbuilding && m_buildConfiguration->qtVersion()->supportsShadowBuilds());
    m_browseButton->setEnabled(isShadowbuilding && m_buildConfiguration->qtVersion()->supportsShadowBuilds());
    m_ui->shadowBuildDirEdit->setPath(m_buildConfiguration->shadowBuildDirectory());
}

void Qt4ProjectConfigWidget::manageQtVersions()
{
    Core::ICore *core = Core::ICore::instance();
    core->showOptionsDialog(Constants::QT_SETTINGS_CATEGORY, Constants::QTVERSION_SETTINGS_PAGE_ID);
}

void Qt4ProjectConfigWidget::manageToolChains()
{
    Core::ICore *core = Core::ICore::instance();
    core->showOptionsDialog(ProjectExplorer::Constants::TOOLCHAIN_SETTINGS_CATEGORY,
                            ProjectExplorer::Constants::TOOLCHAIN_SETTINGS_PAGE_ID);
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
        disconnect(m_buildConfiguration, SIGNAL(qtVersionChanged()),
                   this, SLOT(qtVersionChanged()));
        disconnect(m_buildConfiguration, SIGNAL(qmakeBuildConfigurationChanged()),
                   this, SLOT(updateImportLabel()));
        disconnect(m_buildConfiguration, SIGNAL(toolChainChanged()),
                   this, SLOT(toolChainChanged()));
    }
    m_buildConfiguration = static_cast<Qt4BuildConfiguration *>(bc);
    m_ui->shadowBuildDirEdit->setEnvironment(m_buildConfiguration->environment());

    connect(m_buildConfiguration, SIGNAL(buildDirectoryChanged()),
            this, SLOT(buildDirectoryChanged()));
    connect(m_buildConfiguration, SIGNAL(qtVersionChanged()),
            this, SLOT(qtVersionChanged()));
    connect(m_buildConfiguration, SIGNAL(qmakeBuildConfigurationChanged()),
            this, SLOT(updateImportLabel()));
    connect(m_buildConfiguration, SIGNAL(toolChainChanged()),
            this, SLOT(toolChainChanged()));

    qtVersionsChanged();
    QtVersionManager *vm = QtVersionManager::instance();
    connect(vm, SIGNAL(qtVersionsChanged(QList<int>)),
            this, SLOT(qtVersionsChanged()));

    bool shadowBuild = m_buildConfiguration->shadowBuild();
    m_ui->shadowBuildCheckBox->setChecked(shadowBuild);
    m_ui->shadowBuildCheckBox->setEnabled(m_buildConfiguration->qtVersion()->supportsShadowBuilds());

    updateShadowBuildUi();
    updateImportLabel();
    updateToolChainCombo();
    updateDetails();

    connect(ToolChainManager::instance(), SIGNAL(toolChainAdded(ProjectExplorer::ToolChain*)),
            this, SLOT(updateToolChainCombo()));
    connect(ToolChainManager::instance(), SIGNAL(toolChainRemoved(ProjectExplorer::ToolChain*)),
            this, SLOT(updateToolChainCombo()));
}

void Qt4ProjectConfigWidget::qtVersionChanged()
{
    if (m_ignoreChange)
        return;

    int versionId = m_buildConfiguration->qtVersion()->uniqueId();
    int comboBoxIndex = m_ui->qtVersionComboBox->findData(QVariant(versionId), Qt::UserRole);
    if (comboBoxIndex > -1)
        m_ui->qtVersionComboBox->setCurrentIndex(comboBoxIndex);

    updateShadowBuildUi();
    updateImportLabel();
    updateToolChainCombo();
    updateDetails();
}

void Qt4ProjectConfigWidget::qtVersionsChanged()
{
    m_ignoreChange = true;
    QtVersionManager *vm = QtVersionManager::instance();

    m_ui->qtVersionComboBox->clear();
    QtVersion * qtVersion = m_buildConfiguration->qtVersion();

    const QList<QtVersion *> validVersions(vm->versionsForTargetId(m_buildConfiguration->target()->id()));
    if (!validVersions.isEmpty()) {
        for (int i = 0; i < validVersions.size(); ++i) {
            m_ui->qtVersionComboBox->addItem(validVersions.at(i)->displayName(),
                                             validVersions.at(i)->uniqueId());

            if (validVersions.at(i) == qtVersion)
                m_ui->qtVersionComboBox->setCurrentIndex(i);
        }
    }
    if (!qtVersion->isValid()) {
        m_ui->qtVersionComboBox->addItem(tr("Invalid Qt version"), -1);
        m_ui->qtVersionComboBox->setCurrentIndex(m_ui->qtVersionComboBox->count() - 1);
    }
    m_ui->qtVersionComboBox->setEnabled(m_ui->qtVersionComboBox->count() > 1);
    m_ignoreChange = false;

    updateToolChainCombo();
    updateShadowBuildUi();
    updateDetails();
    updateImportLabel();
}

void Qt4ProjectConfigWidget::buildDirectoryChanged()
{
    if (m_ignoreChange)
        return;
    m_ui->shadowBuildDirEdit->setPath(m_buildConfiguration->shadowBuildDirectory());
    updateDetails();
    updateImportLabel();
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
    updateImportLabel();
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
    updateImportLabel();
    updateDetails();
}

void Qt4ProjectConfigWidget::updateImportLabel()
{
    bool visible = false;
    bool targetMatches = false;
    bool incompatibleBuild = false;

    QtVersionManager *vm = QtVersionManager::instance();
    // we only show if we actually have a qmake and makestep
    if (m_buildConfiguration->qmakeStep() && m_buildConfiguration->makeStep()) {
        QString makefile = m_buildConfiguration->buildDirectory();
        if (m_buildConfiguration->makefile().isEmpty())
            makefile.append("/Makefile");
        else
            makefile.append(m_buildConfiguration->makefile());

        QString qmakePath = QtVersionManager::findQMakeBinaryFromMakefile(makefile);
        QtVersion *version = m_buildConfiguration->qtVersion();
        // check that there's a makefile
        if (!qmakePath.isEmpty()) {
            // Is it from the same build?
            if (!QtVersionManager::makefileIsFor(makefile, m_buildConfiguration->target()->project()->file()->fileName())) {
                incompatibleBuild = true;
            } else if (qmakePath != (version ? version->qmakeCommand() : QString())) {
                // and that the qmake path is different from the current version
                // import enable
                visible = true;
                QtVersion *newVersion = vm->qtVersionForQMakeBinary(qmakePath);
                bool mustDelete(false);
                if (!newVersion) {
                    newVersion = new QtVersion(qmakePath);
                    mustDelete = true;
                }
                targetMatches = newVersion->supportsTargetId(m_buildConfiguration->target()->id());
                if (mustDelete)
                    delete newVersion;
            } else {
                // check that the qmake flags, arguments match
                visible = !m_buildConfiguration->compareToImportFrom(makefile);
                targetMatches = true;
            }
        }
    }

    QString buildDirectory = m_buildConfiguration->target()->project()->projectDirectory();;
    if (m_buildConfiguration->shadowBuild())
        buildDirectory = m_buildConfiguration->buildDirectory();
    QList<ProjectExplorer::Task> issues = m_buildConfiguration->qtVersion()->reportIssues(m_buildConfiguration->target()->project()->file()->fileName(),
                                                                                          buildDirectory);

    if (incompatibleBuild) {
        m_ui->problemLabel->setVisible(true);
        m_ui->warningLabel->setVisible(true);
        m_ui->importLabel->setVisible(false);
        m_ui->problemLabel->setText(tr("An build for a different project exists in %1, which will be overwritten.",
                                       "%1 build directory").
                                    arg(m_ui->shadowBuildDirEdit->path()));
    } else if (!issues.isEmpty()) {
        m_ui->problemLabel->setVisible(true);
        m_ui->warningLabel->setVisible(true);
        m_ui->importLabel->setVisible(visible);
        QString text = "<nobr>";
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
    } else if (targetMatches) {
        m_ui->problemLabel->setVisible(false);
        m_ui->warningLabel->setVisible(false);
        m_ui->importLabel->setVisible(visible);
    } else {
        m_ui->warningLabel->setVisible(visible);
        m_ui->problemLabel->setVisible(visible);
        m_ui->problemLabel->setText(tr("An incompatible build exists in %1, which will be overwritten.",
                                       "%1 build directory").
                                    arg(m_ui->shadowBuildDirEdit->path()));
        m_ui->importLabel->setVisible(false);
    }
}

void Qt4ProjectConfigWidget::importLabelClicked()
{
    if (!m_buildConfiguration->qmakeStep() || !m_buildConfiguration->makeStep())
        return;

    // We do the importing via a single shot timer due to QTCREATORBUG-2723
    // Adding a qtversion might trigger a supportedTargetIds changed signal
    // which results in a recreation of all the widgets on the page
    // That means also "this" gets deleted
    QTimer::singleShot(0, m_buildConfiguration, SLOT(importFromBuildDirectory()));
}

void Qt4ProjectConfigWidget::qtVersionSelected(const QString &)
{
    if (m_ignoreChange)
        return;

    int newQtVersionId = m_ui->qtVersionComboBox->itemData(m_ui->qtVersionComboBox->currentIndex()).toInt();

    if (m_ui->qtVersionComboBox->itemData(m_ui->qtVersionComboBox->count() - 1).toInt() == -1)
        m_ui->qtVersionComboBox->removeItem(m_ui->qtVersionComboBox->count() - 1);

    QtVersionManager *vm = QtVersionManager::instance();
    QtVersion *newQtVersion = vm->version(newQtVersionId);

    m_ignoreChange = true;
    m_buildConfiguration->setQtVersion(newQtVersion);
    m_ignoreChange = false;

    updateShadowBuildUi();
    updateToolChainCombo();
    updateImportLabel();
    updateDetails();
}

void Qt4ProjectConfigWidget::toolChainChanged()
{
    if (m_ignoreChange)
        return;
    for (int i=0; i < m_ui->toolChainComboBox->count(); ++i) {
        ProjectExplorer::ToolChain *tc =
                static_cast<ProjectExplorer::ToolChain *>(m_ui->toolChainComboBox->itemData(i, Qt::UserRole).value<void *>());
        if (tc == m_buildConfiguration->toolChain()) {
            m_ignoreChange = true;
            m_ui->toolChainComboBox->setCurrentIndex(i);
            m_ignoreChange = false;
        }
    }
}

void Qt4ProjectConfigWidget::updateToolChainCombo()
{
    m_ui->toolChainComboBox->clear();
    QList<ProjectExplorer::ToolChain *> toolchains =
            m_buildConfiguration->qt4Target()->possibleToolChains(m_buildConfiguration);

    foreach (ProjectExplorer::ToolChain *toolchain, toolchains)
        m_ui->toolChainComboBox->addItem(toolchain->displayName(),
                                         qVariantFromValue(static_cast<void *>(toolchain)));
    m_ignoreChange = true;
    if (!m_buildConfiguration->toolChain() || toolchains.isEmpty()) {
        m_ui->toolChainComboBox->addItem(tr("<Invalid Toolchain>"), qVariantFromValue(static_cast<void *>(0)));
        m_ui->toolChainComboBox->setCurrentIndex(m_ui->toolChainComboBox->count() - 1);
    } else if (toolchains.contains(m_buildConfiguration->toolChain())) {
        m_ui->toolChainComboBox->setCurrentIndex(toolchains.indexOf(m_buildConfiguration->toolChain()));
    } else { // reset to some sensible toolchain
        ToolChain *tc = 0;
        if (!toolchains.isEmpty())
            tc = toolchains.at(0);
        m_buildConfiguration->setToolChain(tc);
    }
    m_ignoreChange = false;
    m_ui->toolChainComboBox->setEnabled(toolchains.size() > 1);
}

void Qt4ProjectConfigWidget::toolChainSelected(int index)
{
    if (m_ignoreChange)
        return;
    ProjectExplorer::ToolChain *selectedToolChain =
            static_cast<ProjectExplorer::ToolChain *>(
                m_ui->toolChainComboBox->itemData(index,
                                                  Qt::UserRole).value<void *>());
    m_ignoreChange = true;
    m_buildConfiguration->setToolChain(selectedToolChain);
    m_ignoreChange = false;
    updateDetails();
}
