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

#include "qt4projectconfigwidget.h"

#include "makestep.h"
#include "qmakestep.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include "qt4projectmanager.h"
#include "qt4buildconfiguration.h"
#include "ui_qt4projectconfigwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/mainwindow.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildconfiguration.h>
#include <extensionsystem/pluginmanager.h>

#include <QtGui/QFileDialog>

namespace {
bool debug = false;
}

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using ProjectExplorer::ToolChain;

Qt4ProjectConfigWidget::Qt4ProjectConfigWidget(Qt4Project *project)
    : BuildConfigWidget(),
      m_buildConfiguration(0),
      m_ignoreChange(false)
{
    Q_UNUSED(project);
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    m_detailsContainer = new Utils::DetailsWidget(this);
    vbox->addWidget(m_detailsContainer);
    QWidget *details = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(details);
    m_ui = new Ui::Qt4ProjectConfigWidget();
    m_ui->setupUi(details);


    m_browseButton = m_ui->shadowBuildDirEdit->buttonAtIndex(0);
    // TODO refix the layout

    m_ui->shadowBuildDirEdit->setPromptDialogTitle(tr("Shadow Build Directory"));
    m_ui->shadowBuildDirEdit->setExpectedKind(Utils::PathChooser::Directory);
    m_ui->invalidQtWarningLabel->setVisible(false);

    connect(m_ui->nameLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(configNameEdited(QString)));

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


    QtVersionManager *vm = QtVersionManager::instance();
    connect(vm, SIGNAL(qtVersionsChanged(QList<int>)),
            this, SLOT(qtVersionsChanged()));
}

Qt4ProjectConfigWidget::~Qt4ProjectConfigWidget()
{
    delete m_ui;
}

void Qt4ProjectConfigWidget::updateDetails()
{
    QtVersion *version = m_buildConfiguration->qtVersion();


    QString versionString;
    if (m_buildConfiguration->qtVersionId() == 0) {
        versionString = tr("Default Qt Version (%1)").arg(version->displayName());
    } else if(version){
        versionString = version->displayName();
    } else {
        versionString = tr("No Qt Version set");
    }

    if (!version->isValid()) {
        // Not a valid qt version
        m_detailsContainer->setSummaryText(
                tr("using <font color=\"#ff0000\">invalid</font> Qt Version: <b>%1</b><br>"
                   "%2")
                .arg(versionString,
                     version->invalidReason()));
    } else {
        // Qt Version, Build Directory and Toolchain
        m_detailsContainer->setSummaryText(
                tr("using Qt version: <b>%1</b><br>"
                   "with tool chain <b>%2</b><br>"
                   "building in <b>%3</b>")
                .arg(versionString,
                     ProjectExplorer::ToolChain::toolChainName(m_buildConfiguration->toolChainType()),
                     QDir::toNativeSeparators(m_buildConfiguration->buildDirectory())));
    }
}

void Qt4ProjectConfigWidget::manageQtVersions()
{
    Core::ICore *core = Core::ICore::instance();
    core->showOptionsDialog(Constants::QT_SETTINGS_CATEGORY, Constants::QTVERSION_SETTINGS_PAGE_ID);
}


QString Qt4ProjectConfigWidget::displayName() const
{
    return tr("General");
}

void Qt4ProjectConfigWidget::init(ProjectExplorer::BuildConfiguration *bc)
{
    if (debug)
        qDebug() << "Qt4ProjectConfigWidget::init() for"<<bc->displayName();

    if (m_buildConfiguration) {
        disconnect(m_buildConfiguration, SIGNAL(buildDirectoryChanged()),
                   this, SLOT(buildDirectoryChanged()));
        disconnect(m_buildConfiguration, SIGNAL(qtVersionChanged()),
                   this, SLOT(qtVersionChanged()));
        disconnect(m_buildConfiguration, SIGNAL(qmakeBuildConfigurationChanged()),
                   this, SLOT(updateImportLabel()));
    }

    m_buildConfiguration = static_cast<Qt4BuildConfiguration *>(bc);

    connect(m_buildConfiguration, SIGNAL(buildDirectoryChanged()),
            this, SLOT(buildDirectoryChanged()));
    connect(m_buildConfiguration, SIGNAL(qtVersionChanged()),
            this, SLOT(qtVersionChanged()));
    connect(m_buildConfiguration, SIGNAL(qmakeBuildConfigurationChanged()),
            this, SLOT(updateImportLabel()));

    m_ui->nameLineEdit->setText(m_buildConfiguration->displayName());

    qtVersionsChanged();

    bool shadowBuild = m_buildConfiguration->shadowBuild();
    m_ui->shadowBuildCheckBox->setChecked(shadowBuild);
    m_ui->shadowBuildDirEdit->setEnabled(shadowBuild);
    m_browseButton->setEnabled(shadowBuild);
    m_ui->shadowBuildDirEdit->setPath(m_buildConfiguration->buildDirectory());
    updateImportLabel();
    updateToolChainCombo();
    updateDetails();
}

