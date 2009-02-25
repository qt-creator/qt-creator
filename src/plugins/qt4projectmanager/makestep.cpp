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

#include "makestep.h"

#include "qt4project.h"
#include "qt4projectmanagerconstants.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

using ProjectExplorer::IBuildParserFactory;
using ProjectExplorer::BuildParserInterface;
using ProjectExplorer::Environment;
using ExtensionSystem::PluginManager;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

namespace {
bool debug = false;
}

MakeStep::MakeStep(Qt4Project * project)
    : AbstractProcessStep(project),
      m_project(project),
      m_buildParser(0)
{
}

MakeStep::~MakeStep()
{
    delete m_buildParser;
    m_buildParser = 0;
}

ProjectExplorer::BuildParserInterface *MakeStep::buildParser(const QtVersion * const version)
{
    QString buildParser;
    QtVersion::ToolchainType type = version->toolchainType();
    if ( type == QtVersion::MSVC || type == QtVersion::WINCE)
        buildParser = Constants::BUILD_PARSER_MSVC;
    else
        buildParser = Constants::BUILD_PARSER_GCC;

    QList<IBuildParserFactory *> buildParserFactories =
            ExtensionSystem::PluginManager::instance()->getObjects<ProjectExplorer::IBuildParserFactory>();

    foreach (IBuildParserFactory * factory, buildParserFactories)
        if (factory->canCreate(buildParser))
            return factory->create(buildParser);
    return 0;
}

bool MakeStep::init(const QString &name)
{
    m_buildConfiguration = name;
    Environment environment = project()->environment(name);
    setEnvironment(name, environment);

    QString workingDirectory;
    if (project()->value(name, "useShadowBuild").toBool())
        workingDirectory = project()->value(name, "buildDirectory").toString();
    if (workingDirectory.isEmpty())
        workingDirectory = QFileInfo(project()->file()->fileName()).absolutePath();
    setWorkingDirectory(name, workingDirectory);

    //NBS only dependency on Qt4Project, we probably simply need a MakeProject from which Qt4Project derives
    QString makeCmd = qobject_cast<Qt4Project *>(project())->qtVersion(name)->makeCommand();
    if (!value(name, "makeCmd").toString().isEmpty())
        makeCmd = value(name, "makeCmd").toString();
    if (!QFileInfo(makeCmd).isAbsolute()) {
        // Try to detect command in environment
        QString tmp = environment.searchInPath(makeCmd);
        if (tmp == QString::null) {
            emit addToOutputWindow(tr("<font color=\"#ff0000\">Could not find make command: %1 "\
                                      "in the build environment</font>").arg(makeCmd));
            return false;
        }
        makeCmd = tmp;
    }
    setCommand(name, makeCmd);

    bool skipMakeClean = false;
    QStringList args;
    if (value("clean").isValid() && value("clean").toBool())  {
        args = QStringList() << "clean";
        if (!QDir(workingDirectory).exists(QLatin1String("Makefile"))) {
            skipMakeClean = true;
        }
    } else {
        args = value(name, "makeargs").toStringList();
    }

    // -w option enables "Enter"/"Leaving directory" messages, which we need for detecting the
    // absolute file path
    // FIXME doing this without the user having a way to override this is rather bad
    // so we only do it for unix and if the user didn't override the make command
    // but for now this is the least invasive change
    QtVersion::ToolchainType t =  qobject_cast<Qt4Project *>(project())->qtVersion(name)->toolchainType();
    if (t != QtVersion::MSVC && t != QtVersion::WINCE) {
        if (value(name, "makeCmd").toString().isEmpty())
            args << "-w";
    }

    setEnabled(name, !skipMakeClean);
    setArguments(name, args);

    m_openDirectories.clear();
    addDirectory(workingDirectory);

    delete m_buildParser;
    m_buildParser = 0;

    m_buildParser = buildParser(qobject_cast<Qt4Project *>(project())->qtVersion(name));
    if (m_buildParser) {
        connect(m_buildParser, SIGNAL(addToOutputWindow(const QString &)),
                this, SIGNAL(addToOutputWindow(const QString &)),
                Qt::DirectConnection);
        connect(m_buildParser, SIGNAL(addToTaskWindow(const QString &, int, int, const QString &)),
                this, SLOT(slotAddToTaskWindow(const QString &, int, int, const QString &)),
                Qt::DirectConnection);
        connect(m_buildParser, SIGNAL(enterDirectory(const QString &)),
                this, SLOT(addDirectory(const QString &)),
                Qt::DirectConnection);
        connect(m_buildParser, SIGNAL(leaveDirectory(const QString &)),
                this, SLOT(removeDirectory(const QString &)),
                Qt::DirectConnection);
    }

    return AbstractProcessStep::init(name);
}

