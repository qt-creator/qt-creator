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

struct PendingEvent {
    QmlDebug::QmlEventLocation location;
    QString localFile;
    int requestId;
};

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
        return (_line > start.startLine || (_line == start.startLine && _col >= start.startColumn)) &&
                (_line < end.startLine || (_line == end.startLine && _col <= end.startColumn));
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

class QmlProfilerDetailsRewriter::QmlProfilerDetailsRewriterPrivate
{
public:
    QmlProfilerDetailsRewriterPrivate(QmlProfilerDetailsRewriter *qq,
                                      Utils::FileInProjectFinder *fileFinder)
                                      : m_projectFinder(fileFinder), q(qq) {}
    ~QmlProfilerDetailsRewriterPrivate() {}

    QList <PendingEvent> m_pendingEvents;
    QStringList m_pendingDocs;
    Utils::FileInProjectFinder *m_projectFinder;
    QMap<QString, QString> m_filesCache;

    QmlProfilerDetailsRewriter *q;
};

QmlProfilerDetailsRewriter::QmlProfilerDetailsRewriter(
        QObject *parent, Utils::FileInProjectFinder *fileFinder)
    : QObject(parent), d(new QmlProfilerDetailsRewriterPrivate(this, fileFinder))
{ }

QmlProfilerDetailsRewriter::~QmlProfilerDetailsRewriter()
{
    delete d;
}

void QmlProfilerDetailsRewriter::requestDetailsForLocation(int requestId,
        const QmlDebug::QmlEventLocation &location)
{
    QString localFile;
    if (!d->m_filesCache.contains(location.filename)) {
        localFile = d->m_projectFinder->findFile(location.filename);
        d->m_filesCache[location.filename] = localFile;
    } else {
        localFile = d->m_filesCache[location.filename];
    }
    QFileInfo fileInfo(localFile);
    if (!fileInfo.exists() || !fileInfo.isReadable())
        return;
    if (!QmlJS::ModelManagerInterface::guessLanguageOfFile(localFile).isQmlLikeLanguage())
        return;

    localFile = fileInfo.canonicalFilePath();

    PendingEvent ev = {location, localFile, requestId};
    d->m_pendingEvents << ev;
    if (!d->m_pendingDocs.contains(localFile)) {
        if (d->m_pendingDocs.isEmpty())
            connect(QmlJS::ModelManagerInterface::instance(),
                    &QmlJS::ModelManagerInterface::documentUpdated,
                    this,
                    &QmlProfilerDetailsRewriter::documentReady);

        d->m_pendingDocs << localFile;
    }
}

void QmlProfilerDetailsRewriter::reloadDocuments()
{
    if (!d->m_pendingDocs.isEmpty())
        QmlJS::ModelManagerInterface::instance()->updateSourceFiles(d->m_pendingDocs, false);
    else
        emit eventDetailsChanged();
}

void QmlProfilerDetailsRewriter::rewriteDetailsForLocation(QTextStream &textDoc,
        QmlJS::Document::Ptr doc, int requestId, const QmlDebug::QmlEventLocation &location)
{
    PropertyVisitor propertyVisitor;
    QmlJS::AST::Node *node = propertyVisitor(doc->ast(), location.line, location.column);

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
    d->m_filesCache.clear();
    d->m_pendingDocs.clear();
}

void QmlProfilerDetailsRewriter::documentReady(QmlJS::Document::Ptr doc)
{
    // this could be triggered by an unrelated reload in Creator
    if (!d->m_pendingDocs.contains(doc->fileName()))
        return;

    // if the file could not be opened this slot is still triggered but source will be an empty string
    QString source = doc->source();
    if (!source.isEmpty()) {
        QTextStream st(&source, QIODevice::ReadOnly);

        for (int i = d->m_pendingEvents.count()-1; i>=0; i--) {
            PendingEvent ev = d->m_pendingEvents[i];
            if (ev.localFile == doc->fileName()) {
                d->m_pendingEvents.removeAt(i);
                rewriteDetailsForLocation(st, doc, ev.requestId, ev.location);
            }
        }
    }

    d->m_pendingDocs.removeOne(doc->fileName());

    if (d->m_pendingDocs.isEmpty()) {
        disconnect(QmlJS::ModelManagerInterface::instance(),
                   &QmlJS::ModelManagerInterface::documentUpdated,
                   this,
                   &QmlProfilerDetailsRewriter::documentReady);
        emit eventDetailsChanged();
        d->m_filesCache.clear();
    }
}

} // namespace Internal
} // namespace QmlProfiler
