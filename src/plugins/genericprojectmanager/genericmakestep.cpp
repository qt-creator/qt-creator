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

#include "genericmakestep.h"
#include "genericprojectconstants.h"
#include "genericproject.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/toolchain.h>
#include <utils/qtcassert.h>

#include <QtGui/QFormLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>

namespace {
bool debug = false;
}

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;

GenericMakeStep::GenericMakeStep(GenericProject *pro)
    : AbstractProcessStep(pro), m_pro(pro), m_buildParser(0)
{
}

GenericMakeStep::~GenericMakeStep()
{
    delete m_buildParser;
    m_buildParser = 0;
}

bool GenericMakeStep::init(const QString &buildConfiguration)
{
    // TODO figure out the correct build parser
    delete m_buildParser;
    m_buildParser = 0;

    const QString buildParser = m_pro->buildParser(buildConfiguration);
    qDebug() << "*** build parser:" << buildParser;

    QList<ProjectExplorer::IBuildParserFactory *> buildParserFactories =
            ExtensionSystem::PluginManager::instance()->getObjects<ProjectExplorer::IBuildParserFactory>();

    foreach (ProjectExplorer::IBuildParserFactory *factory, buildParserFactories) {
        if (factory->canCreate(buildParser)) {
            m_buildParser = factory->create(buildParser);
            break;
        }
    }

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

    m_openDirectories.clear();
    addDirectory(m_pro->buildDirectory(buildConfiguration));

    setEnabled(buildConfiguration, true);
    setWorkingDirectory(buildConfiguration, m_pro->buildDirectory(buildConfiguration));
    if (ProjectExplorer::ToolChain *toolChain = m_pro->toolChain())
        setCommand(buildConfiguration, toolChain->makeCommand());
    else
        setCommand(buildConfiguration, "make");
    setArguments(buildConfiguration, value(buildConfiguration, "buildTargets").toStringList()); // TODO
    setEnvironment(buildConfiguration, m_pro->environment(buildConfiguration));
    return AbstractProcessStep::init(buildConfiguration);
}

void GenericMakeStep::run(QFutureInterface<bool> &fi)
{
    AbstractProcessStep::run(fi);
}

QString GenericMakeStep::name()
{
    return Constants::MAKESTEP;
}

QString GenericMakeStep::displayName()
{
    return "Make";
}

ProjectExplorer::BuildStepConfigWidget *GenericMakeStep::createConfigWidget()
{
    return new GenericMakeStepConfigWidget(this);
}

bool GenericMakeStep::immutable() const
{
    return true;
}

void GenericMakeStep::stdOut(const QString &line)
{
    if (m_buildParser)
        m_buildParser->stdOutput(line);
    AbstractProcessStep::stdOut(line);
}

void GenericMakeStep::stdError(const QString &line)
{
    if (m_buildParser)
        m_buildParser->stdError(line);
    AbstractProcessStep::stdError(line);
}

void GenericMakeStep::slotAddToTaskWindow(const QString & fn, int type, int linenumber, const QString & description)
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
            foreach (const QString &file, project()->files(ProjectExplorer::Project::AllFiles)) {
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

void GenericMakeStep::addDirectory(const QString &dir)
{
    if (!m_openDirectories.contains(dir))
        m_openDirectories.insert(dir);
}

void GenericMakeStep::removeDirectory(const QString &dir)
{
    if (m_openDirectories.contains(dir))
        m_openDirectories.remove(dir);
}


GenericProject *GenericMakeStep::project() const
{
    return m_pro;
}

bool GenericMakeStep::buildsTarget(const QString &buildConfiguration, const QString &target) const
{
    return value(buildConfiguration, "buildTargets").toStringList().contains(target);
}

void GenericMakeStep::setBuildTarget(const QString &buildConfiguration, const QString &target, bool on)
{
    QStringList old = value(buildConfiguration, "buildTargets").toStringList();
    if (on && !old.contains(target))
        setValue(buildConfiguration, "buildTargets", old << target);
    else if(!on && old.contains(target))
        setValue(buildConfiguration, "buildTargets", old.removeOne(target));
}

//
// GenericMakeStepConfigWidget
//

GenericMakeStepConfigWidget::GenericMakeStepConfigWidget(GenericMakeStep *makeStep)
    : m_makeStep(makeStep)
{
    QFormLayout *fl = new QFormLayout(this);
    setLayout(fl);

    m_targetsList = new QListWidget;
    fl->addRow("Targets:", m_targetsList);

    // TODO update this list also on rescans of the GenericLists.txt
    GenericProject *pro = m_makeStep->project();
    foreach(const QString& target, pro->targets()) {
        QListWidgetItem *item = new QListWidgetItem(target, m_targetsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
    connect(m_targetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
}

void GenericMakeStepConfigWidget::itemChanged(QListWidgetItem *item)
{
    m_makeStep->setBuildTarget(m_buildConfiguration, item->text(), item->checkState() & Qt::Checked);
}

QString GenericMakeStepConfigWidget::displayName() const
{
    return "Make";
}

void GenericMakeStepConfigWidget::init(const QString &buildConfiguration)
{
    // TODO

    // disconnect to make the changes to the items
    disconnect(m_targetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
    m_buildConfiguration = buildConfiguration;
    int count = m_targetsList->count();
    for(int i = 0; i < count; ++i) {
        QListWidgetItem *item = m_targetsList->item(i);
        item->setCheckState(m_makeStep->buildsTarget(buildConfiguration, item->text()) ? Qt::Checked : Qt::Unchecked);
    }
    // and connect again
    connect(m_targetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
}

//
// GenericMakeStepFactory
//

bool GenericMakeStepFactory::canCreate(const QString &name) const
{
    return (Constants::MAKESTEP == name);
}

ProjectExplorer::BuildStep *GenericMakeStepFactory::create(ProjectExplorer::Project *project, const QString &name) const
{
    Q_ASSERT(name == Constants::MAKESTEP);
    GenericProject *pro = qobject_cast<GenericProject *>(project);
    Q_ASSERT(pro);
    return new GenericMakeStep(pro);
}

QStringList GenericMakeStepFactory::canCreateForProject(ProjectExplorer::Project * /* pro */) const
{
    return QStringList();
}

QString GenericMakeStepFactory::displayNameForName(const QString & /* name */) const
{
    return "Make";
}