void Qt4ProjectConfigWidget::qtVersionChanged()
{
    updateImportLabel();
    updateToolChainCombo();
    updateDetails();
}

void Qt4ProjectConfigWidget::configNameEdited(const QString &newName)
{
    m_buildConfiguration->setDisplayName(newName);
}

void Qt4ProjectConfigWidget::qtVersionsChanged()
{
    if (!m_buildConfiguration) // not yet initialized
        return;

    disconnect(m_ui->qtVersionComboBox, SIGNAL(currentIndexChanged(QString)),
        this, SLOT(qtVersionSelected(QString)));

    QtVersionManager *vm = QtVersionManager::instance();

    m_ui->qtVersionComboBox->clear();
    m_ui->qtVersionComboBox->addItem(tr("Default Qt Version (%1)").arg(vm->defaultVersion()->displayName()), 0);

    int qtVersionId = m_buildConfiguration->qtVersionId();

    if (qtVersionId == 0) {
        m_ui->qtVersionComboBox->setCurrentIndex(0);
        m_ui->invalidQtWarningLabel->setVisible(false);
    }
    // Add Qt Versions to the combo box
    const QList<QtVersion *> &versions = vm->versions();
    for (int i = 0; i < versions.size(); ++i) {
        m_ui->qtVersionComboBox->addItem(versions.at(i)->displayName(), versions.at(i)->uniqueId());

        if (versions.at(i)->uniqueId() == qtVersionId) {
            m_ui->qtVersionComboBox->setCurrentIndex(i + 1);
            m_ui->invalidQtWarningLabel->setVisible(!versions.at(i)->isValid());
        }
    }

    // And connect again
    connect(m_ui->qtVersionComboBox, SIGNAL(currentIndexChanged(QString)),
        this, SLOT(qtVersionSelected(QString)));
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
    QString initialDirectory = QFileInfo(m_buildConfiguration->project()->file()->fileName()).absolutePath();
    if (!initialDirectory.isEmpty())
        m_ui->shadowBuildDirEdit->setInitialBrowsePathBackup(initialDirectory);
}

void Qt4ProjectConfigWidget::shadowBuildClicked(bool checked)
{
    m_ui->shadowBuildDirEdit->setEnabled(checked);
    m_browseButton->setEnabled(checked);
    bool b = m_ui->shadowBuildCheckBox->isChecked();

    m_ignoreChange = true;
    m_buildConfiguration->setShadowBuildAndDirectory(b, m_ui->shadowBuildDirEdit->path());
    m_ignoreChange = false;

    updateDetails();
    updateImportLabel();
}

void Qt4ProjectConfigWidget::shadowBuildEdited()
{
    if (m_buildConfiguration->shadowBuildDirectory() == m_ui->shadowBuildDirEdit->path())
        return;
    m_ignoreChange = true;
    m_buildConfiguration->setShadowBuildAndDirectory(true, m_ui->shadowBuildDirEdit->path());
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

    // we only show if we actually have a qmake and makestep
    if (m_buildConfiguration->qmakeStep() && m_buildConfiguration->makeStep()) {
        QString qmakePath = QtVersionManager::findQMakeBinaryFromMakefile(m_buildConfiguration->buildDirectory());
        QtVersion *version = m_buildConfiguration->qtVersion();
        // check that there's a makefile
        if (!qmakePath.isEmpty()) {
            // and that the qmake path is different from the current version
            if (qmakePath != (version ? version->qmakeCommand() : QString())) {
                // import enable
                visible = true;
            } else {
                // check that the qmake flags, arguments match
                visible = !m_buildConfiguration->compareToImportFrom(m_buildConfiguration->buildDirectory());
            }
        } else {
            visible = false;
        }
    }

    m_ui->importLabel->setVisible(visible);
}

