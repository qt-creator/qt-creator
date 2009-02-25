/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "projectloadwizard.h"

#include "qt4project.h"
#include "qtversionmanager.h"
#include "qt4projectmanager.h"
#include "qmakestep.h"
#include "makestep.h"

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QFileDialog>
#include <QtGui/QFormLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>
#include <QtGui/QRadioButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWizard>
#include <QtGui/QWizardPage>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

ProjectLoadWizard::ProjectLoadWizard(Qt4Project *project, QWidget *parent, Qt::WindowFlags flags)
    : QWizard(parent, flags), m_project(project), m_importVersion(0), m_temporaryVersion(false)
{
    QtVersionManager * vm = project->qt4ProjectManager()->versionManager();
    QString directory = QFileInfo(project->file()->fileName()).absolutePath();
    QString importVersion =  vm->findQtVersionFromMakefile(directory);

    if (!importVersion.isNull()) {
        // This also means we have a build in there
        // First get the qt version
        m_importVersion = vm->qtVersionForDirectory(importVersion);
        // Okay does not yet exist, create
        if (!m_importVersion) {
            m_importVersion = new QtVersion(QFileInfo(importVersion).baseName(), importVersion);
            m_temporaryVersion = true;
        }

        m_importBuildConfig = m_importVersion->defaultBuildConfig();
        m_importBuildConfig= vm->scanMakefileForQmakeConfig(directory, m_importBuildConfig);
    }

    // So now we have the version and the configuration for that version
    // If buildAll we create debug and release configurations,
    // if not then just either debug or release
    // The default buildConfiguration depends on QmakeBuildConfig::DebugBuild
    // Also if the qt version is not yet in the Tools Options dialog we offer to add it there

    if (m_importVersion)
        setupImportPage(m_importVersion, m_importBuildConfig);

    setOptions(options() | QWizard::NoCancelButton | QWizard::NoBackButtonOnLastPage);
}

// We don't want to actually show the dialog if we don't show the import page
// We used to simply call ::exec() on the dialog
void ProjectLoadWizard::execDialog()
{
    if (m_importVersion)
        exec();
    else
        done(QDialog::Accepted);
}

ProjectLoadWizard::~ProjectLoadWizard()
{

}

void ProjectLoadWizard::addBuildConfiguration(QString name, QtVersion *qtversion, QtVersion::QmakeBuildConfig buildConfiguration)
{
    QMakeStep *qmakeStep = m_project->qmakeStep();
    MakeStep *makeStep = m_project->makeStep();

    bool debug = buildConfiguration & QtVersion::DebugBuild;
    // Check that bc.name is not already in use
    if (m_project->buildConfigurations().contains(name)) {
        int i =1;
        do {
            ++i;
        } while (m_project->buildConfigurations().contains(name + " " + QString::number(i)));
        name.append(" " + QString::number(i));
    }

    // Add the buildconfiguration
    m_project->addBuildConfiguration(name);
    // set some options for qmake and make
    if (buildConfiguration & QtVersion::BuildAll) // debug_and_release => explicit targets
        makeStep->setValue(name, "makeargs", QStringList() << (debug ? "debug" : "release"));

    qmakeStep->setValue(name, "buildConfiguration", int(buildConfiguration));

    // Finally set the qt version
    bool defaultQtVersion = (qtversion == 0);
    if (defaultQtVersion)
        m_project->setQtVersion(name, 0);
    else
        m_project->setQtVersion(name, qtversion->uniqueId());

}

void ProjectLoadWizard::done(int result)
{
    QWizard::done(result);
    // This normally happens on showing the final page, but since we
    // don't show it anymore, do it here

    QString directory = QFileInfo(m_project->file()->fileName()).absolutePath();
    if (m_importVersion && importCheckbox->isChecked()) {
        if (m_temporaryVersion)
            m_project->qt4ProjectManager()->versionManager()->addVersion(m_importVersion);
        // Import the existing stuff
        // qDebug()<<"Creating m_buildconfiguration entry from imported stuff";
        // qDebug()<<((m_importBuildConfig& QtVersion::BuildAll)? "debug_and_release" : "")<<((m_importBuildConfig & QtVersion::DebugBuild)? "debug" : "release");
        bool debug = m_importBuildConfig & QtVersion::DebugBuild;
        addBuildConfiguration(debug ? "Debug" : "Release", m_importVersion, m_importBuildConfig);

        if (m_importBuildConfig & QtVersion::BuildAll) {
            // Also create the other configuration
            QtVersion::QmakeBuildConfig otherBuildConfiguration = m_importBuildConfig;
            if (debug)
                otherBuildConfiguration = QtVersion::QmakeBuildConfig(otherBuildConfiguration & ~ QtVersion::DebugBuild);
            else
                otherBuildConfiguration = QtVersion::QmakeBuildConfig(otherBuildConfiguration | QtVersion::DebugBuild);

            addBuildConfiguration(debug ? "Release" : "Debug", m_importVersion, otherBuildConfiguration);
        }
    } else {
        if (m_temporaryVersion)
            delete m_importVersion;
        // Create default   
        addBuildConfiguration("Debug", 0, QtVersion::QmakeBuildConfig(QtVersion::BuildAll | QtVersion::DebugBuild));
        addBuildConfiguration("Release", 0, QtVersion::BuildAll);
    }

    if (!m_project->buildConfigurations().isEmpty())
        m_project->setActiveBuildConfiguration(m_project->buildConfigurations().at(0));
}

// This function used to do the commented stuff instead of having only one page
int ProjectLoadWizard::nextId() const
{
    return -1;
}

void ProjectLoadWizard::setupImportPage(QtVersion *version, QtVersion::QmakeBuildConfig buildConfig)
{
    resize(605, 490);
    // Import Page
    importPage = new QWizardPage(this);
    importPage->setTitle("Import existing settings");
    QVBoxLayout *importLayout = new QVBoxLayout(importPage);
    importLabel = new QLabel(importPage);

    QString versionString = version->name() + " (" + version->path() + ")";
    QString buildConfigString = (buildConfig & QtVersion::BuildAll) ? QLatin1String("debug_and_release ") : QLatin1String("");
    buildConfigString.append((buildConfig & QtVersion::DebugBuild) ? QLatin1String("debug") : QLatin1String("release"));
    importLabel->setTextFormat(Qt::RichText);
    importLabel->setText(tr("Qt Creator has found an already existing build in the source directory.<br><br>"
                         "<b>Qt Version:</b> %1<br>"
                         "<b>Build configuration:</b> %2<br>")
                         .arg(versionString)
                         .arg(buildConfigString));

    importLayout->addWidget(importLabel);


    importCheckbox = new QCheckBox(importPage);
    importCheckbox->setText("Import existing build settings.");
    importCheckbox->setChecked(true);
    importLayout->addWidget(importCheckbox);
    import2Label = new QLabel(importPage);
    import2Label->setTextFormat(Qt::RichText);
    if (m_temporaryVersion)
        import2Label->setText(QString("<b>Note:</b> Importing the settings will automatically add the Qt Version from:<br><b>%1</b> to the list of qt versions.")
                              .arg(m_importVersion->path()));
    importLayout->addWidget(import2Label);
    addPage(importPage);
}

