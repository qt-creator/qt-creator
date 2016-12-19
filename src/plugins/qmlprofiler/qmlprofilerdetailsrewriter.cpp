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

    m_pendingEvents.append({location, localFile, requestId});
    if (!m_pendingDocs.contains(localFile)) {
        if (m_pendingDocs.isEmpty() && QmlJS::ModelManagerInterface::instance())
            connect(QmlJS::ModelManagerInterface::instance(),
                    &QmlJS::ModelManagerInterface::documentUpdated,
                    this,
                    &QmlProfilerDetailsRewriter::documentReady);

        m_pendingDocs.append(localFile);
    }
}

void QmlProfilerDetailsRewriter::reloadDocuments()
{
    if (!m_pendingDocs.isEmpty()) {
        if (QmlJS::ModelManagerInterface *manager = QmlJS::ModelManagerInterface::instance()) {
            manager->updateSourceFiles(m_pendingDocs, false);
        } else {
            m_pendingDocs.clear();
            emit eventDetailsChanged();
        }
    } else {
        emit eventDetailsChanged();
    }
}

void QmlProfilerDetailsRewriter::rewriteDetailsForLocation(
        QTextStream &textDoc, QmlJS::Document::Ptr doc, int requestId,
        const QmlEventLocation &location)
{
    PropertyVisitor propertyVisitor;
    QmlJS::AST::Node *node = propertyVisitor(doc->ast(), location.line(), location.column());

    if (!node)
        return;

    qint64 startPos = node->firstSourceLocation().begin();
    qint64 len = node->lastSourceLocation().end() - startPos;

    textDoc.seek(startPos);
    QString details = textDoc.read(len).replace(QLatin1Char('\n'), QLatin1Char(' ')).simplified();

    emit rewriteDetailsString(requestId, details);
}

void QmlProfilerDetailsRewriter::clearRequests()
{
    m_filesCache.clear();
    m_pendingDocs.clear();
}

void QmlProfilerDetailsRewriter::documentReady(QmlJS::Document::Ptr doc)
{
    // this could be triggered by an unrelated reload in Creator
    if (!m_pendingDocs.contains(doc->fileName()))
        return;

    // if the file could not be opened this slot is still triggered but source will be an empty string
    QString source = doc->source();
    if (!source.isEmpty()) {
        QTextStream st(&source, QIODevice::ReadOnly);

        for (int i = m_pendingEvents.count() - 1; i >= 0; --i) {
            PendingEvent ev = m_pendingEvents[i];
            if (ev.localFile == doc->fileName()) {
                m_pendingEvents.removeAt(i);
                rewriteDetailsForLocation(st, doc, ev.requestId, ev.location);
            }
        }
    }

    m_pendingDocs.removeOne(doc->fileName());

    if (m_pendingDocs.isEmpty()) {
        disconnect(QmlJS::ModelManagerInterface::instance(),
                   &QmlJS::ModelManagerInterface::documentUpdated,
                   this,
                   &QmlProfilerDetailsRewriter::documentReady);
        emit eventDetailsChanged();
        m_filesCache.clear();
    }
}

} // namespace Internal
} // namespace QmlProfiler
