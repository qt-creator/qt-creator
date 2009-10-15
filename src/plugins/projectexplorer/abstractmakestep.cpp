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

#include "abstractmakestep.h"

#include "projectexplorerconstants.h"
#include "project.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

using ExtensionSystem::PluginManager;

using namespace ProjectExplorer;

namespace {
bool debug = false;
}

AbstractMakeStep::AbstractMakeStep(Project *project)
    : AbstractProcessStep(project),
      m_project(project),
      m_buildParser(0)
{
}

AbstractMakeStep::~AbstractMakeStep()
{
    delete m_buildParser;
    m_buildParser = 0;
}

bool AbstractMakeStep::init(const QString &buildConfiguration)
{
    m_buildConfiguration = buildConfiguration;

    m_openDirectories.clear();
    addDirectory(workingDirectory());

    return AbstractProcessStep::init(buildConfiguration);
}

QString AbstractMakeStep::buildParser() const
{
    return m_buildParserName;
}

void AbstractMakeStep::setBuildParser(const QString &parser)
{
    // Nothing to do?
    if (m_buildParserName == parser)
        return;

    // Clean up
    delete m_buildParser;
    m_buildParser = 0;
    m_buildParserName = QString::null;

    // Now look for new parser
    QList<IBuildParserFactory *> buildParserFactories =
            ExtensionSystem::PluginManager::instance()->getObjects<ProjectExplorer::IBuildParserFactory>();

    foreach (IBuildParserFactory * factory, buildParserFactories)
        if (factory->canCreate(parser)) {
            m_buildParser = factory->create(parser);
            break;
        }

    if (m_buildParser) {
        m_buildParserName = parser;
        connect(m_buildParser, SIGNAL(addToOutputWindow(QString)),
                this, SIGNAL(addToOutputWindow(QString)),
                Qt::DirectConnection);
        connect(m_buildParser, SIGNAL(addToTaskWindow(ProjectExplorer::TaskWindow::Task)),
                this, SLOT(slotAddToTaskWindow(ProjectExplorer::TaskWindow::Task)),
                Qt::DirectConnection);
        connect(m_buildParser, SIGNAL(enterDirectory(QString)),
                this, SLOT(addDirectory(QString)),
                Qt::DirectConnection);
        connect(m_buildParser, SIGNAL(leaveDirectory(QString)),
                this, SLOT(removeDirectory(QString)),
                Qt::DirectConnection);
    }
}

void AbstractMakeStep::slotAddToTaskWindow(const TaskWindow::Task &task)
{
    TaskWindow::Task editable(task);
    QString filePath = QDir::cleanPath(task.file.trimmed());
    if (!filePath.isEmpty() && !QDir::isAbsolutePath(filePath)) {
        // We have no save way to decide which file in which subfolder
        // is meant. Therefore we apply following heuristics:
        // 1. Search for unique file in directories currently indicated as open by GNU make
        //    (Enter directory xxx, Leave directory xxx...) + current directory
        // 3. Check if file is unique in whole project
        // 4. Otherwise give up

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
        if (possibleFiles.count() == 1) {
            editable.file = possibleFiles.first().filePath();
        } else {
            // More then one filename, so do a better compare
            // Chop of any "../"
            while (filePath.startsWith("../"))
                filePath = filePath.mid(3);
            int count = 0;
            QString possibleFilePath;
            foreach(const QFileInfo & fi, possibleFiles) {
                if (fi.filePath().endsWith(filePath)) {
                    possibleFilePath = fi.filePath();
                    ++count;
                }
            }
            if (count == 1)
                editable.file = possibleFilePath;
            else
                qWarning() << "Could not find absolute location of file " << filePath;
        }
    }
    emit addToTaskWindow(editable);
}

void AbstractMakeStep::addDirectory(const QString &dir)
{
    if (!m_openDirectories.contains(dir))
        m_openDirectories.insert(dir);
}

void AbstractMakeStep::removeDirectory(const QString &dir)
{
    if (m_openDirectories.contains(dir))
        m_openDirectories.remove(dir);
}

void AbstractMakeStep::run(QFutureInterface<bool> & fi)
{
    AbstractProcessStep::run(fi);
}

void AbstractMakeStep::stdOut(const QString &line)
{
    if (m_buildParser)
        m_buildParser->stdOutput(line);
    AbstractProcessStep::stdOut(line);
}

void AbstractMakeStep::stdError(const QString &line)
{
    if (m_buildParser)
        m_buildParser->stdError(line);
    AbstractProcessStep::stdError(line);
}