void Qt4ProjectConfigWidget::importLabelClicked()
{
    if (!m_buildConfiguration->qmakeStep() || !m_buildConfiguration->makeStep())
        return;
    QString directory = m_buildConfiguration->buildDirectory();
    if (!directory.isEmpty()) {
        QString qmakePath = QtVersionManager::findQMakeBinaryFromMakefile(directory);
        if (!qmakePath.isEmpty()) {
            QtVersionManager *vm = QtVersionManager::instance();
            QtVersion *version = vm->qtVersionForQMakeBinary(qmakePath);
            if (!version) {
                version = new QtVersion(qmakePath);
                vm->addVersion(version);
            }

            QPair<QtVersion::QmakeBuildConfigs, QStringList> result =
                    QtVersionManager::scanMakeFile(directory, version->defaultBuildConfig());
            QtVersion::QmakeBuildConfigs qmakeBuildConfig = result.first;
            QStringList additionalArguments = Qt4BuildConfiguration::removeSpecFromArgumentList(result.second);
            QString parsedSpec = Qt4BuildConfiguration::extractSpecFromArgumentList(result.second, directory, version);
            QString versionSpec = version->mkspec();
            if (parsedSpec.isEmpty() || parsedSpec == versionSpec || parsedSpec == "default") {
                // using the default spec, don't modify additional arguments
            } else {
                additionalArguments.prepend(parsedSpec);
                additionalArguments.prepend("-spec");
            }

            // So we got all the information now apply it...
            m_buildConfiguration->setQtVersion(version->uniqueId());
            // Combo box will be updated at the end

            QMakeStep *qmakeStep = m_buildConfiguration->qmakeStep();
            qmakeStep->setUserArguments(additionalArguments);
            MakeStep *makeStep = m_buildConfiguration->makeStep();

            m_buildConfiguration->setQMakeBuildConfiguration(qmakeBuildConfig);
            // Adjust command line arguments, this is ugly as hell
            // If we are switching to BuildAll we want "release" in there and no "debug"
            // or "debug" in there and no "release"
            // If we are switching to not BuildAl we want neither "release" nor "debug" in there
            QStringList makeCmdArguments = makeStep->userArguments();
            bool debug = qmakeBuildConfig & QtVersion::DebugBuild;
            if (qmakeBuildConfig & QtVersion::BuildAll) {
                makeCmdArguments.removeAll(debug ? "release" : "debug");
                if (!makeCmdArguments.contains(debug ? "debug" : "release"))
                    makeCmdArguments.append(debug ? "debug" : "release");
            } else {
                makeCmdArguments.removeAll("debug");
                makeCmdArguments.removeAll("release");
            }
            makeStep->setUserArguments(makeCmdArguments);
        }
    }
    // All our widgets are updated by signals from the buildconfiguration
    // if not, there's either a signal missing
    // or we don't respond to it correctly
}

void Qt4ProjectConfigWidget::qtVersionSelected(const QString &)
{
    //Qt Version
    int newQtVersion;
    if (m_ui->qtVersionComboBox->currentIndex() == 0) {
        newQtVersion = 0;
    } else {
        newQtVersion = m_ui->qtVersionComboBox->itemData(m_ui->qtVersionComboBox->currentIndex()).toInt();
    }
    QtVersionManager *vm = QtVersionManager::instance();
    bool isValid = vm->version(newQtVersion)->isValid();
    m_ui->invalidQtWarningLabel->setVisible(!isValid);
    if (newQtVersion != m_buildConfiguration->qtVersionId()) {
        m_ignoreChange = true;
        m_buildConfiguration->setQtVersion(newQtVersion);
        m_ignoreChange = false;
        updateToolChainCombo();
    }
    updateDetails();
}

void Qt4ProjectConfigWidget::updateToolChainCombo()
{
    m_ui->toolChainComboBox->clear();
    QList<ProjectExplorer::ToolChain::ToolChainType> toolchains = m_buildConfiguration->qtVersion()->possibleToolChainTypes();
    foreach (ToolChain::ToolChainType toolchain, toolchains) {
        m_ui->toolChainComboBox->addItem(ToolChain::toolChainName(toolchain), qVariantFromValue(toolchain));
    }
    m_ui->toolChainComboBox->setEnabled(toolchains.size() > 1);
    m_ui->toolChainComboBox->setCurrentIndex(toolchains.indexOf(m_buildConfiguration->toolChainType()));
    updateDetails();
}

void Qt4ProjectConfigWidget::toolChainSelected(int index)
{
    ProjectExplorer::ToolChain::ToolChainType selectedToolChainType =
        m_ui->toolChainComboBox->itemData(index,
            Qt::UserRole).value<ProjectExplorer::ToolChain::ToolChainType>();
    m_ignoreChange = true;
    m_buildConfiguration->setToolChainType(selectedToolChainType);
    m_ignoreChange = false;
    updateDetails();
}
