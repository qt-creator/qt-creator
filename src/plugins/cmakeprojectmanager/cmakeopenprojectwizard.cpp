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

#include "cmakeopenprojectwizard.h"
#include "cmakeprojectmanager.h"

#include <utils/pathchooser.h>
#include <projectexplorer/environment.h>

#include <QtGui/QVBoxLayout>
#include <QtGui/QFormLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QPlainTextEdit>
#include <QtCore/QDateTime>
#include <QtCore/QStringList>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

///////
//  Page Flow:
//   Start (No .user file)
//    |
//    |---> In Source Build --> Page: Tell the user about that
//                               |--> Already existing cbp file (and new enough) --> Page: Ready to load the project
//                               |--> Page: Ask for cmd options, run generator
//    |---> No in source Build --> Page: Ask the user for the build directory
//                                   |--> Already existing cbp file (and new enough) --> Page: Ready to load the project
//                                   |--> Page: Ask for cmd options, run generator

CMakeOpenProjectWizard::CMakeOpenProjectWizard(CMakeManager *cmakeManager, const QString &sourceDirectory)
    : m_cmakeManager(cmakeManager),
      m_sourceDirectory(sourceDirectory),
      m_creatingCbpFiles(false)
{
    int startid;
    if (hasInSourceBuild()) {
        startid = InSourcePageId;
        m_buildDirectory = m_sourceDirectory;
    } else {
        startid = ShadowBuildPageId;
        m_buildDirectory = m_sourceDirectory + "/qtcreator-build";
    }

    setPage(InSourcePageId, new InSourceBuildPage(this));
    setPage(ShadowBuildPageId, new ShadowBuildPage(this));
    setPage(XmlFileUpToDatePageId, new XmlFileUpToDatePage(this));
    setPage(CMakeRunPageId, new CMakeRunPage(this));

    setStartId(startid);
}

CMakeOpenProjectWizard::CMakeOpenProjectWizard(CMakeManager *cmakeManager, const QString &sourceDirectory,
                                               const QStringList &needToCreate, const QStringList &needToUpdate)
    : m_cmakeManager(cmakeManager),
      m_sourceDirectory(sourceDirectory),
      m_creatingCbpFiles(true)
{
    foreach(const QString &buildDirectory, needToCreate)
        addPage(new CMakeRunPage(this, buildDirectory, false));
    foreach(const QString &buildDirectory, needToUpdate)
        addPage(new CMakeRunPage(this, buildDirectory, true));
}

CMakeManager *CMakeOpenProjectWizard::cmakeManager() const
{
    return m_cmakeManager;
}

int CMakeOpenProjectWizard::nextId() const
{
    if (m_creatingCbpFiles)
        return QWizard::nextId();
    int cid = currentId();
    if (cid == InSourcePageId) {
        if (existsUpToDateXmlFile())
            return XmlFileUpToDatePageId;
        else
            return CMakeRunPageId;
    } else if (cid == ShadowBuildPageId) {
        if (existsUpToDateXmlFile())
            return XmlFileUpToDatePageId;
        else
            return CMakeRunPageId;
    }
    return -1;
}


bool CMakeOpenProjectWizard::hasInSourceBuild() const
{
    QFileInfo fi(m_sourceDirectory + "/CMakeCache.txt");
    if (fi.exists())
        return true;
    return false;
}

bool CMakeOpenProjectWizard::existsUpToDateXmlFile() const
{
    QString cbpFile = CMakeManager::findCbpFile(QDir(buildDirectory()));
    if (!cbpFile.isEmpty()) {
        // We already have a cbp file
        QFileInfo cbpFileInfo(cbpFile);
        QFileInfo cmakeListsFileInfo(sourceDirectory() + "/CMakeLists.txt");

        if (cbpFileInfo.lastModified() > cmakeListsFileInfo.lastModified())
            return true;
    }
    return false;
}

QString CMakeOpenProjectWizard::buildDirectory() const
{
    return m_buildDirectory;
}

QString CMakeOpenProjectWizard::sourceDirectory() const
{
    return m_sourceDirectory;
}

void CMakeOpenProjectWizard::setBuildDirectory(const QString &directory)
{
    m_buildDirectory = directory;
}

QStringList CMakeOpenProjectWizard::arguments() const
{
    return m_arguments;
}

void CMakeOpenProjectWizard::setArguments(const QStringList &args)
{
    m_arguments = args;
}


InSourceBuildPage::InSourceBuildPage(CMakeOpenProjectWizard *cmakeWizard)
    : QWizardPage(cmakeWizard), m_cmakeWizard(cmakeWizard)
{
    setLayout(new QVBoxLayout);
    QLabel *label = new QLabel(this);
    label->setWordWrap(true);
    label->setText(tr("Qt Creator has detected an in source build. "
                   "This prevents out of souce builds, Qt Creator won't allow you to change the build directory. "
                   "If you want a out of source build, clean your source directory and open the project again"));
    layout()->addWidget(label);
}


XmlFileUpToDatePage::XmlFileUpToDatePage(CMakeOpenProjectWizard *cmakeWizard)
    : QWizardPage(cmakeWizard), m_cmakeWizard(cmakeWizard)
{
    setLayout(new QVBoxLayout);
    QLabel *label = new QLabel(this);
    label->setWordWrap(true);
    label->setText(tr("Qt Creator has found a recent cbp file, which Qt Creator parses to gather information about the project. "
                   "You can change the command line arguments used to create this file in the project mode. "
                   "Click finish to load the project"));
    layout()->addWidget(label);
}

