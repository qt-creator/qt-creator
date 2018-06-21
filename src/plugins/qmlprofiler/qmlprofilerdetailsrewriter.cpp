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
#include <qtsupport/baseqtversion.h>

#include <utils/qtcassert.h>

namespace QmlProfiler {
namespace Internal {

class PropertyVisitor: protected QmlJS::AST::Visitor
{
public:
    QmlJS::AST::Node *operator()(QmlJS::AST::Node *node, int line, int column)
    {
        QTC_ASSERT(line >= 0, return nullptr);
        QTC_ASSERT(column >= 0, return nullptr);
        QTC_ASSERT(node, return nullptr);
        m_line = line;
        m_column = column;
        m_lastValidNode = nullptr;
        node->accept(this);
        return m_lastValidNode;
    }

protected:
    using QmlJS::AST::Visitor::visit;

    bool preVisit(QmlJS::AST::Node *node) override
    {
        if (QmlJS::AST::cast<QmlJS::AST::UiQualifiedId *>(node))
            return false;
        return containsLocation(node->firstSourceLocation(), node->lastSourceLocation());
    }

    bool visit(QmlJS::AST::UiScriptBinding *ast) override
    {
        m_lastValidNode = ast;
        return true;
    }

    bool visit(QmlJS::AST::UiPublicMember *ast) override
    {
        m_lastValidNode = ast;
        return true;
    }

private:
    QmlJS::AST::Node *m_lastValidNode = nullptr;
    quint32 m_line = 0;
    quint32 m_column = 0;

    bool containsLocation(QmlJS::AST::SourceLocation start, QmlJS::AST::SourceLocation end)
    {
        return (m_line > start.startLine
                    || (m_line == start.startLine && m_column >= start.startColumn))
                && (m_line < end.startLine
                    || (m_line == end.startLine && m_column <= end.startColumn));
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

void QmlProfilerDetailsRewriter::clear()
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

void QmlProfilerDetailsRewriter::populateFileFinder(const ProjectExplorer::Target *target)
{
    QtSupport::BaseQtVersion::populateQmlFileFinder(&m_projectFinder, target);
    m_filesCache.clear();
}

} // namespace Internal
} // namespace QmlProfiler
