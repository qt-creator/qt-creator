/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlprofilerdetailsrewriter.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/runconfiguration.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljstools/qmljsmodelmanager.h>

#include <utils/qtcassert.h>

namespace QmlProfiler {
namespace Internal {

class PropertyVisitor: protected QmlJS::AST::Visitor
{
    QmlJS::AST::Node * _lastValidNode;
    unsigned _line;
    unsigned _col;
public:
    QmlJS::AST::Node * operator()(QmlJS::AST::Node *node, unsigned line, unsigned col)
    {
        _line = line;
        _col = col;
        _lastValidNode = 0;
        accept(node);
        return _lastValidNode;
    }

protected:
    using QmlJS::AST::Visitor::visit;

    void accept(QmlJS::AST::Node *node)
    {
        if (node)
            node->accept(this);
    }

    bool containsLocation(QmlJS::AST::SourceLocation start, QmlJS::AST::SourceLocation end)
    {
        return (_line > start.startLine || (_line == start.startLine && _col >= start.startColumn))
                && (_line < end.startLine || (_line == end.startLine && _col <= end.startColumn));
    }


    virtual bool preVisit(QmlJS::AST::Node *node)
    {
        if (QmlJS::AST::cast<QmlJS::AST::UiQualifiedId *>(node))
            return false;
        return containsLocation(node->firstSourceLocation(), node->lastSourceLocation());
    }

    virtual bool visit(QmlJS::AST::UiScriptBinding *ast)
    {
        _lastValidNode = ast;
        return true;
    }

    virtual bool visit(QmlJS::AST::UiPublicMember *ast)
    {
        _lastValidNode = ast;
        return true;
    }
};

QmlProfilerDetailsRewriter::QmlProfilerDetailsRewriter(QObject *parent)
    : QObject(parent)
{
}

void QmlProfilerDetailsRewriter::requestDetailsForLocation(int typeId,
                                                           const QmlEventLocation &location)
{
    const QString localFile = getLocalFile(location.filename());
    if (localFile.isEmpty())
        return;

    if (m_pendingEvents.isEmpty())
        connectQmlModel();

    m_pendingEvents.insert(localFile, {location, typeId});
}

QString QmlProfilerDetailsRewriter::getLocalFile(const QString &remoteFile)
{
    QString localFile;
    if (!m_filesCache.contains(remoteFile)) {
        localFile = m_projectFinder.findFile(remoteFile);
        m_filesCache[remoteFile] = localFile;
    } else {
        localFile = m_filesCache[remoteFile];
    }
    QFileInfo fileInfo(localFile);
    if (!fileInfo.exists() || !fileInfo.isReadable())
        return QString();
    if (!QmlJS::ModelManagerInterface::guessLanguageOfFile(localFile).isQmlLikeOrJsLanguage())
        return QString();

    return fileInfo.canonicalFilePath();
}

void QmlProfilerDetailsRewriter::reloadDocuments()
{
    if (!m_pendingEvents.isEmpty()) {
        if (QmlJS::ModelManagerInterface *manager = QmlJS::ModelManagerInterface::instance()) {
            manager->updateSourceFiles(m_pendingEvents.uniqueKeys(), false);
        } else {
            m_pendingEvents.clear();
            disconnectQmlModel();
            emit eventDetailsChanged();
        }
    } else {
        emit eventDetailsChanged();
    }
}

void QmlProfilerDetailsRewriter::rewriteDetailsForLocation(
        const QString &source, QmlJS::Document::Ptr doc, int typeId,
        const QmlEventLocation &location)
{
    PropertyVisitor propertyVisitor;
    QmlJS::AST::Node *node = propertyVisitor(doc->ast(), location.line(), location.column());

    if (!node)
        return;

    const quint32 startPos = node->firstSourceLocation().begin();
    const quint32 len = node->lastSourceLocation().end() - startPos;

    emit rewriteDetailsString(typeId, source.mid(startPos, len).simplified());
}

void QmlProfilerDetailsRewriter::connectQmlModel()
{
    if (auto manager = QmlJS::ModelManagerInterface::instance()) {
        connect(manager, &QmlJS::ModelManagerInterface::documentUpdated,
                this, &QmlProfilerDetailsRewriter::documentReady);
    }
}

void QmlProfilerDetailsRewriter::disconnectQmlModel()
{
    if (auto manager = QmlJS::ModelManagerInterface::instance()) {
        disconnect(manager, &QmlJS::ModelManagerInterface::documentUpdated,
                   this, &QmlProfilerDetailsRewriter::documentReady);
    }
}

void QmlProfilerDetailsRewriter::clearRequests()
{
    m_filesCache.clear();
    m_pendingEvents.clear();
    disconnectQmlModel();
}

void QmlProfilerDetailsRewriter::documentReady(QmlJS::Document::Ptr doc)
{
    const QString &fileName = doc->fileName();
    auto first = m_pendingEvents.find(fileName);

    // this could be triggered by an unrelated reload in Creator
    if (first == m_pendingEvents.end())
        return;

    // if the file could not be opened this slot is still triggered
    // but source will be an empty string
    QString source = doc->source();
    const bool sourceHasContents = !source.isEmpty();
    for (auto it = first; it != m_pendingEvents.end() && it.key() == fileName;) {
        if (sourceHasContents)
            rewriteDetailsForLocation(source, doc, it->typeId, it->location);
        it = m_pendingEvents.erase(it);
    }

    if (m_pendingEvents.isEmpty()) {
        disconnectQmlModel();
        emit eventDetailsChanged();
        m_filesCache.clear();
    }
}

void QmlProfilerDetailsRewriter::populateFileFinder(
        const ProjectExplorer::RunConfiguration *runConfiguration)
{
    // Prefer the given runConfiguration's target if available
    const ProjectExplorer::Target *target = runConfiguration ? runConfiguration->target() : nullptr;

    // If runConfiguration given, then use the project associated with that ...
    const ProjectExplorer::Project *startupProject = target ? target->project() : nullptr;

    // ... else try the session manager's global startup project ...
    if (!startupProject)
        startupProject = ProjectExplorer::SessionManager::startupProject();

    // ... and if that is null, use the first project available.
    const QList<ProjectExplorer::Project *> projects = ProjectExplorer::SessionManager::projects();
    if (!startupProject && !projects.isEmpty())
        startupProject = projects.first();

    QString projectDirectory;
    QStringList sourceFiles;

    // Sort files from startupProject to the front of the list ...
    if (startupProject) {
        projectDirectory = startupProject->projectDirectory().toString();
        sourceFiles.append(startupProject->files(ProjectExplorer::Project::SourceFiles));
    }

    // ... then add all the other projects' files.
    for (const ProjectExplorer::Project *project : projects) {
        if (project != startupProject)
            sourceFiles.append(project->files(ProjectExplorer::Project::SourceFiles));
    }

    // If no runConfiguration was given, but we've found a startupProject, then try to deduct a
    // target from that.
    if (!target && startupProject)
        target = startupProject->activeTarget();

    // ... and find the sysroot if we have any target at all.
    QString activeSysroot;
    if (target) {
        const ProjectExplorer::Kit *kit = target->kit();
        if (kit && ProjectExplorer::SysRootKitInformation::hasSysRoot(kit))
            activeSysroot = ProjectExplorer::SysRootKitInformation::sysRoot(kit).toString();
    }

    // Finally, do populate m_projectFinder
    m_projectFinder.setProjectDirectory(projectDirectory);
    m_projectFinder.setProjectFiles(sourceFiles);
    m_projectFinder.setSysroot(activeSysroot);
}

} // namespace Internal
} // namespace QmlProfiler