void MakeStep::slotAddToTaskWindow(const QString & fn, int type, int linenumber, const QString & description)
{
    QString filePath = fn;
    if (!filePath.isEmpty() && !QDir::isAbsolutePath(filePath)) {
        // We have no save way to decide which file in which subfolder
        // is meant. Therefore we apply following heuristics:
        // 1. Search for unique file in directories currently indicated as open by GNU make
        //    (Enter directory xxx, Leave directory xxx...) + current directory
        // 3. Check if file is unique in whole project
        // 4. Otherwise give up

        filePath = filePath.trimmed();

        QList<QFileInfo> possibleFiles;
        foreach (const QString &dir, m_openDirectories) {
            QFileInfo candidate(dir + QLatin1Char('/') + filePath);
            if (debug)
                qDebug() << "Checking path " << candidate.filePath();
            if (candidate.exists()
                    && !possibleFiles.contains(candidate)) {
                if (debug)
                    qDebug() << candidate.filePath() << "exists!";
                possibleFiles << candidate;
            }
        }
        if (possibleFiles.count() == 0) {
            if (debug)
                qDebug() << "No success. Trying all files in project ...";
            QString fileName = QFileInfo(filePath).fileName();
            foreach (const QString &file, m_project->files(ProjectExplorer::Project::AllFiles)) {
                QFileInfo candidate(file);
                if (candidate.fileName() == fileName) {
                    if (debug)
                        qDebug() << "Found " << file;
                    possibleFiles << candidate;
                }
            }
        }
        if (possibleFiles.count() == 1)
            filePath = possibleFiles.first().filePath();
        else
            qWarning() << "Could not find absolute location of file " << filePath;
    }
    emit addToTaskWindow(filePath, type, linenumber, description);
}

void MakeStep::addDirectory(const QString &dir)
{
    if (!m_openDirectories.contains(dir))
        m_openDirectories.insert(dir);
}

void MakeStep::removeDirectory(const QString &dir)
{
    if (m_openDirectories.contains(dir))
        m_openDirectories.remove(dir);
}

void MakeStep::run(QFutureInterface<bool> & fi)
{
    if (qobject_cast<Qt4Project *>(project())->rootProjectNode()->projectType() == ScriptTemplate) {
        fi.reportResult(true);
        return;
    }

    if (!enabled(m_buildConfiguration)) {
        emit addToOutputWindow(tr("<font color=\"#0000ff\"><b>No Makefile found, assuming project is clean.</b></font>"));
        fi.reportResult(true);
        return;
    }

    AbstractProcessStep::run(fi);
}

void MakeStep::stdOut(const QString &line)
{
    if (m_buildParser)
        m_buildParser->stdOutput(line);
    AbstractProcessStep::stdOut(line);
}

void MakeStep::stdError(const QString &line)
{
    if (m_buildParser)
        m_buildParser->stdError(line);
    AbstractProcessStep::stdError(line);
}

QString MakeStep::name()
{
    return Constants::MAKESTEP;
}

QString MakeStep::displayName()
{
    return "Make";
}

bool MakeStep::immutable() const
{
    return true;
}

ProjectExplorer::BuildStepConfigWidget *MakeStep::createConfigWidget()
{
    return new MakeStepConfigWidget(this);
}

MakeStepConfigWidget::MakeStepConfigWidget(MakeStep *makeStep)
    : BuildStepConfigWidget(), m_makeStep(makeStep)
{
    m_ui.setupUi(this);
    connect(m_ui.makeLineEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(makeLineEditTextEdited()));
    connect(m_ui.makeArgumentsLineEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(makeArgumentsLineEditTextEdited()));
}

QString MakeStepConfigWidget::displayName() const
{
    return m_makeStep->displayName();
}

void MakeStepConfigWidget::init(const QString &buildConfiguration)
{
    m_buildConfiguration = buildConfiguration;
    bool showPage0 = buildConfiguration.isNull();
    m_ui.stackedWidget->setCurrentIndex(showPage0 ? 0 : 1);

    if (!showPage0) {
        Qt4Project *pro = qobject_cast<Qt4Project *>(m_makeStep->project());
        Q_ASSERT(pro);
        m_ui.makeLabel->setText(tr("Override %1:").arg(pro->qtVersion(buildConfiguration)->makeCommand()));

        const QString &makeCmd = m_makeStep->value(buildConfiguration, "makeCmd").toString();
        m_ui.makeLineEdit->setText(makeCmd);

        const QStringList &makeArguments =
            m_makeStep->value(buildConfiguration, "makeargs").toStringList();
        m_ui.makeArgumentsLineEdit->setText(ProjectExplorer::Environment::joinArgumentList(makeArguments));
    }

}

void MakeStepConfigWidget::makeLineEditTextEdited()
{
    Q_ASSERT(!m_buildConfiguration.isNull());
    m_makeStep->setValue(m_buildConfiguration, "makeCmd", m_ui.makeLineEdit->text());
}

void MakeStepConfigWidget::makeArgumentsLineEditTextEdited()
{
    Q_ASSERT(!m_buildConfiguration.isNull());
    m_makeStep->setValue(m_buildConfiguration, "makeargs", ProjectExplorer::Environment::parseCombinedArgString(m_ui.makeArgumentsLineEdit->text()));
}