ShadowBuildPage::ShadowBuildPage(CMakeOpenProjectWizard *cmakeWizard)
    : QWizardPage(cmakeWizard), m_cmakeWizard(cmakeWizard)
{
    QFormLayout *fl = new QFormLayout;
    this->setLayout(fl);

    QLabel *label = new QLabel(this);
    label->setWordWrap(true);
    label->setText(tr("Please enter the directory in which you want to build your project. "
                   "Qt Creator recommends to not use the source directory for building. "
                   "This ensures that the source directory remains clean and enables multiple builds "
                   "with different settings."));
    fl->addWidget(label);
    m_pc = new Core::Utils::PathChooser(this);
    m_pc->setPath(m_cmakeWizard->buildDirectory());
    connect(m_pc, SIGNAL(changed()), this, SLOT(buildDirectoryChanged()));
    fl->addRow("Build directory:", m_pc);
}

void ShadowBuildPage::buildDirectoryChanged()
{
    m_cmakeWizard->setBuildDirectory(m_pc->path());
}

CMakeRunPage::CMakeRunPage(CMakeOpenProjectWizard *cmakeWizard)
    : QWizardPage(cmakeWizard),
      m_cmakeWizard(cmakeWizard),
      m_complete(false)
{
    initWidgets();
}

CMakeRunPage::CMakeRunPage(CMakeOpenProjectWizard *cmakeWizard, const QString &buildDirectory, bool update)
    : QWizardPage(cmakeWizard),
      m_cmakeWizard(cmakeWizard),
      m_complete(false),
      m_update(update),
      m_presetBuildDirectory(buildDirectory)
{
    initWidgets();
}

void CMakeRunPage::initWidgets()
{
    QFormLayout *fl = new QFormLayout;
    setLayout(fl);
    m_descriptionLabel = new QLabel(this);
    m_descriptionLabel->setWordWrap(true);

    fl->addRow(m_descriptionLabel);

    m_argumentsLineEdit = new QLineEdit(this);
    //fl->addRow(tr("Arguments:"), m_argumentsLineEdit);

    m_runCMake = new QPushButton(this);
    m_runCMake->setText(tr("Run CMake"));
    connect(m_runCMake, SIGNAL(clicked()), this, SLOT(runCMake()));
    //fl->addWidget(m_runCMake);

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget(m_argumentsLineEdit);
    hbox->addWidget(m_runCMake);

    fl->addRow(tr("Arguments"), hbox);


    m_output = new QPlainTextEdit(this);
    m_output->setReadOnly(true);
    fl->addRow(m_output);
}

void CMakeRunPage::initializePage()
{
    if (m_presetBuildDirectory.isEmpty()) {
        m_buildDirectory = m_cmakeWizard->buildDirectory();
        m_descriptionLabel->setText(
                tr("The directory %1 does not contain a cbp file. Qt Creator needs to create this file, by running cmake. "
                   "Some projects require command line arguments to the initial cmake call.").arg(m_buildDirectory));
    } else {
        m_buildDirectory = m_presetBuildDirectory;
        // TODO tell the user more?
        if (m_update)
            m_descriptionLabel->setText(tr("The directory %1 contains an outdated .cbp file. Qt "
                                           "Creator needs to update this file by running cmake. "
                                           "If you want to add additional command line arguments, "
                                           "add them in the below.").arg(m_buildDirectory));
        else
            m_descriptionLabel->setText(tr("The directory %1, specified in a buildconfiguration, "
                                           "does not contain a cbp file. Qt Creator needs to "
                                           "recreate this file, by running cmake. "
                                           "Some projects require command line arguments to "
                                           "the initial cmake call.").arg(m_buildDirectory));
    }
}

void CMakeRunPage::runCMake()
{
    m_runCMake->setEnabled(false);
    m_argumentsLineEdit->setEnabled(false);
    QStringList arguments = ProjectExplorer::Environment::parseCombinedArgString(m_argumentsLineEdit->text());
    CMakeManager *cmakeManager = m_cmakeWizard->cmakeManager();
    m_cmakeProcess = cmakeManager->createXmlFile(arguments, m_cmakeWizard->sourceDirectory(), m_buildDirectory);
    connect(m_cmakeProcess, SIGNAL(readyRead()), this, SLOT(cmakeReadyRead()));
    connect(m_cmakeProcess, SIGNAL(finished(int)), this, SLOT(cmakeFinished()));
}

void CMakeRunPage::cmakeReadyRead()
{
    m_output->appendPlainText(m_cmakeProcess->readAll());
}

void CMakeRunPage::cmakeFinished()
{
    m_runCMake->setEnabled(true);
    m_argumentsLineEdit->setEnabled(true);
    m_cmakeProcess->deleteLater();
    m_cmakeProcess = 0;
    m_cmakeWizard->setArguments(ProjectExplorer::Environment::parseCombinedArgString(m_argumentsLineEdit->text()));
    //TODO Actually test that running cmake was finished, for setting this bool
    m_complete = true;
    emit completeChanged();
}

void CMakeRunPage::cleanupPage()
{
    m_output->clear();
    m_complete = false;
    emit completeChanged();
}

bool CMakeRunPage::isComplete() const
{
    return m_complete;
}

