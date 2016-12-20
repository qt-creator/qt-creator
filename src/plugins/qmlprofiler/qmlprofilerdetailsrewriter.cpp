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

QmlProfilerDetailsRewriter::QmlProfilerDetailsRewriter(Utils::FileInProjectFinder *fileFinder,
                                                       QObject *parent)
    : QObject(parent), m_projectFinder(fileFinder)
{
}

void QmlProfilerDetailsRewriter::requestDetailsForLocation(int requestId,
                                                           const QmlEventLocation &location)
{
    QString localFile;
    const QString locationFile = location.filename();
    if (!m_filesCache.contains(locationFile)) {
        localFile = m_projectFinder->findFile(locationFile);
        m_filesCache[locationFile] = localFile;
    } else {
        localFile = m_filesCache[locationFile];
    }
    QFileInfo fileInfo(localFile);
    if (!fileInfo.exists() || !fileInfo.isReadable())
        return;
    if (!QmlJS::ModelManagerInterface::guessLanguageOfFile(localFile).isQmlLikeLanguage())
        return;

    localFile = fileInfo.canonicalFilePath();

    if (m_pendingEvents.isEmpty())
        connectQmlModel();

    m_pendingEvents.insert(localFile, {location, requestId});
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
        const QString &source, QmlJS::Document::Ptr doc, int requestId,
        const QmlEventLocation &location)
{
    PropertyVisitor propertyVisitor;
    QmlJS::AST::Node *node = propertyVisitor(doc->ast(), location.line(), location.column());

    if (!node)
        return;

    const quint32 startPos = node->firstSourceLocation().begin();
    const quint32 len = node->lastSourceLocation().end() - startPos;

    emit rewriteDetailsString(requestId, source.mid(startPos, len).simplified());
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
            rewriteDetailsForLocation(source, doc, it->requestId, it->location);
        it = m_pendingEvents.erase(it);
    }

    if (m_pendingEvents.isEmpty()) {
        disconnectQmlModel();
        emit eventDetailsChanged();
        m_filesCache.clear();
    }
}

} // namespace Internal
} // namespace QmlProfiler
