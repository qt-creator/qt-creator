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

#include "projectloadwizard.h"

#include "qt4project.h"
#include "qt4projectmanager.h"
#include "qmakestep.h"
#include "qt4target.h"
#include "makestep.h"
#include "qt4buildconfiguration.h"

#include <extensionsystem/pluginmanager.h>

#include <QtGui/QCheckBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWizardPage>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

ProjectLoadWizard::ProjectLoadWizard(Qt4Project *project, QWidget *parent, Qt::WindowFlags flags)
    : QWizard(parent, flags), m_project(project), m_importVersion(0), m_temporaryVersion(false)
{
    setWindowTitle(tr("Import existing build settings"));
    QtVersionManager * vm = QtVersionManager::instance();
    QString directory = QFileInfo(project->file()->fileName()).absolutePath();
    QString importVersion =  QtVersionManager::findQMakeBinaryFromMakefile(directory);

    if (!importVersion.isNull()) {
        // This also means we have a build in there
        // First get the qt version
        m_importVersion = vm->qtVersionForQMakeBinary(importVersion);
        // Okay does not yet exist, create
        if (!m_importVersion) {
            m_importVersion = new QtVersion(importVersion);
            m_temporaryVersion = true;
        }

        QPair<QtVersion::QmakeBuildConfigs, QStringList> result =
                QtVersionManager::scanMakeFile(directory, m_importVersion->defaultBuildConfig());
        m_importBuildConfig = result.first;
        m_additionalArguments = Qt4BuildConfiguration::removeSpecFromArgumentList(result.second);

        QString parsedSpec = Qt4BuildConfiguration::extractSpecFromArgumentList(result.second, directory, m_importVersion);
        QString versionSpec = m_importVersion->mkspec();

        // Compare mkspecs and add to additional arguments
        if (parsedSpec.isEmpty() || parsedSpec == versionSpec || parsedSpec == "default") {
            // using the default spec, don't modify additional arguments
        } else {
            m_additionalArguments.prepend(parsedSpec);
            m_additionalArguments.prepend("-spec");
        }
    }

    // So now we have the version and the configuration for that version
    // If buildAll we create debug and release configurations,
    // if not then just either debug or release
    // The default buildConfiguration depends on QmakeBuildConfig::DebugBuild
    // Also if the qt version is not yet in the Tools Options dialog we offer to add it there

    if (m_importVersion)
        setupImportPage(m_importVersion, m_importBuildConfig, m_additionalArguments);

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

void ProjectLoadWizard::done(int result)
{
    QtVersionManager *vm = QtVersionManager::instance();
    QWizard::done(result);
    // This normally happens on showing the final page, but since we
    // don't show it anymore, do it here

    if (m_importVersion && importCheckbox->isChecked()) {
        // Importing
        if (m_temporaryVersion)
            vm->addVersion(m_importVersion);

        // Import the existing stuff
        // qDebug()<<"Creating m_buildconfiguration entry from imported stuff";
        // qDebug()<<((m_importBuildConfig& QtVersion::BuildAll)? "debug_and_release" : "")<<((m_importBuildConfig & QtVersion::DebugBuild)? "debug" : "release");
        foreach (const QString &id, m_importVersion->supportedTargetIds()) {
            Qt4Target *t(m_project->targetFactory()->create(m_project, id, QList<QtVersion*>() << m_importVersion));
            if (!t)
                continue;
            m_project->addTarget(t);
        }
        if (m_project->targets().isEmpty())
            qWarning() << "Failed to populate project with default targets for imported Qt" << m_importVersion->displayName();
    } else {
        // Not importing
        if (m_temporaryVersion)
            delete m_importVersion;

        // Find a Qt version:
        QList<QtVersion *> candidates = vm->versions();
        QtVersion *defaultVersion = candidates.at(0); // always there and always valid!
        // Check for the first valid desktop-Qt, fall back to any valid Qt if no desktop
        // flavour is available.
        foreach (QtVersion *v, candidates) {
            if (v->isValid())
                defaultVersion = v;
            if (v->supportsTargetId(DESKTOP_TARGET_ID) && v->isValid())
                break;
        }

        foreach (const QString &id, defaultVersion->supportedTargetIds()) {
            Qt4Target *t(m_project->targetFactory()->create(m_project, id, QList<QtVersion *>() << defaultVersion));
            if (!t)
                continue;
            m_project->addTarget(t);
        }
        if (m_project->targets().isEmpty())
            qWarning() << "Failed to populate project with default targets for default Qt" << defaultVersion->displayName();
    }
}

// This function used to do the commented stuff instead of having only one page
int ProjectLoadWizard::nextId() const
{
    return -1;
}

void ProjectLoadWizard::setupImportPage(QtVersion *version, QtVersion::QmakeBuildConfigs buildConfig, QStringList addtionalArguments)
{
    resize(605, 490);
    // Import Page
    importPage = new QWizardPage(this);
    importPage->setTitle(tr("Import existing build settings"));
    QVBoxLayout *importLayout = new QVBoxLayout(importPage);
    importLabel = new QLabel(importPage);

    QString versionString = version->displayName() + QLatin1String(" (") +
                            QDir::toNativeSeparators(version->qmakeCommand()) +
                            QLatin1Char(')');
    QString buildConfigString = (buildConfig & QtVersion::BuildAll) ? QLatin1String("debug_and_release ") : QString();
    buildConfigString.append((buildConfig & QtVersion::DebugBuild) ? QLatin1String("debug") : QLatin1String("release"));
    importLabel->setTextFormat(Qt::RichText);
    importLabel->setText(tr("Qt Creator has found an already existing build in the source directory.<br><br>"
                         "<b>Qt Version:</b> %1<br>"
                         "<b>Build configuration:</b> %2<br>"
                         "<b>Additional qmake Arguments:</b>%3")
                         .arg(versionString)
                         .arg(buildConfigString)
                         .arg(ProjectExplorer::Environment::joinArgumentList(addtionalArguments)));

    importLayout->addWidget(importLabel);


    importCheckbox = new QCheckBox(importPage);
    importCheckbox->setText(tr("Import existing build settings."));
    importCheckbox->setChecked(true);
    importLayout->addWidget(importCheckbox);
    import2Label = new QLabel(importPage);
    import2Label->setTextFormat(Qt::RichText);
    if (m_temporaryVersion)
        import2Label->setText(tr("<b>Note:</b> Importing the settings will automatically add the Qt Version identified by <br><b>%1</b> to the list of Qt versions.")
                              .arg(QDir::toNativeSeparators(m_importVersion->qmakeCommand())));
    importLayout->addWidget(import2Label);
    addPage(importPage);
}

